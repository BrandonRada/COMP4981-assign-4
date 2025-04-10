//
// Created by brandon-rada on 4/9/25.
//
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SERVER_PORT 8080
#define SERVER_ADDR "127.0.0.1"
#define BUFFER_SIZE 4096

void send_request(const char *method, const char *path, const char *body);

void send_request(const char *method, const char *path, const char *body)
{
    int                sockfd;
    struct sockaddr_in server_addr;
    char               request[BUFFER_SIZE];
    char               response[BUFFER_SIZE];
    ssize_t            bytes_received;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_ADDR, &server_addr.sin_addr);

    // Connect to server
    if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Prepare HTTP request
    if(strcmp(method, "POST") == 0)
    {
        snprintf(request,
                 sizeof(request),
                 "%s %s HTTP/1.1\r\n"
                 "Host: localhost\r\n"
                 "Content-Type: application/x-www-form-urlencoded\r\n"
                 "Content-Length: %zu\r\n"
                 "\r\n"
                 "%s",
                 method,
                 path,
                 strlen(body),
                 body);
    }
    else
    {
        snprintf(request,
                 sizeof(request),
                 "%s %s HTTP/1.1\r\n"
                 "Host: localhost\r\n"
                 "\r\n",
                 method,
                 path);
    }

    // Send request
    send(sockfd, request, strlen(request), 0);

    // Read and print response

    while((bytes_received = recv(sockfd, response, sizeof(response) - 1, 0)) > 0)
    {
        response[bytes_received] = '\0';
        printf("%s", response);
    }

    close(sockfd);
}

int main(int argc, char *argv[])
{
    const char *method = argv[1];
    const char *path   = argv[2];
    const char *body   = (argc > 3) ? argv[3] : "";

    if(argc < 3)
    {
        printf("Usage:\n");
        printf("  %s METHOD PATH [BODY]\n", argv[0]);
        printf("Examples:\n");
        printf("  %s GET /index.html\n", argv[0]);
        printf("  %s POST / \"username=chatgpt\"\n", argv[0]);
        return EXIT_FAILURE;
    }

    send_request(method, path, body);
    return EXIT_SUCCESS;
}
