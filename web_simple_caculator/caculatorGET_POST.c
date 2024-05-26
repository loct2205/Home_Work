#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

void *client_proc(void *);

int main() {
    // Tạo socket cho kết nối
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // Khai báo địa chỉ server
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);

    // Gán socket với cấu trúc địa chỉ
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        return 1;
    }

    // Chuyển socket sang trạng thái chờ kết nối
    if (listen(listener, 10)) {
        perror("listen() failed");
        return 1;
    }

    while (1) {
        printf("Waiting for new client\n");
        int client = accept(listener, NULL, NULL);
        if (client < 0) {
            perror("accept() failed");
            continue;
        }
        printf("New client accepted, client = %d\n", client);
        
        pthread_t tid;
        pthread_create(&tid, NULL, client_proc, &client);
        pthread_detach(tid);
    }

    return 0;
}

void *client_proc(void *arg) {
    int client = *(int *)arg;
    char buf[2048];

    // Nhận dữ liệu từ client
    int ret = recv(client, buf, sizeof(buf), 0);
    if (ret <= 0) {
        close(client);
        pthread_exit(NULL);
    }

    buf[ret] = 0;
    printf("Received from %d: %s\n", client, buf);
    
    if (strncmp(buf, "GET /calc?", 10) == 0) {
        // Xử lý lệnh GET
        char var_a[100], var_b[100], var_cmd[100];
        double a, b, result;
        int calc_result = 1;

        sscanf(buf, "GET /calc?a=%[^&]&b=%[^&]&cmd=%s ", var_a, var_b, var_cmd);
        
        a = atof(var_a);
        b = atof(var_b);

        if (strcmp(var_cmd, "add") == 0) {
            result = a + b;
        } else if (strcmp(var_cmd, "sub") == 0) {
            result = a - b;
        } else if (strcmp(var_cmd, "mul") == 0) {
            result = a * b;
        } else if (strcmp(var_cmd, "div") == 0) {
            if (b == 0) {
                calc_result = 0;
            } else {
                result = a / b;
            }
        } else {
            calc_result = 0;
        }

        if (calc_result) {
            sprintf(buf, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Calculation Result</h1><p>%f %s %f = %f</p></body></html>", a, var_cmd, b, result);
        } else {
            strcpy(buf, "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Error: Invalid calculation</h1></body></html>");
        }
        send(client, buf, strlen(buf), 0);
    } else if (strncmp(buf, "POST /calc", 10) == 0) {
        // Xử lý lệnh POST
        char *body = strstr(buf, "\r\n\r\n");
        if (body) {
            body += 4;
            char var_a[100], var_b[100], var_cmd[100];
            double a, b, result;
            int calc_result = 1;

            sscanf(body, "a=%[^&]&b=%[^&]&cmd=%s", var_a, var_b, var_cmd);
            
            a = atof(var_a);
            b = atof(var_b);

            if (strcmp(var_cmd, "add") == 0) {
                result = a + b;
            } else if (strcmp(var_cmd, "sub") == 0) {
                result = a - b;
            } else if (strcmp(var_cmd, "mul") == 0) {
                result = a * b;
            } else if (strcmp(var_cmd, "div") == 0) {
                if (b == 0) {
                    calc_result = 0;
                } else {
                    result = a / b;
                }
            } else {
                calc_result = 0;
            }

            if (calc_result) {
                sprintf(buf, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Calculation Result</h1><p>%f %s %f = %f</p></body></html>", a, var_cmd, b, result);
            } else {
                strcpy(buf, "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Error: Invalid calculation</h1></body></html>");
            }
            send(client, buf, strlen(buf), 0);
        }
    } else {
        // Trả về trang web Hello World cho các yêu cầu khác
        strcpy(buf, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Hello World</h1>");
        send(client, buf, strlen(buf), 0);
    }

    close(client);
    pthread_exit(NULL);
}
