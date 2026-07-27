// database.h
#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include "file_scanner.h"

typedef struct {
    sqlite3 *db;
    char db_path[1024];
} Database;

Database* database_open(const char *db_path);
void database_close(Database *db);
int database_init_tables(Database *db);
int database_store_file(Database *db, const FileMetadata *file, const char *content);
int database_clear(Database *db);
int database_get_file_count(Database *db);

#endif
