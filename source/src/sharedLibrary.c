//
// Created by brandon-rada on 4/6/25.
//
#include "sharedLibrary.h"

// sharedLibrary.c

// Updated version
void handle_request(client_info *info)
{
    char buffer[4096];
    int  bytes_received = recv(info->socket_fd, buffer, sizeof(buffer) - 1, 0);
    if(bytes_received <= 0)
    {
        perror("recv");
        return;
    }

    buffer[bytes_received] = '\0';

    char method[8], path[256];
    if(sscanf(buffer, "%7s %255s", method, path) != 2)
    {
        fprintf(stderr, "Invalid HTTP request: %s\n", buffer);
        return;
    }

    if(strcmp(method, "GET") == 0 || strcmp(method, "HEAD") == 0)
    {
        const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nHello, World!";
        send(info->socket_fd, response, strlen(response), 0);
    }
    else if(strcmp(method, "POST") == 0)
    {
        DBM *db = dbm_open("data.ndbm", O_RDWR | O_CREAT, 0666);
        if(!db)
        {
            perror("dbm_open");
            return;
        }

        char  data[] = "Sample POST data";
        datum key    = {.dptr = "key1", .dsize = strlen("key1")};
        datum value  = {.dptr = data, .dsize = strlen(data)};

        if(dbm_store(db, key, value, DBM_REPLACE) < 0)
        {
            perror("dbm_store");
            dbm_close(db);
            return;
        }

        dbm_close(db);

        const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nPOST data received.";
        send(info->socket_fd, response, strlen(response), 0);
    }
    else
    {
        const char *response = "HTTP/1.1 405 Method Not Allowed\r\nContent-Type: text/plain\r\n\r\nMethod Not Allowed";
        send(info->socket_fd, response, strlen(response), 0);
    }
}
