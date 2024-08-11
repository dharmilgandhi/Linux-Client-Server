#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 6666

void send_command(int socket, char *command);

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char command[1024];

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    int connectStatus = connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (connectStatus == -1) {
        perror("Connection failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
            char strData[255];
  
        recv(client_socket, strData, sizeof(strData), 0);
  
        printf("Message: %s\n", strData);
    while (1) {
        printf("Enter command: ");
        fgets(command, 1024, stdin);
        command[strcspn(command, "\n")] = 0; // Remove newline character

        send_command(client_socket, command);
    }

    close(client_socket);
    return 0;
}

void send_command(int socket, char *command) {
    // Send command to Smain
    // send(socket, command, strlen(command), 0);
    if ((send(socket,command, strlen(command),0))== -1) {
            fprintf(stderr, "Failure Sending Message\n");
            close(socket);
            exit(1);
        }
        else {
            printf("Message being sent: %s\n",command);
        }
}
