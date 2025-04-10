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
#include <sys/wait.h>
#include <unistd.h>

#define NUM_WORKERS 4

static pid_t worker_pids[NUM_WORKERS];    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
void         spawn_worker(int server_fd, int index);
static int   global_server_fd = -1;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
#define SOURCE_FILE "src/handler.c"
#define LIBRARY_FILE "lib/handler.so"
#define COMPILE_CMD "gcc -fPIC -shared -o " LIBRARY_FILE " " SOURCE_FILE
#define Perm 0755

// Global so the handler can access it
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
    struct stat   st = {0};

    if(stat("lib", &st) == -1 && mkdir("lib", Perm) == -1)
    {
        perror("mkdir lib");
        exit(EXIT_FAILURE);
    }

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

void spawn_worker(int server_fd, int index)
{
    pid_t pid = fork();
    if(pid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if(pid == 0)
    {
        while(1)
        {
            int client_fd = accept(server_fd, NULL, NULL);
            if(client_fd < 0)
            {
                perror("accept");
                continue;
            }

            call_handler(client_fd);    // use shared library
            close(client_fd);
        }
    }
    else
    {
        worker_pids[index] = pid;
    }
}

__attribute__((noreturn)) void start_workers(int server_fd)
{
    global_server_fd = server_fd;    // Save so monitor_workers can use it
    for(int i = 0; i < NUM_WORKERS; ++i)
    {
        spawn_worker(server_fd, i);
    }
    monitor_workers(NUM_WORKERS);

    while(1)
    {
        check_and_recompile();
        sleep(1);
    }
}

__attribute__((noreturn)) void monitor_workers(int num_workers)
{
    while(1)
    {
        int   status;
        pid_t pid = wait(&status);
        check_and_recompile();
        if(pid > 0)
        {
            for(int i = 0; i < num_workers; ++i)
            {
                if(worker_pids[i] == pid)
                {
                    fprintf(stderr, "Worker %d (PID %d) died, restarting...\n", i, pid);
                    spawn_worker(global_server_fd, i);    // assumes server_fd is preserved
                    break;
                }
            }
        }
    }
}
