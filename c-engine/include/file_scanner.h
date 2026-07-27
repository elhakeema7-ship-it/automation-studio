// file_scanner.h
#ifndef FILE_SCANNER_H
#define FILE_SCANNER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_PATH_LEN 4096
#define MAX_FILE_SIZE 10485760  // 10MB
#define MAX_FILES 10000

typedef struct {
    char path[MAX_PATH_LEN];
    char filename[256];
    char extension[32];
    long size;
    char content_hash[65];  // SHA256
} FileMetadata;

typedef struct {
    FileMetadata *files;
    int count;
    int capacity;
} FileCollection;

FileCollection* file_collection_create(int initial_capacity);
void file_collection_add(FileCollection *collection, const char *filepath);
void file_collection_free(FileCollection *collection);
void scan_directory(FileCollection *collection, const char *dirpath, int recursive);
void calculate_file_hash(const char *filepath, char *hash_output);

#endif
