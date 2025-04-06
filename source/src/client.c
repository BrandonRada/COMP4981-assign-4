#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __APPLE__
    #define SOCK_CLOEXEC 0
#endif

#define BUFFER_SIZE 4096
#define TEN 10

int main(int argc, char *argv[])
{
    const char        *server_ip;
    long               server_port;
    const char        *method;
    const char        *path;
    const char        *post_data = NULL;
    int                sockfd;
    struct sockaddr_in server_addr;
    char               send_buffer[BUFFER_SIZE];
    char               recv_buffer[BUFFER_SIZE];
    ssize_t            bytes_received;

    // Check argument count
    if(argc < 5)
    {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  %s <server_ip> <server_port> GET|HEAD <path>\n", argv[0]);
        fprintf(stderr, "  %s <server_ip> <server_port> POST <path> <data>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    server_ip   = argv[1];
    server_port = strtol(argv[2], NULL, TEN);
    method      = argv[3];
    path        = argv[4];

    if(strcmp(method, "POST") == 0)
    {
        if(argc < 6)
        {
            fprintf(stderr, "POST requires data argument.\n");
            exit(EXIT_FAILURE);
        }
        post_data = argv[5];
    }

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if(sockfd < 0)
    {
        perror("Failed to create socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons((uint16_t)server_port);

    if(inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        perror("Invalid address/Address not supported");
        close(sockfd);
        return 1;
    }

    // Connect to server
    if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        close(sockfd);
        return 1;
    }

    // Build HTTP request
    if(strcmp(method, "POST") == 0 && post_data != NULL)
    {
        snprintf(send_buffer,
                 BUFFER_SIZE,
                 "%s %s HTTP/1.1\r\n"
                 "Host: %s\r\n"
                 "Content-Type: application/x-www-form-urlencoded\r\n"
                 "Content-Length: %zu\r\n"
                 "Connection: close\r\n"
                 "\r\n"
                 "%s",
                 method,
                 path,
                 server_ip,
                 strlen(post_data),
                 post_data);
    }
    else
    {
        snprintf(send_buffer,
                 BUFFER_SIZE,
                 "%s %s HTTP/1.1\r\n"
                 "Host: %s\r\n"
                 "Connection: close\r\n"
                 "\r\n",
                 method,
                 path,
                 server_ip);
    }

    // Send HTTP request
    if(send(sockfd, send_buffer, strlen(send_buffer), 0) < 0)
    {
        perror("Failed to send request");
        close(sockfd);
        return 1;
    }

    printf("Request sent:\n%s\n", send_buffer);

    // Receive and print server response
    while((bytes_received = recv(sockfd, recv_buffer, BUFFER_SIZE - 1, 0)) > 0)
    {
        recv_buffer[bytes_received] = '\0';    // Null-terminate the received data
        printf("%s", recv_buffer);
    }

    if(bytes_received < 0)
    {
        perror("Failed to receive response");
    }

    close(sockfd);
    return 0;
}
