//
// Created by brandon-rada on 4/6/25.
//

// server.c

#include "handleClient.h"
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT 8080
#define NUM_WORKERS 4

static int sockfd;

// Signal handler to cleanly shut down the server
__attribute__((noreturn)) static void handle_sigint(int signal)
{
    (void)signal;
    close(sockfd);
    _exit(0);
}

// Function to clean up child processes on server shutdown
void cleanup_children()
{
    int   status;
    pid_t pid;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        if(WIFEXITED(status))
        {
            printf("Worker %d terminated normally\n", pid);
        }
        else if(WIFSIGNALED(status))
        {
            printf("Worker %d terminated by signal\n", pid);
            // Restart worker process if it crashed
            pid_t new_pid = fork();
            if(new_pid == 0)
            {
                close(sockfd);    // Child doesn't need listening socket
                while(1)
                {
                    int                client_fd;
                    struct sockaddr_in client_addr;
                    socklen_t          client_len = sizeof(client_addr);
                    client_fd                     = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
                    if(client_fd >= 0)
                    {
                        handle_client(client_fd);
                        close(client_fd);
                    }
                }
                _exit(0);
            }
        }
    }
}

int main(void)
{
    struct sockaddr_in host_addr;
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
            return 1;
        }

        if(pid == 0)
        {                     // Child process
            close(sockfd);    // Child doesn't need the server socket
            while(1)
            {
                int                client_fd;
                struct sockaddr_in client_addr;
                socklen_t          client_len = sizeof(client_addr);

                // Accept client connection
                client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
                if(client_fd < 0)
                {
                    perror("accept");
                    continue;    // Continue listening for other clients
                }

                // Handle the client request
                handle_client(client_fd);

                // Close the client connection
                close(client_fd);
            }
            _exit(0);    // Child exits after finishing work
        }
    }

    // Parent process should clean up child processes on shutdown
    cleanup_children();

    // Parent process waits for children to terminate
    for(int i = 0; i < NUM_WORKERS; i++)
    {
        wait(NULL);    // Wait for each child process to terminate
    }

    // Cleanup resources
    close(sockfd);
    return 0;
}
