#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CLIENTS 2
#define BUFFER_SIZE 1024

typedef struct {
    int client_socket;
    pthread_t thread_id;
} ClientInfo;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int connected_clients = 0;
ClientInfo clients[MAX_CLIENTS];

void *handle_client(void *arg) {
    int client_index = *((int *)arg);
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes_received = recv(clients[client_index].client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            // Client disconnected
            printf("Client %d disconnected\n", client_index + 1);
            close(clients[client_index].client_socket);
            pthread_mutex_lock(&mutex);
            connected_clients--;
            pthread_mutex_unlock(&mutex);
            break;
        }

        buffer[bytes_received] = '\0'; // Ensure null-terminated string

        // Forward the message to the paired client
        int paired_index = (client_index + 1) % 2;
        int bytes_sent = send(clients[paired_index].client_socket, buffer, bytes_received, 0);
        if (bytes_sent <= 0) {
            // Unable to send message to paired client
            printf("Failed to send message to paired client\n");
            close(clients[client_index].client_socket);
            pthread_mutex_lock(&mutex);
            connected_clients--;
            pthread_mutex_unlock(&mutex);
            break;
        }
    }

    pthread_exit(NULL);
}

void *accept_clients(void *arg) {
    int server_socket = *((int *)arg);
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (1) {
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);

        if (client_socket < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        printf("New client connected\n");

        pthread_mutex_lock(&mutex);
        clients[connected_clients].client_socket = client_socket;
        pthread_mutex_unlock(&mutex);

        connected_clients++;

        if (connected_clients == MAX_CLIENTS) {
            // Pair clients
            for (int i = 0; i < MAX_CLIENTS; i++) {
                pthread_create(&clients[i].thread_id, NULL, handle_client, &i);
                pthread_detach(clients[i].thread_id);
            }
            connected_clients = 0; // Reset connected_clients for next pair
        }
        else if (connected_clients > MAX_CLIENTS) {
            // More than MAX_CLIENTS clients connected, disconnect the extra client
            printf("Too many clients connected. Disconnecting extra client.\n");
            close(client_socket);
        }
    }
}

int main() {
    int server_socket;
    struct sockaddr_in server_addr;
    
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Bind to port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8000);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, accept_clients, &server_socket);
    pthread_join(thread_id, NULL);

    close(server_socket);

    return 0;
}
