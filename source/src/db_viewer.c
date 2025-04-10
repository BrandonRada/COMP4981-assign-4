//
// Created by brandon-rada on 4/9/25.
//
#include <fcntl.h>
#include <ndbm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_db_entries(const char *db_path);

void print_db_entries(const char *db_path)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
    DBM *db = dbm_open((char *)db_path, O_RDONLY, 0);
#pragma clang diagnostic pop
    datum key;

    if(!db)
    {
        perror("Failed to open DB");
        exit(EXIT_FAILURE);
    }

    printf("=== Contents of %s ===\n", db_path);

    key = dbm_firstkey(db);
    while(key.dptr != NULL)
    {
        datum value = dbm_fetch(db, key);
        if(value.dptr != NULL)
        {
            printf("Key: %.*s\n", key.dsize, key.dptr);
            printf("Value: %.*s\n", value.dsize, value.dptr);
            printf("----\n");
        }

        key = dbm_nextkey(db);
    }

    dbm_close(db);
}

int main(int argc, char *argv[])
{
    const char *db_path = "data/data_store";    // default

    if(argc == 2)
    {
        db_path = argv[1];
    }

    print_db_entries(db_path);

    return 0;
}
