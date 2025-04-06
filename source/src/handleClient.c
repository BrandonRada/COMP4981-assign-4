#include "../include/handleClient.h"

#define MAX_BUFFER_SIZE 4096

void handle_client(int sockfd)
{
    char               buffer[MAX_BUFFER_SIZE];
    int                client_fd;
    struct sockaddr_in client_addr;
    socklen_t          client_len = sizeof(client_addr);

    // Accept client connection
    client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
    if(client_fd < 0)
    {
        perror("accept");
        return;
    }

    int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
    if(bytes_received < 0)
    {
        perror("recv");
        close(client_fd);
        return;
    }

    buffer[bytes_received] = '\0';    // Null-terminate the received data

    // Simple HTTP request parsing (GET, HEAD, POST)
    char method[8], path[256];
    if(sscanf(buffer, "%7s %255s", method, path) != 2)
    {
        perror("Invalid HTTP request");
        close(client_fd);
        return;
    }

    // Dynamically load the request handler function
    void (*handle_request)(int client_fd, const char *method, const char *path);
    void *handle_request_lib = dlopen("./librequest_handler.so", RTLD_NOW);
    if(!handle_request_lib)
    {
        fprintf(stderr, "Failed to load library: %s\n", dlerror());
        close(client_fd);
        return;
    }

    handle_request = dlsym(handle_request_lib, "handle_request");
    if(!handle_request)
    {
        fprintf(stderr, "Failed to find handle_request function: %s\n", dlerror());
        close(client_fd);
        return;
    }

    // Call the dynamically loaded request handler
    handle_request(client_fd, method, path);

    // Close client connection
    close(client_fd);
}