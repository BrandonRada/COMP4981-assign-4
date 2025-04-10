//
// Created by brandon-rada on 4/9/25.
//
#include <ndbm.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "../include/db.h"

#define Perm2 0666

int handle_post_request(int client_fd, const HTTPRequest *req, const char *body)
{
        DBM *db = dbm_open("../data/data_store", O_RDWR | O_CREAT, Perm2);
    if (!db) {
        dprintf(client_fd, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
        return -1;
    }

    char key[128], value[128];
    if (sscanf(body, "%127[^=]=%127s", key, value) == 2) {
        datum dkey = {key, (int)strlen(key)};
        datum dval = {value, (int)strlen(value)};
        dbm_store(db, dkey, dval, DBM_REPLACE);
        dprintf(client_fd, "HTTP/1.1 200 OK\r\n\r\nSaved: %s=%s\n", key, value);
    } else {
        dprintf(client_fd, "HTTP/1.1 400 Bad Request\r\n\r\n");
    }

    dbm_close(db);
    return 0;
}
