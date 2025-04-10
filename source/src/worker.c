//
// Created by brandon-rada on 4/9/25.
//
#include "handler.h"
#include "server.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUM_WORKERS 4

static pid_t worker_pids[NUM_WORKERS];    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
void         spawn_worker(int server_fd, int index);

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

// void start_workers(int server_fd)
//{
//     for(int i = 0; i < NUM_WORKERS; ++i)
//     {
//         spawn_worker(server_fd, i);
//     }
//     monitor_workers(NUM_WORKERS);
// }

__attribute__((noreturn)) void start_workers(int server_fd)
{
    for(int i = 0; i < NUM_WORKERS; ++i)
    {
        spawn_worker(server_fd, i);
    }
    monitor_workers(NUM_WORKERS);

    while(1)
    {
        sleep(1);    // Prevent returning
    }
}

__attribute__((noreturn)) void monitor_workers(int num_workers)
{
    while(1)
    {
        int   status;
        pid_t pid = wait(&status);
        if(pid > 0)
        {
            for(int i = 0; i < num_workers; ++i)
            {
                if(worker_pids[i] == pid)
                {
                    fprintf(stderr, "Worker %d (PID %d) died, restarting...\n", i, pid);
                    spawn_worker(worker_pids[0], i);    // assumes server_fd is preserved
                    break;
                }
            }
        }
    }
}
