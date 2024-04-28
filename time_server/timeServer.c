#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h> 

#define PORT 8080
#define MAX_CLIENTS 10

bool is_valid_command(const char *command) {
    return strncmp(command, "GET_TIME ", 9) == 0;
}

void trim(char *str) {
    char *end;
    while (isspace((unsigned char) *str)) str++; 
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char) *end)) end--; 
    *(end + 1) = '\0'; 
}

const char *get_time_in_format(const char *format) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    static char time_str[20];

    if (strcmp(format, "dd/mm/yyyy") == 0) {
        snprintf(time_str, sizeof(time_str), "%02d/%02d/%04d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
    } else if (strcmp(format, "dd/mm/yy") == 0) {
        snprintf(time_str, sizeof(time_str), "%02d/%02d/%02d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year % 100);
    } else if (strcmp(format, "mm/dd/yyyy") == 0) {
        snprintf(time_str, sizeof(time_str), "%02d/%02d/%04d\n", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900);
    } else if (strcmp(format, "mm/dd/yy") == 0) {
        snprintf(time_str, sizeof(time_str), "%02d/%02d/%02d\n", tm.tm_mon + 1, tm.tm_mday, tm.tm_year % 100);
    } else {
        return "Invalid format";
    }

    return time_str;
}

void handle_client(int client) {
    char buf[256];

    while (true) { 
        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) { 
            break; 
        }

        buf[ret] = '\0'; 
        printf("Received command: '%s'\n", buf); 

        if (is_valid_command(buf)) { 
            char format[256];
            strncpy(format, buf + 9, sizeof(format) - 1);
            trim(format); 
            printf("Format: '%s'\n", format); 

            const char *time_str = get_time_in_format(format);
            if (strcmp(time_str, "Invalid format") == 0) {
                send(client, time_str, strlen(time_str), 0); 
            } else {
                send(client, time_str, strlen(time_str), 0); 
            }
        } else {
            send(client, "Invalid command", strlen("Invalid command"), 0); 
        }
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
            while (true) { 
                int client = accept(listener, NULL, NULL);
                if (client >= 0) {
                    handle_client(client); 
                }
            }
        } else if (pid < 0) { 
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }
    }

    while (true) {
        pause(); 
    }

    close(listener); 
    return 0;
}
