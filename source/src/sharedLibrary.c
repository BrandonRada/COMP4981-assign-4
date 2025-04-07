//
// Created by brandon-rada on 4/6/25.
//
#include "sharedLibrary.h"

#define EIGHT 8
#define MAX_PATH 256

// sharedLibrary.c

// Updated version
void handle_request(client_info *info)
{
    char buffer[4096];

    char    method[EIGHT];
    char    path[MAX_PATH];
    ssize_t bytes_received = recv(info->socket_fd, buffer, sizeof(buffer) - 1, 0);
    if(bytes_received <= 0)
    {
        perror("recv");
        return;
    }

    buffer[bytes_received] = '\0';

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
        char        db_file[]  = "data.ndbm";
        char        data[]     = "Sample POST data";
        char        key_data[] = "key1";
        const char *response   = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nPOST data received.";
        datum       key        = {.dptr = key_data, .dsize = (int)strlen(key_data)};
        datum       value      = {.dptr = data, .dsize = (int)strlen(data)};
        DBM        *db         = dbm_open(db_file, O_RDWR | O_CREAT, 0666);
        if(!db)
        {
            perror("dbm_open");
            return;
        }

        if(dbm_store(db, key, value, DBM_REPLACE) < 0)
        {
            perror("dbm_store");
            dbm_close(db);
            return;
        }

        dbm_close(db);

        send(info->socket_fd, response, strlen(response), 0);
    }
    else
    {
        const char *response = "HTTP/1.1 405 Method Not Allowed\r\nContent-Type: text/plain\r\n\r\nMethod Not Allowed";
        send(info->socket_fd, response, strlen(response), 0);
    }
}
