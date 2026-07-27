#include "../include/file_scanner.h"
#include "../include/database.h"
#include "../include/json_exporter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void print_usage(const char *program) {
    printf("Usage: %s [OPTIONS]\n", program);
    printf("\nOptions:\n");
    printf("  -h, --help              Show this help message\n");
    printf("  -s, --scan <directory>  Scan directory and store files\n");
    printf("  -d, --database <path>   Database file path (default: files.db)\n");
    printf("  -o, --output <path>     Output JSON path (default: database_export.json)\n");
    printf("  -e, --export            Export database to JSON\n");
    printf("  -c, --clear             Clear database\n");
    printf("  -r, --recursive         Recursive directory scan\n");
    printf("\nExample:\n");
    printf("  %s -s /path/to/project -d project.db -o export.json -e -r\n", program);
}

char* read_file_content(const char *filepath) {
    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        return (char *)malloc(1);
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size > 5242880) {  // 5MB limit for content storage
        fclose(file);
        char *msg = (char *)malloc(100);
        sprintf(msg, "[FILE TOO LARGE - %ld bytes]", size);
        return msg;
    }

    char *content = (char *)malloc(size + 1);
    fread(content, 1, size, file);
    content[size] = '\0';
    fclose(file);

    return content;
}

int main(int argc, char *argv[]) {
    printf("\n╔════════════════════════════════════════════════╗\n");
    printf("║     Automation Studio - C Engine v1.0.0        ║\n");
    printf("║     File Scanner & Database Manager             ║\n");
    printf("╚════════════════════════════════════════════════╝\n\n");

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *db_path = "files.db";
    const char *output_path = "database_export.json";
    const char *scan_dir = NULL;
    int should_export = 0;
    int should_clear = 0;
    int recursive = 0;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--database") == 0) {
            if (i + 1 < argc) db_path = argv[++i];
        }
        else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) output_path = argv[++i];
        }
        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--scan") == 0) {
            if (i + 1 < argc) scan_dir = argv[++i];
        }
        else if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--export") == 0) {
            should_export = 1;
        }
        else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--clear") == 0) {
            should_clear = 1;
        }
        else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--recursive") == 0) {
            recursive = 1;
        }
    }

    // Initialize database
    printf("[*] Opening database: %s\n", db_path);
    Database *db = database_open(db_path);
    if (db == NULL) {
        fprintf(stderr, "[!] Failed to open database\n");
        return 1;
    }

    // Initialize tables
    printf("[*] Initializing database tables...\n");
    if (database_init_tables(db) != 0) {
        fprintf(stderr, "[!] Failed to initialize tables\n");
        database_close(db);
        return 1;
    }

    // Clear database if requested
    if (should_clear) {
        printf("[*] Clearing database...\n");
        database_clear(db);
        printf("[✓] Database cleared\n");
    }

    // Scan directory if specified
    if (scan_dir != NULL) {
        printf("[*] Starting file scan: %s\n", scan_dir);
        printf("[*] Recursive: %s\n", recursive ? "Yes" : "No");

        FileCollection *collection = file_collection_create(100);
        scan_directory(collection, scan_dir, recursive);

        printf("[*] Found %d files\n", collection->count);
        printf("[*] Storing files in database...\n");

        int stored_count = 0;
        for (int i = 0; i < collection->count; i++) {
            printf("\r  Processing: %d/%d", i + 1, collection->count);
            fflush(stdout);

            char *content = read_file_content(collection->files[i].path);
            if (database_store_file(db, &collection->files[i], content) == 0) {
                stored_count++;
            }
            free(content);
        }
        printf("\n[✓] Stored %d/%d files\n", stored_count, collection->count);

        file_collection_free(collection);
    }

    // Export to JSON if requested
    if (should_export) {
        printf("[*] Exporting database to JSON: %s\n", output_path);
        if (export_database_to_json(db, output_path) == 0) {
            printf("[✓] Export completed successfully\n");
        } else {
            fprintf(stderr, "[!] Export failed\n");
        }
    }

    // Print summary
    printf("\n[*] Database Summary:\n");
    printf("    Total files: %d\n", database_get_file_count(db));
    printf("    Database: %s\n", db->db_path);
    if (should_export) {
        printf("    Export: %s\n", output_path);
    }

    printf("\n");
    database_close(db);

    printf("[✓] Done!\n\n");
    return 0;
}
