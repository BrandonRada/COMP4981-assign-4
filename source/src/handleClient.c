#include "../include/handleClient.h"
#include <arpa/inet.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 4096
#define EIGHT 8
#define MAX_PATH 256

void handle_client(int sockfd)
{
    char               buffer[MAX_BUFFER_SIZE];
    int                client_fd;
    struct sockaddr_in client_addr;
    socklen_t          client_len = sizeof(client_addr);
    char               method[EIGHT];
    char               path[MAX_PATH];
    ssize_t            bytes_received;    // Catches recv return type

    void (*handle_request)(int, const char *, const char *);    // Declare early
    void *handle_request_lib;                                   // Declare early

    // Accept client connection
    client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
    if(client_fd < 0)
    {
        perror("accept");
        return;
    }

    bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if(bytes_received < 0)
    {
        perror("recv");
        close(client_fd);
        return;
    }

    buffer[bytes_received] = '\0';    // Null-terminate the received data

    // Simple HTTP request parsing (GET, HEAD, POST)
    if(sscanf(buffer, "%7s %255s", method, path) != 2)
    {
        perror("Invalid HTTP request");
        close(client_fd);
        return;
    }

    // Dynamically load the request handler function
    handle_request_lib = dlopen("./librequest_handler.so", RTLD_NOW);
    if(!handle_request_lib)
    {
        fprintf(stderr, "Failed to load library: %s\n", dlerror());
        close(client_fd);
        return;
    }

    *(void **)(&handle_request) = dlsym(handle_request_lib, "handle_request");
    if(!handle_request)
    {
        fprintf(stderr, "Failed to find handle_request function: %s\n", dlerror());
        dlclose(handle_request_lib);
        close(client_fd);
        return;
    }

    // Call the dynamically loaded request handler
    handle_request(client_fd, method, path);

    // Clean up
    dlclose(handle_request_lib);
    close(client_fd);
}
