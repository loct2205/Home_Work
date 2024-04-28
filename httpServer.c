#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 8080
#define MAX_CLIENTS 10

void handle_client(int client) {
    char buf[256];
    int ret = recv(client, buf, sizeof(buf) - 1, 0);
    if (ret > 0) {
        buf[ret] = '\0';
        printf("Received from client %d: %s\n", client, buf);
        
        char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Xin chao cac ban</h1></body></html>";
        send(client, msg, strlen(msg), 0);
    }

    close(client);
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(listener, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            while (1) {
                int client = accept(listener, NULL, NULL);
                if (client >= 0) {
                    handle_client(client);
                }
            }
            exit(0); 
        } else if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }
    }

    while (1) {
        pause(); 
    }

    close(listener);
    return 0;
}
