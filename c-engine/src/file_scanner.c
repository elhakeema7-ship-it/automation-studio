#include "../include/file_scanner.h"
#include <openssl/sha.h>

FileCollection* file_collection_create(int initial_capacity) {
    FileCollection *collection = (FileCollection *)malloc(sizeof(FileCollection));
    collection->capacity = initial_capacity;
    collection->count = 0;
    collection->files = (FileMetadata *)malloc(sizeof(FileMetadata) * initial_capacity);
    return collection;
}

void file_collection_add(FileCollection *collection, const char *filepath) {
    if (collection->count >= collection->capacity) {
        collection->capacity *= 2;
        collection->files = (FileMetadata *)realloc(collection->files, sizeof(FileMetadata) * collection->capacity);
    }

    FileMetadata *file = &collection->files[collection->count];
    strcpy(file->path, filepath);

    // Extract filename and extension
    const char *filename_start = strrchr(filepath, '/');
    if (filename_start == NULL) {
        filename_start = filepath;
    } else {
        filename_start++;
    }
    strcpy(file->filename, filename_start);

    // Extract extension
    const char *ext_start = strrchr(filename_start, '.');
    if (ext_start != NULL) {
        strcpy(file->extension, ext_start + 1);
    } else {
        strcpy(file->extension, "no_extension");
    }

    // Get file size
    struct stat st;
    if (stat(filepath, &st) == 0) {
        file->size = st.st_size;
    } else {
        file->size = 0;
    }

    // Calculate hash
    calculate_file_hash(filepath, file->content_hash);

    collection->count++;
}

void calculate_file_hash(const char *filepath, char *hash_output) {
    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        strcpy(hash_output, "error_reading_file");
        return;
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    unsigned char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), file)) != 0) {
        SHA256_Update(&sha256, buffer, bytes);
    }
    SHA256_Final(hash, &sha256);
    fclose(file);

    // Convert hash to hex string
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hash_output + (i * 2), "%02x", hash[i]);
    }
}

void scan_directory(FileCollection *collection, const char *dirpath, int recursive) {
    DIR *dir = opendir(dirpath);
    if (dir == NULL) {
        fprintf(stderr, "Error: Cannot open directory %s\n", dirpath);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s/%s", dirpath, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISREG(st.st_mode)) {
                // Regular file
                if (st.st_size <= MAX_FILE_SIZE) {
                    file_collection_add(collection, full_path);
                }
            } else if (S_ISDIR(st.st_mode) && recursive) {
                // Directory - recurse if requested
                scan_directory(collection, full_path, recursive);
            }
        }
    }
    closedir(dir);
}

void file_collection_free(FileCollection *collection) {
    if (collection != NULL) {
        free(collection->files);
        free(collection);
    }
}
