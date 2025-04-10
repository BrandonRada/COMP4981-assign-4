//
// Created by brandon-rada on 4/9/25.
//
#include <ctype.h>
#include <fcntl.h>
#include <ndbm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define WEBROOT "web"
#define MAX_REQUEST 4096
#define BUFFER_SIZE 1026
#define METHOD_LENGTH 8
#define PATH_LENGTH 256
#define FULL_PATH_LENGTH 512
#define DATABASE_PERMISSION 0666

void handle_post(int client_fd, const char *body);
void serve_file(int client_fd, const char *path, int head_only);
int  handle_request(int client_fd);

// Simple key=value POST parsing (not robust, just demo)
void handle_post(int client_fd, const char *body)
{
    char       *saveptr;
    char       *key;
    char       *value;
    DBM        *db;
    const char *db_filename;

    // Copy body into mutable buffer (strtok_r modifies input)
    char body_copy[MAX_REQUEST];

    db_filename = "data_store";

    strncpy(body_copy, body, sizeof(body_copy) - 1);
    body_copy[sizeof(body_copy) - 1] = '\0';

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
    db = dbm_open((char *)db_filename, O_RDWR | O_CREAT, DATABASE_PERMISSION);
#pragma clang diagnostic pop

    if(!db)
    {
        dprintf(client_fd, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
        return;
    }

    key   = strtok_r(body_copy, "=", &saveptr);
    value = strtok_r(NULL, "=", &saveptr);
    if(key && value)
    {
        datum dkey = {key, (int)strlen(key)};
        datum dval = {value, (int)strlen(value)};
        dbm_store(db, dkey, dval, DBM_REPLACE);
        dprintf(client_fd, "HTTP/1.1 200 OK\r\n\r\nSaved: %s=%s\n", key, value);
    }
    else
    {
        dprintf(client_fd, "HTTP/1.1 400 Bad Request\r\n\r\n");
    }

    dbm_close(db);
}

void serve_file(int client_fd, const char *path, int head_only)
{
    char        fullpath[FULL_PATH_LENGTH];
    struct stat st;
    int         fd;
    snprintf(fullpath, sizeof(fullpath), "%s/%s", WEBROOT, path);

    fd = open(fullpath, O_RDONLY | O_CLOEXEC);
    if(fd < 0)
    {
        dprintf(client_fd, "HTTP/1.1 404 Not Found\r\n\r\n");
        return;
    }

    fstat(fd, &st);

    dprintf(client_fd,
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: %ld\r\n"
            "Content-Type: text/html\r\n"
            "\r\n",
            st.st_size);

    if(!head_only)
    {
        char    buf[BUFFER_SIZE];
        ssize_t bytes;
        while((bytes = read(fd, buf, sizeof(buf))) > 0)
        {
            write(client_fd, buf, (size_t)bytes);    // Fix: cast to size_t after checking bytes > 0
        }
    }

    close(fd);
}

int handle_request(int client_fd)
{
    char        buffer[MAX_REQUEST];
    char        method[METHOD_LENGTH];
    char        path[PATH_LENGTH];
    const char *trimmed_path;

    ssize_t len = read(client_fd, buffer, sizeof(buffer) - 1);
    if(len <= 0)
    {
        return -1;
    }

    // Null-terminate the buffer safely
    buffer[len] = '\0';

    // Parse method and path
    sscanf(buffer, "%7s %255s", method, path);

    // Strip leading /
    trimmed_path = path[0] == '/' ? path + 1 : path;

    if(strcasecmp(method, "GET") == 0)
    {
        serve_file(client_fd, trimmed_path, 0);
    }
    else if(strcasecmp(method, "HEAD") == 0)
    {
        serve_file(client_fd, trimmed_path, 1);
    }
    else if(strcasecmp(method, "POST") == 0)
    {
        // Find POST body
        char *body = strstr(buffer, "\r\n\r\n");
        if(body)
        {
            body += 4;    // skip the "\r\n\r\n"
            handle_post(client_fd, body);
        }
        else
        {
            dprintf(client_fd, "HTTP/1.1 400 Bad Request\r\n\r\n");
        }
    }
    else
    {
        dprintf(client_fd, "HTTP/1.1 405 Method Not Allowed\r\n\r\n");
    }

    return 0;
}
