//
// Created by brandon-rada on 4/9/25.
//
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
#define MIN_ARGS 5
#define BODY_ARG_INDEX 5
#define MAX_PORT 65535
#define TEN 10

void send_request(const char *ip, int port, const char *method, const char *path, const char *body);

void send_request(const char *ip, int port, const char *method, const char *path, const char *body)
{
    int                sockfd;
    struct sockaddr_in server_addr;
    char               request[BUFFER_SIZE];
    char               response[BUFFER_SIZE];
    ssize_t            bytes_received;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons((uint16_t)port);

    if(inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Build HTTP request
    if(strcmp(method, "POST") == 0)
    {
        snprintf(request,
                 sizeof(request),
                 "%s %s HTTP/1.1\r\n"
                 "Host: %s:%d\r\n"
                 "Content-Type: application/x-www-form-urlencoded\r\n"
                 "Content-Length: %zu\r\n"
                 "\r\n"
                 "%s",
                 method,
                 path,
                 ip,
                 port,
                 strlen(body),
                 body);
    }
    else
    {
        snprintf(request,
                 sizeof(request),
                 "%s %s HTTP/1.1\r\n"
                 "Host: %s:%d\r\n"
                 "\r\n",
                 method,
                 path,
                 ip,
                 port);
    }

    send(sockfd, request, strlen(request), 0);

    while((bytes_received = recv(sockfd, response, sizeof(response) - 1, 0)) > 0)
    {
        response[bytes_received] = '\0';
        printf("%s", response);
    }

    close(sockfd);
}

int main(int argc, char *argv[])
{
    const char *ip;
    char       *endptr;
    long        port_long;
    int         port;
    const char *method;
    const char *path;
    const char *body;

    if(argc < MIN_ARGS)
    {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  %s <IP> <PORT> <METHOD> <PATH> [BODY]\n", argv[0]);
        fprintf(stderr, "Example:\n");
        fprintf(stderr, "  %s 127.0.0.1 8080 POST / \"username=brandon\"\n", argv[0]);
        return EXIT_FAILURE;
    }

    ip = argv[1];

    port_long = strtol(argv[2], &endptr, TEN);
    if(*endptr != '\0' || port_long <= 0 || port_long > MAX_PORT)
    {
        fprintf(stderr, "Invalid port number: %s\n", argv[2]);
        return EXIT_FAILURE;
    }
    port = (int)port_long;

    method = argv[3];
    path   = argv[4];
    body   = (argc >= BODY_ARG_INDEX + 1) ? argv[BODY_ARG_INDEX] : "";

    send_request(ip, port, method, path, body);
    return EXIT_SUCCESS;
}
