//
// Created by brandon-rada on 4/9/25.
//
#include "server.h"
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef __APPLE__
    #define SOCK_CLOEXEC 0
#endif

#define PORT 8080
#define BACKLOG 10
// Global so the handler can access it
static int server_fd = -1;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

// void       sigint_handler(int signum);
__attribute__((noreturn)) void sigint_handler(int signum);

__attribute__((noreturn)) void sigint_handler(int signum)
{
    const char *msg = "\nReceived SIGINT. Shutting down...\n";
    (void)signum;
    write(STDERR_FILENO, msg, strlen(msg));    // Async-signal-safe
    close(server_fd);
    _exit(EXIT_SUCCESS);    // Also safe
}

int main(void)
{
    struct sockaddr_in addr;
    int                opt;

    // Set up signal handler
    signal(SIGINT, sigint_handler);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);

    if(bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd, BACKLOG) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);
    start_workers(server_fd);

    while(1)
    {
        pause();    // Parent process waits for signals
    }
}
