#include "../include/database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Database* database_open(const char *db_path) {
    Database *db = (Database *)malloc(sizeof(Database));
    strcpy(db->db_path, db_path);

    int rc = sqlite3_open(db_path, &db->db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error: Cannot open database %s: %s\n", db_path, sqlite3_errmsg(db->db));
        free(db);
        return NULL;
    }

    return db;
}

void database_close(Database *db) {
    if (db != NULL) {
        sqlite3_close(db->db);
        free(db);
    }
}

int database_init_tables(Database *db) {
    if (db == NULL || db->db == NULL) {
        return -1;
    }

    const char *sql = 
        "CREATE TABLE IF NOT EXISTS files (\n"
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
        "  filepath TEXT NOT NULL UNIQUE,\n"
        "  filename TEXT NOT NULL,\n"
        "  extension TEXT,\n"
        "  file_size INTEGER,\n"
        "  content_hash TEXT UNIQUE,\n"
        "  content LONGBLOB,\n"
        "  language TEXT,\n"
        "  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,\n"
        "  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP\n"
        ");\n"
        "\n"
        "CREATE TABLE IF NOT EXISTS file_metadata (\n"
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
        "  file_id INTEGER NOT NULL,\n"
        "  key TEXT NOT NULL,\n"
        "  value TEXT,\n"
        "  FOREIGN KEY(file_id) REFERENCES files(id),\n"
        "  UNIQUE(file_id, key)\n"
        ");\n"
        "\n"
        "CREATE INDEX IF NOT EXISTS idx_extension ON files(extension);\n"
        "CREATE INDEX IF NOT EXISTS idx_filepath ON files(filepath);\n"
        "CREATE INDEX IF NOT EXISTS idx_hash ON files(content_hash);\n";

    char *err_msg = NULL;
    int rc = sqlite3_exec(db->db, sql, 0, 0, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error creating tables: %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }

    return 0;
}

int database_store_file(Database *db, const FileMetadata *file, const char *content) {
    if (db == NULL || db->db == NULL || file == NULL) {
        return -1;
    }

    const char *sql = 
        "INSERT INTO files (filepath, filename, extension, file_size, content_hash, content, language) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)\n"
        "ON CONFLICT(filepath) DO UPDATE SET "
        "  content = excluded.content, "
        "  content_hash = excluded.content_hash, "
        "  updated_at = CURRENT_TIMESTAMP;";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error preparing statement: %s\n", sqlite3_errmsg(db->db));
        return -1;
    }

    // Bind parameters
    sqlite3_bind_text(stmt, 1, file->path, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, file->filename, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, file->extension, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, file->size);
    sqlite3_bind_text(stmt, 5, file->content_hash, -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 6, content, strlen(content), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, file->extension, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Error inserting file: %s\n", sqlite3_errmsg(db->db));
        return -1;
    }

    return 0;
}

int database_clear(Database *db) {
    if (db == NULL || db->db == NULL) {
        return -1;
    }

    char *err_msg = NULL;
    int rc = sqlite3_exec(db->db, "DELETE FROM file_metadata; DELETE FROM files;", 0, 0, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error clearing database: %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }

    return 0;
}

int database_get_file_count(Database *db) {
    if (db == NULL || db->db == NULL) {
        return -1;
    }

    const char *sql = "SELECT COUNT(*) FROM files;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error preparing statement: %s\n", sqlite3_errmsg(db->db));
        return -1;
    }

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}
