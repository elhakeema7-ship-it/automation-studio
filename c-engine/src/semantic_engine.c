#include "../include/semantic_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SemanticEngine* semantic_engine_init(Database *db, const char *minilm_path, const char *llama_path) {
    SemanticEngine *engine = (SemanticEngine *)malloc(sizeof(SemanticEngine));
    engine->db = db;
    engine->search_engine = vector_search_init(db, minilm_path, llama_path);

    if (engine->search_engine == NULL) {
        fprintf(stderr, "[!] Failed to initialize search engine\n");
        free(engine);
        return NULL;
    }

    printf("[✓] Semantic Engine initialized\n");
    return engine;
}

void semantic_engine_free(SemanticEngine *engine) {
    if (engine != NULL) {
        if (engine->search_engine != NULL) {
            vector_search_free(engine->search_engine);
        }
        free(engine);
    }
}

int semantic_engine_index(SemanticEngine *engine) {
    if (engine == NULL || engine->search_engine == NULL) {
        return -1;
    }

    printf("\n[*] Starting semantic indexing...\n");
    return vector_search_index_all_files(engine->search_engine);
}

SearchResults* semantic_engine_search(SemanticEngine *engine, const char *query, int top_k) {
    if (engine == NULL || query == NULL) {
        return NULL;
    }

    printf("\n[*] Searching: \"%s\"\n", query);
    return vector_search_query(engine->search_engine, query, top_k);
}

void semantic_engine_print_results(SearchResults *results) {
    if (results == NULL || results->count == 0) {
        printf("[!] No results found\n");
        return;
    }

    printf("\n╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║              SEMANTIC SEARCH RESULTS (Top %d)              ║\n", results->count);
    printf("╠═══════════════════════════════════════════════════════════════╣\n");

    for (int i = 0; i < results->count; i++) {
        SearchResult *result = &results->results[i];
        printf("║ [%d] %s\n", i + 1, result->filename);
        printf("║     Path: %s\n", result->filepath);
        printf("║     Similarity: %.2f%%\n", result->similarity_score * 100);
        printf("║\n");
    }

    printf("╚═══════════════════════════════════════════════════════════════╝\n");
}
