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
    load_handler_if_updated();
    if(handler_func)
    {
        return handler_func(client_fd);
    }
    return -1;
}
