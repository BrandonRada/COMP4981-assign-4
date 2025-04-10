//
// Created by brandon-rada on 4/9/25.
//
#include "handler.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static void  *lib_handle        = NULL;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
static time_t last_loaded_time  = 0;       // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
static int (*handler_func)(int) = NULL;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

#define SHARED_LIB_PATH "./lib/handler.so"
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

static time_t get_mod_time(const char *path)
{
    struct stat st;
    if(stat(path, &st) == 0)
    {
        return st.st_mtime;
    }
    return 0;
}

void load_handler_if_updated(void)
{
    time_t mod_time = get_mod_time(SHARED_LIB_PATH);
    check_and_recompile();
    if(mod_time == 0 || mod_time <= last_loaded_time)
    {
        return;    // No update
    }

    if(lib_handle != NULL)
    {
        dlclose(lib_handle);
    }

    lib_handle = dlopen(SHARED_LIB_PATH, RTLD_LAZY);
    if(!lib_handle)
    {
        fprintf(stderr, "Failed to load handler.so: %s\n", dlerror());
        exit(1);
    }

    *(void **)(&handler_func) = dlsym(lib_handle, "handle_request");
    if(!handler_func)
    {
        fprintf(stderr, "Symbol not found: %s\n", dlerror());
        exit(1);
    }

    last_loaded_time = mod_time;
    printf("Loaded new handler.so at %ld\n", last_loaded_time);
}

int call_handler(int client_fd)
{
    check_and_recompile();
    load_handler_if_updated();
    if(handler_func)
    {
        return handler_func(client_fd);
    }
    return -1;
}
