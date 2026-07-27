// semantic_engine.h
#ifndef SEMANTIC_ENGINE_H
#define SEMANTIC_ENGINE_H

#include "vector_search.h"
#include "json_exporter.h"

typedef struct {
    VectorSearchEngine *search_engine;
    Database *db;
} SemanticEngine;

SemanticEngine* semantic_engine_init(Database *db, const char *minilm_path, const char *llama_path);
void semantic_engine_free(SemanticEngine *engine);

int semantic_engine_index(SemanticEngine *engine);
SearchResults* semantic_engine_search(SemanticEngine *engine, const char *query, int top_k);
void semantic_engine_print_results(SearchResults *results);

#endif
