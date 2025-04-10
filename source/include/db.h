//
// Created by brandon-rada on 4/9/25.
//

#ifndef DB_H
#define DB_H

#include "shared_lib.h"

int handle_post_request(int client_fd, const HTTPRequest *request, const char *body);

#endif //DB_H
