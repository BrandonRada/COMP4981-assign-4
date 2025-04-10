//
// Created by brandon-rada on 4/9/25.
//
// handler.c
#include <ctype.h>
#include <fcntl.h>
#include <ndbm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#define WEBROOT "web"
#define MAX_REQUEST 4096
#define BUFFER_SIZE 1024
#define METHOD_LENGTH 8
#define PATH_LENGTH 256
#define FULL_PATH_LENGTH 512
#define DATABASE_PERMISSION 0666

static void handle_post(int client_fd, const char *body)
{
    DBM *db = dbm_open("data/data_store", O_RDWR | O_CREAT, DATABASE_PERMISSION);
    if(!db)
    {
        dprintf(client_fd, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
        return;
    }

    char key[128], value[128];
    if(sscanf(body, "%127[^=]=%127s", key, value) == 2)
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

static void serve_file(int client_fd, const char *path, int head_only)
{
    char fullpath[FULL_PATH_LENGTH];
    char resolved_path[PATH_MAX];

    snprintf(fullpath, sizeof(fullpath), "%s/%s", WEBROOT, path);

    if (strstr(path, "..") != NULL)
    {
        dprintf(client_fd, "HTTP/1.1 403 Forbidden\r\n\r\n");
        return;
    }


    // Resolve the absolute path
    if (realpath(fullpath, resolved_path) == NULL) {
        perror("realpath error");
        dprintf(client_fd, "HTTP/1.1 404 Not Found\r\n\r\n");
        return;
    }
    printf("Resolved Path: %s\n", resolved_path);

    int fd = open(resolved_path, O_RDONLY);
    if(fd < 0)
    {
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

    if(!head_only)
    {
        char buf[BUFFER_SIZE];
        ssize_t bytes;
        while((bytes = read(fd, buf, sizeof(buf))) > 0)
        {
            write(client_fd, buf, (size_t)bytes);
        }
    }

    close(fd);
}

// This is the function that must be exported in the shared library
int handle_request(int client_fd)
{
    char buffer[MAX_REQUEST];
    char method[METHOD_LENGTH];
    char path[PATH_LENGTH];
    const char *trimmed_path;

    ssize_t len = read(client_fd, buffer, sizeof(buffer) - 1);
    if(len <= 0)
        return -1;

    buffer[len] = '\0';
    sscanf(buffer, "%7s %255s", method, path);

    trimmed_path = (path[0] == '/') ? path + 1 : path;

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
        char *body = strstr(buffer, "\r\n\r\n");
        if(body)
        {
            body += 4;
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
