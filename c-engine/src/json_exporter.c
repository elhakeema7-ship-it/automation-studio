#include "../include/json_exporter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

JsonBuffer* json_buffer_create() {
    JsonBuffer *buf = (JsonBuffer *)malloc(sizeof(JsonBuffer));
    buf->capacity = 4096;
    buf->size = 0;
    buf->data = (char *)malloc(buf->capacity);
    buf->data[0] = '\0';
    return buf;
}

void json_buffer_append(JsonBuffer *buf, const char *str) {
    if (str == NULL) return;

    size_t str_len = strlen(str);
    while (buf->size + str_len >= buf->capacity) {
        buf->capacity *= 2;
        buf->data = (char *)realloc(buf->data, buf->capacity);
    }

    strcpy(buf->data + buf->size, str);
    buf->size += str_len;
}

char* json_buffer_get(JsonBuffer *buf) {
    return buf->data;
}

void json_buffer_free(JsonBuffer *buf) {
    if (buf != NULL) {
        free(buf->data);
        free(buf);
    }
}

char* escape_json_string(const char *str) {
    if (str == NULL) return (char *)malloc(1);

    size_t len = strlen(str);
    char *escaped = (char *)malloc(len * 2 + 1);
    int j = 0;

    for (int i = 0; i < len; i++) {
        switch (str[i]) {
            case '\"':
                escaped[j++] = '\\';
                escaped[j++] = '\"';
                break;
            case '\\':
                escaped[j++] = '\\';
                escaped[j++] = '\\';
                break;
            case '\n':
                escaped[j++] = '\\';
                escaped[j++] = 'n';
                break;
            case '\r':
                escaped[j++] = '\\';
                escaped[j++] = 'r';
                break;
            case '\t':
                escaped[j++] = '\\';
                escaped[j++] = 't';
                break;
            default:
                escaped[j++] = str[i];
        }
    }
    escaped[j] = '\0';
    return escaped;
}

int export_database_to_json(Database *db, const char *output_path) {
    if (db == NULL || db->db == NULL) {
        return -1;
    }

    FILE *output = fopen(output_path, "w");
    if (output == NULL) {
        fprintf(stderr, "Error: Cannot create output file %s\n", output_path);
        return -1;
    }

    fprintf(output, "{\n");
    fprintf(output, "  \"metadata\": {\n");
    fprintf(output, "    \"total_files\": %d,\n", database_get_file_count(db));
    fprintf(output, "    \"export_timestamp\": \"%s\",\n", "2024-generated");
    fprintf(output, "    \"database_path\": \"%s\"\n", db->db_path);
    fprintf(output, "  },\n");
    fprintf(output, "  \"files\": [\n");

    const char *sql = "SELECT id, filepath, filename, extension, file_size, content_hash FROM files ORDER BY id;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error preparing statement: %s\n", sqlite3_errmsg(db->db));
        fclose(output);
        return -1;
    }

    int first = 1;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) {
            fprintf(output, ",\n");
        }
        first = 0;

        int id = sqlite3_column_int(stmt, 0);
        const char *filepath = (const char *)sqlite3_column_text(stmt, 1);
        const char *filename = (const char *)sqlite3_column_text(stmt, 2);
        const char *extension = (const char *)sqlite3_column_text(stmt, 3);
        int file_size = sqlite3_column_int(stmt, 4);
        const char *content_hash = (const char *)sqlite3_column_text(stmt, 5);

        char *esc_filepath = escape_json_string(filepath);
        char *esc_filename = escape_json_string(filename);
        char *esc_extension = escape_json_string(extension);
        char *esc_hash = escape_json_string(content_hash);

        fprintf(output, "    {\n");
        fprintf(output, "      \"id\": %d,\n", id);
        fprintf(output, "      \"filepath\": \"%s\",\n", esc_filepath);
        fprintf(output, "      \"filename\": \"%s\",\n", esc_filename);
        fprintf(output, "      \"extension\": \"%s\",\n", esc_extension);
        fprintf(output, "      \"file_size\": %d,\n", file_size);
        fprintf(output, "      \"content_hash\": \"%s\"\n", esc_hash);
        fprintf(output, "    }");

        free(esc_filepath);
        free(esc_filename);
        free(esc_extension);
        free(esc_hash);
    }

    sqlite3_finalize(stmt);
    fprintf(output, "\n  ]\n");
    fprintf(output, "}\n");
    fclose(output);

    printf("✓ JSON export completed: %s\n", output_path);
    return 0;
}

char* generate_json_summary(Database *db) {
    if (db == NULL || db->db == NULL) {
        return NULL;
    }

    JsonBuffer *buf = json_buffer_create();

    json_buffer_append(buf, "{\n");
    json_buffer_append(buf, "  \"database_info\": {\n");
    
    int count = database_get_file_count(db);
    char count_str[64];
    snprintf(count_str, sizeof(count_str), "    \"total_files\": %d,\n", count);
    json_buffer_append(buf, count_str);
    
    json_buffer_append(buf, "    \"database_path\": \"");
    json_buffer_append(buf, db->db_path);
    json_buffer_append(buf, "\"\n");
    json_buffer_append(buf, "  }\n");
    json_buffer_append(buf, "}\n");

    char *result = (char *)malloc(strlen(json_buffer_get(buf)) + 1);
    strcpy(result, json_buffer_get(buf));
    json_buffer_free(buf);

    return result;
}
