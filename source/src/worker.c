//
// Created by brandon-rada on 4/9/25.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>

#include "server.h"
#include "handler.h"

#define NUM_WORKERS 4

void start_workers(int server_fd) {
    for (int i = 0; i < NUM_WORKERS; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Worker process
            while (1) {
                int client_fd = accept(server_fd, NULL, NULL);
                if (client_fd < 0) {
                    perror("accept");
                    continue;
                }
                call_handler(client_fd);  // Use dynamic handler
                close(client_fd);
            }
            exit(0);  // never reached
        }
    }
}
