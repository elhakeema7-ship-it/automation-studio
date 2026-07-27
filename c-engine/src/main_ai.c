#include "../include/file_scanner.h"
#include "../include/database.h"
#include "../include/json_exporter.h"
#include "../include/semantic_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════════╗\n");
    printf("║   Automation Studio - AI-Powered C Engine v2.0                     ║\n");
    printf("║   File Scanner + Semantic Search + Vector Embeddings              ║\n");
    printf("╚════════════════════════════════════════════════════════════════════╝\n\n");

    // Configuration
    const char *db_path = "semantic.db";
    const char *minilm_path = "D:\\models\\encode_decoder\\all-MiniLM-L6-v2\\model.onnx";
    const char *llama_path = "D:\\models\\meta-llama-onnx\\model.onnx";
    const char *export_path = "semantic_export.json";

    // Initialize database
    printf("[*] Initializing database...\n");
    Database *db = database_open(db_path);
    if (db == NULL) {
        fprintf(stderr, "[!] Failed to open database\n");
        return 1;
    }

    database_init_tables(db);
    printf("[✓] Database ready\n");

    // Scan directory if provided
    if (argc > 1 && strcmp(argv[1], "-s") == 0 && argc > 2) {
        printf("\n[*] Scanning directory: %s\n", argv[2]);
        FileCollection *collection = file_collection_create(100);
        scan_directory(collection, argv[2], 1);

        printf("[*] Found %d files\n", collection->count);
        printf("[*] Storing in database...\n");

        for (int i = 0; i < collection->count; i++) {
            printf("\r  [%d/%d]", i + 1, collection->count);
            fflush(stdout);
            
            char *content = "";
            database_store_file(db, &collection->files[i], content);
        }
        printf("\n[✓] Files stored\n");
        file_collection_free(collection);
    }

    // Initialize semantic engine
    printf("\n[*] Loading AI models...\n");
    printf("    - all-MiniLM-L6-v2 (384 dims)\n");
    printf("    - meta-llama-onnx (4096 dims)\n");
    
    SemanticEngine *engine = semantic_engine_init(db, minilm_path, llama_path);
    if (engine == NULL) {
        fprintf(stderr, "[!] Failed to initialize semantic engine\n");
        database_close(db);
        return 1;
    }

    // Index all files
    semantic_engine_index(engine);

    // Interactive search
    printf("\n╔════════════════════════════════════════════════════════════════════╗\n");
    printf("║                    SEMANTIC SEARCH MODE                           ║\n");
    printf("║              Type 'exit' to quit, 'help' for options              ║\n");
    printf("╚════════════════════════════════════════════════════════════════════╝\n\n");

    char query[1024];
    while (1) {
        printf("\n> ");
        if (fgets(query, sizeof(query), stdin) == NULL) break;

        // Remove newline
        size_t len = strlen(query);
        if (len > 0 && query[len - 1] == '\n') {
            query[len - 1] = '\0';
        }

        if (strcmp(query, "exit") == 0) {
            printf("[✓] Goodbye!\n");
            break;
        }

        if (strcmp(query, "help") == 0) {
            printf("\nCommands:\n");
            printf("  search <query>    - Search for files\n");
            printf("  index             - Re-index all files\n");
            printf("  export            - Export to JSON\n");
            printf("  info              - Show database info\n");
            printf("  exit              - Quit\n");
            continue;
        }

        if (strncmp(query, "search ", 7) == 0) {
            SearchResults *results = semantic_engine_search(engine, query + 7, 10);
            if (results != NULL) {
                semantic_engine_print_results(results);
                search_results_free(results);
            }
            continue;
        }

        if (strcmp(query, "export") == 0) {
            printf("[*] Exporting to %s...\n", export_path);
            export_database_to_json(db, export_path);
            continue;
        }

        if (strcmp(query, "info") == 0) {
            printf("\nDatabase Info:\n");
            printf("  Files: %d\n", database_get_file_count(db));
            printf("  Path: %s\n", db->db_path);
            continue;
        }

        // Direct search
        SearchResults *results = semantic_engine_search(engine, query, 10);
        if (results != NULL) {
            semantic_engine_print_results(results);
            search_results_free(results);
        }
    }

    // Cleanup
    semantic_engine_free(engine);
    database_close(db);

    printf("\n[✓] Done!\n\n");
    return 0;
}
