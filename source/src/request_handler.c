//
// Created by brandon-rada on 4/9/25.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <ndbm.h>

#define WEBROOT "web"
#define MAX_REQUEST 4096

// Simple key=value POST parsing (not robust, just demo)
void handle_post(int client_fd, const char *body) {
    DBM *db = dbm_open("data_store", O_RDWR | O_CREAT, 0666);
    if (!db) {
        dprintf(client_fd, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
        return;
    }

    char *key = strtok((char *)body, "=");
    char *value = strtok(NULL, "&");
    if (key && value) {
        datum dkey = { key, strlen(key) };
        datum dval = { value, strlen(value) };
        dbm_store(db, dkey, dval, DBM_REPLACE);
        dprintf(client_fd, "HTTP/1.1 200 OK\r\n\r\nSaved: %s=%s\n", key, value);
    } else {
        dprintf(client_fd, "HTTP/1.1 400 Bad Request\r\n\r\n");
    }

    dbm_close(db);
}

void serve_file(int client_fd, const char *path, int head_only) {
    char fullpath[512];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", WEBROOT, path);

    int fd = open(fullpath, O_RDONLY);
    if (fd < 0) {
        dprintf(client_fd, "HTTP/1.1 404 Not Found\r\n\r\n");
        return;
    }

    struct stat st;
    fstat(fd, &st);

    dprintf(client_fd,
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: %ld\r\n"
            "Content-Type: text/html\r\n"
            "\r\n",
            st.st_size);

    if (!head_only) {
        char buf[1024];
        ssize_t bytes;
        while ((bytes = read(fd, buf, sizeof(buf))) > 0) {
            write(client_fd, buf, bytes);
        }
    }

    close(fd);
}

int handle_request(int client_fd) {
    char buffer[MAX_REQUEST] = {0};
    ssize_t len = read(client_fd, buffer, sizeof(buffer) - 1);
    if (len <= 0) return -1;

    // Parse method and path
    char method[8], path[256];
    sscanf(buffer, "%s %s", method, path);

    // Strip leading /
    char *trimmed_path = path[0] == '/' ? path + 1 : path;

    if (strcasecmp(method, "GET") == 0) {
        serve_file(client_fd, trimmed_path, 0);
    } else if (strcasecmp(method, "HEAD") == 0) {
        serve_file(client_fd, trimmed_path, 1);
    } else if (strcasecmp(method, "POST") == 0) {
        // Find POST body
        char *body = strstr(buffer, "\r\n\r\n");
        if (body) {
            body += 4; // skip the "\r\n\r\n"
            handle_post(client_fd, body);
        } else {
            dprintf(client_fd, "HTTP/1.1 400 Bad Request\r\n\r\n");
        }
    } else {
        dprintf(client_fd, "HTTP/1.1 405 Method Not Allowed\r\n\r\n");
    }

    return 0;
}

