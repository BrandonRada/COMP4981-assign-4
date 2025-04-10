//
// Created by brandon-rada on 4/9/25.
//

#ifndef SHARED_LIB_H
#define SHARED_LIB_H
typedef struct {
    const char *method;
    const char *path;
    const char *body;
} HTTPRequest;

int handle_request(int client_fd, const HTTPRequest *request);
#endif //SHARED_LIB_H
