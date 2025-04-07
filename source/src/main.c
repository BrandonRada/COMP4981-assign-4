#include "../include/handleClient.h"
#include <arpa/inet.h>
#include <dlfcn.h>    // For dynamic library loading
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>    // For UNIX domain sockets
#include <sys/wait.h>
#include <unistd.h>

#ifdef __APPLE__
    #define SOCK_CLOEXEC 0
#endif

#define PORT 8080
#define NUM_WORKERS 4                                 // The number of worker processes
#define DOMAIN_SOCKET_PATH "/tmp/webserver_socket"    // Domain socket path

static int sockfd;              // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
static int domain_socket_fd;    // Domain socket for IPC

// Define function pointers for dynamic library loading
static void (*handle_request)(client_info *);
static void worker_process(int d_socket_fd);
int         main(void);

__attribute__((noreturn)) static void handle_sigint(int signal)
{
    (void)signal;
    close(sockfd);
    close(domain_socket_fd);    // Close domain socket
    _exit(0);
}

// Worker process function
void worker_process(int d_socket_fd)
{
    struct sockaddr_in client_addr;
    socklen_t          client_len = sizeof(client_addr);
    (void)d_socket_fd;

    for(;;)
    {
        // Accept connection
        client_info *info = (client_info *)malloc(sizeof(client_info));
        if(!info)
        {
            perror("malloc");
            continue;
        }

        info->socket_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if(info->socket_fd < 0)
        {
            if(errno == EINTR)
            {
                free(info);
                break;
            }
            perror("accept");
            free(info);
            continue;
        }

        info->client_addr = client_addr;
        printf("Worker %d: connection accepted\n", getpid());

        // Dynamically load the shared library if necessary
        if(dlopen("librequest_handler.so", RTLD_NOW))
        {
            handle_request = (void (*)(client_info *))dlsym(dlopen("librequest_handler.so", RTLD_NOW), "handle_request");
            if(!handle_request)
            {
                fprintf(stderr, "Failed to load request handler function: %s\n", dlerror());
                free(info);
                continue;
            }
        }

        // Handle the client request using the dynamically loaded function
        handle_request(info);

        // Clean up
        close(info->socket_fd);
        free(info);
    }
}

// Set up and start the server
int main(void)
{
    struct sockaddr_in host_addr;
    struct sockaddr_un domain_addr;
    int                opt = 1;

    // Create the main listening socket
    sockfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if(sockfd == -1)
    {
        perror("socket");
        return 1;
    }

    // Set socket options
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        close(sockfd);
        return 1;
    }

    // Set up server address structure
    host_addr.sin_family      = AF_INET;
    host_addr.sin_port        = htons(PORT);
    host_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the server socket
    if(bind(sockfd, (struct sockaddr *)&host_addr, sizeof(host_addr)) != 0)
    {
        perror("bind");
        close(sockfd);
        return 1;
    }

    // Listen for incoming connections
    if(listen(sockfd, SOMAXCONN) != 0)
    {
        perror("listen");
        close(sockfd);
        return 1;
    }

    // Create domain socket for IPC
    domain_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(domain_socket_fd == -1)
    {
        perror("domain socket");
        return 1;
    }

    memset(&domain_addr, 0, sizeof(domain_addr));
    domain_addr.sun_family = AF_UNIX;
    strcpy(domain_addr.sun_path, DOMAIN_SOCKET_PATH);

    if(bind(domain_socket_fd, (struct sockaddr *)&domain_addr, sizeof(domain_addr)) < 0)
    {
        perror("domain socket bind");
        close(domain_socket_fd);
        close(sockfd);
        return 1;
    }

    if(listen(domain_socket_fd, NUM_WORKERS) != 0)
    {
        perror("domain socket listen");
        close(domain_socket_fd);
        close(sockfd);
        return 1;
    }

    // Handle SIGINT for graceful shutdown
    signal(SIGINT, handle_sigint);

    // Prefork worker processes
    for(int i = 0; i < NUM_WORKERS; i++)
    {
        pid_t pid = fork();
        if(pid == -1)
        {
            perror("fork");
            close(sockfd);
            close(domain_socket_fd);
            return 1;
        }

        if(pid == 0)
        {                     // Child process
            close(sockfd);    // Child doesn't need the server socket
            worker_process(domain_socket_fd);
            _exit(0);    // Child exits after processing
        }
    }

    // Parent process waits for children to terminate
    for(int i = 0; i < NUM_WORKERS; i++)
    {
        wait(NULL);    // Wait for each child process to terminate
    }

    // Cleanup resources
    close(sockfd);
    close(domain_socket_fd);
    return 0;
}
