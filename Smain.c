#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORT 6666


void handle_client(int client_socket);
void process_command(int client_socket, char *command);

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 15) == -1) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Smain server listening on port %d\n", PORT);

    // Infinite loop to accept connections
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        if (client_socket == -1) {
            perror("Client connection failed");
            continue;
        }

        // Fork a child process to handle the client
        if (fork() == 0) {
            printf("Successfully Connected to a client\n");
            // close(server_socket);
            send(client_socket,"Hello",5,0);
            handle_client(client_socket);
            close(client_socket);
            exit(0);
        }
        
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

void handle_client(int client_socket) {
    
    char buffer[1024];
    int bytes_received;

    // Infinite loop to process client commands
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            printf("Client disconnected\n");
            break;
        }
        printf("%s\n",buffer);
        process_command(client_socket, buffer);
    }
}

void process_command(int client_socket, char *command) {
    char response[1024];
    char filename[256];
    char filepath[512];
    char destinationPath[512];
    FILE *file;
    int bytes_received;

    // Command: ufile <filename> <file_extension>
    if (strncmp(command, "ufile", 5) == 0) {
        sscanf(command, "ufile %s", filename);
        
        if (strstr(filename, ".c")) {
            // Save .c files locally
            snprintf(filepath, sizeof(filepath), "smain/folder1/folder2/%s", filename);
            // printf("FIle Path:%s\n",filepath);

            printf("File copying started\n");
        int fd1 = open(filename,O_RDONLY);
        int fd2 = open(filepath,O_CREAT|O_RDWR,0711);

        char buff[4096];
        long int n;
        long int x;

        while((n = read(fd1,buff,sizeof(buff))) > 0){
                // printf("Bytes read: %d\n",n);
                // printf("Char: %s\n",buff);
                if (write(fd2, buff, n) == -1) {
            perror("Error writing to destination file");
            close(fd1);
            close(fd2);
            return;
        }
        }


        close(fd1);
        close(fd2);
        printf("File copying compelted\n");


            // Receive file data
            while ((bytes_received = recv(client_socket, response, sizeof(response), 0)) > 0) {
                printf("Response:%s\n",response);
                fwrite(response, 1, bytes_received, file);
            }

        } else if (strstr(filename, ".pdf")) {
            // Forward to Spdf server
            forward_file(client_socket, filename, "localhost", 8081);
        } else if (strstr(filename, ".txt")) {
            // Forward to Stext server
            forward_file(client_socket, filename, "localhost", 8082);
        }
    }
    // Command: dfile <filename>
    else if (strncmp(command, "dfile", 5) == 0) {
        sscanf(command, "dfile %s", filename);
        snprintf(filepath, sizeof(filepath), "sc/%s", filename);
        file = fopen(filepath, "rb");
        if (file == NULL) {
            strcpy(response, "File not found");
            send(client_socket, response, strlen(response), 0);
            return;
        }

        // Send file data
        while ((bytes_received = fread(response, 1, sizeof(response), file)) > 0) {
            send(client_socket, response, bytes_received, 0);
        }
        fclose(file);
    }
    // Command: rmfile <filename>
    else if (strncmp(command, "rmfile", 6) == 0) {
        sscanf(command, "rmfile %s", filename);
        snprintf(filepath, sizeof(filepath), "sc/%s", filename);
        if (remove(filepath) == 0) {
            strcpy(response, "File deleted successfully");
        } else {
            strcpy(response, "File deletion failed");
        }
        send(client_socket, response, strlen(response), 0);
    }
    // Command: dtar <tar_name>
    else if (strncmp(command, "dtar", 4) == 0) {
        char tar_command[512];
        sscanf(command, "dtar %s", filename);
        snprintf(tar_command, sizeof(tar_command), "tar -czvf %s.tar.gz sc", filename);
        system(tar_command);
        snprintf(filepath, sizeof(filepath), "%s.tar.gz", filename);
        file = fopen(filepath, "rb");
        if (file == NULL) {
            strcpy(response, "Tar creation failed");
            send(client_socket, response, strlen(response), 0);
            return;
        }

        // Send tar file data
        while ((bytes_received = fread(response, 1, sizeof(response), file)) > 0) {
            send(client_socket, response, bytes_received, 0);
        }
        fclose(file);
    }
    // Command: display
    else if (strncmp(command, "display", 7) == 0) {
        system("ls sc > directory_list.txt");
        file = fopen("directory_list.txt", "r");
        if (file == NULL) {
            strcpy(response, "Failed to display directory contents");
            send(client_socket, response, strlen(response), 0);
            return;
        }

        // Send directory listing
        while (fgets(response, sizeof(response), file) != NULL) {
            send(client_socket, response, strlen(response), 0);
        }
        fclose(file);
    }
}

void forward_file(int client_socket, char *filename, char *server_ip, int port) {
    int server_socket;
    struct sockaddr_in server_addr;
    char buffer[1024];
    int bytes_received;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection to server failed");
        close(server_socket);
        return;
    }

    // Forward file name
    send(server_socket, filename, strlen(filename), 0);

    // Receive file data from client and forward it to the appropriate server
    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        send(server_socket, buffer, bytes_received, 0);
    }

    close(server_socket);
}

