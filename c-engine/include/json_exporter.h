// json_exporter.h
#ifndef JSON_EXPORTER_H
#define JSON_EXPORTER_H

#include "database.h"

typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} JsonBuffer;

JsonBuffer* json_buffer_create();
void json_buffer_append(JsonBuffer *buf, const char *str);
char* json_buffer_get(JsonBuffer *buf);
void json_buffer_free(JsonBuffer *buf);

int export_database_to_json(Database *db, const char *output_path);
char* generate_json_summary(Database *db);

#endif
