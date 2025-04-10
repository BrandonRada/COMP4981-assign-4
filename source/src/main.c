//
// Created by brandon-rada on 4/9/25.
//
#include "handler.h"
#include "server.h"
#include <dlfcn.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __APPLE__
    #define SOCK_CLOEXEC 0
#endif

#define PORT 8080
#define BACKLOG 10
#define SOURCE_FILE "src/handler.c"
#define LIBRARY_FILE "lib/handler.so"
#define COMPILE_CMD "gcc -fPIC -shared -o " LIBRARY_FILE " " SOURCE_FILE

// Global so the handler can access it
static int   server_fd      = -1;      // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
static void *handler_handle = NULL;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

static void reload_handler(void)
{
    void *new_handle = dlopen(LIBRARY_FILE, RTLD_LAZY);
    if(!new_handle)
    {
        perror("dlopen");
    }
    else
    {
        if(handler_handle)
        {
            dlclose(handler_handle);    // Close the old library
        }
        handler_handle = new_handle;
    }
}

static void check_and_recompile(void)
{
    static time_t last_modified = 0;
    struct stat   file_stat;

    if(stat(SOURCE_FILE, &file_stat) == 0 && file_stat.st_mtime > last_modified)
    {
        printf("Detected changes in handler.c, recompiling...\n");
        system(COMPILE_CMD);    // Execute GCC command
        reload_handler();       // Reload new handler.so
        last_modified = file_stat.st_mtime;
    }
    else
    {
        perror("stat");
    }
}

__attribute__((noreturn)) static void sigint_handler(int signum)
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
    check_and_recompile();
    start_workers(server_fd);

    while(1)
    {
        pause();    // Parent process waits for signals
    }
}
