// vector_search.h
#ifndef VECTOR_SEARCH_H
#define VECTOR_SEARCH_H

#include "database.h"
#include "onnx_runtime.h"

#define MAX_SEARCH_RESULTS 10
#define SIMILARITY_THRESHOLD 0.7f

typedef struct {
    int file_id;
    char filepath[MAX_PATH_LEN];
    char filename[256];
    float similarity_score;
} SearchResult;

typedef struct {
    SearchResult *results;
    int count;
    int capacity;
} SearchResults;

typedef struct {
    Database *db;
    ONNXModel *model_minilm;
    ONNXModel *model_llama;
    float *stored_embeddings;
    int embedding_count;
    int embedding_dim;
} VectorSearchEngine;

// Engine initialization
VectorSearchEngine* vector_search_init(Database *db, const char *minilm_path, const char *llama_path);
void vector_search_free(VectorSearchEngine *engine);

// Indexing
int vector_search_index_all_files(VectorSearchEngine *engine);
int vector_search_store_embedding(VectorSearchEngine *engine, int file_id, float *embedding, int dim);

// Search
SearchResults* vector_search_query(VectorSearchEngine *engine, const char *query, int top_k);
void search_results_free(SearchResults *results);

// Utilities
float* generate_query_embedding(VectorSearchEngine *engine, const char *query);

#endif
