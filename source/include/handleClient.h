#ifndef HANDLE_CLIENT
#define HANDLE_CLIENT

#include <arpa/inet.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct
{
    int                socket_fd;    // cppcheck-suppress unusedStructMember
    struct sockaddr_in client_addr;
} client_info;

void handle_client(int sockfd);

#endif    // HANDLE_CLIENT
