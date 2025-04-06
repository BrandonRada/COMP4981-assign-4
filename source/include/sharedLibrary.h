//
// Created by brandon-rada on 4/6/25.
//

#ifndef SHAREDLIBRARY_H
#define SHAREDLIBRARY_H
#include "../include/handleClient.h"
#include <fcntl.h>
#include <ndbm.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
void handle_request(client_info *info);

#endif    // SHAREDLIBRARY_H
