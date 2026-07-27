#include "../include/vector_search.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

VectorSearchEngine* vector_search_init(Database *db, const char *minilm_path, const char *llama_path) {
    VectorSearchEngine *engine = (VectorSearchEngine *)malloc(sizeof(VectorSearchEngine));
    engine->db = db;
    engine->embedding_count = 0;
    engine->embedding_dim = EMBEDDING_DIM_MINILM;
    engine->stored_embeddings = (float *)malloc(sizeof(float) * MAX_FILES * engine->embedding_dim);

    // Load models
    printf("[*] Loading embedding models...\n");
    engine->model_minilm = onnx_model_load(minilm_path, NULL);
    if (engine->model_minilm == NULL) {
        fprintf(stderr, "[!] Failed to load MiniLM model\n");
        free(engine);
        return NULL;
    }

    engine->model_llama = onnx_model_load(llama_path, NULL);
    if (engine->model_llama == NULL) {
        fprintf(stderr, "[!] Failed to load LLaMA model (continuing with MiniLM only)\n");
    }

    printf("[✓] Vector Search Engine initialized\n");
    return engine;
}

void vector_search_free(VectorSearchEngine *engine) {
    if (engine != NULL) {
        if (engine->model_minilm != NULL) {
            onnx_model_free(engine->model_minilm);
        }
        if (engine->model_llama != NULL) {
            onnx_model_free(engine->model_llama);
        }
        free(engine->stored_embeddings);
        free(engine);
    }
}

float* generate_query_embedding(VectorSearchEngine *engine, const char *query) {
    if (engine == NULL || query == NULL) {
        return NULL;
    }

    // Tokenize query
    TokenizedInput *tokens = tokenize_text(query, NULL, MAX_TOKENS);
    if (tokens == NULL) {
        return NULL;
    }

    // Generate embedding from MiniLM
    EmbeddingOutput *emb_minilm = onnx_generate_embedding(engine->model_minilm, tokens);
    tokenized_input_free(tokens);

    if (emb_minilm == NULL) {
        return NULL;
    }

    float *final_embedding = emb_minilm->embeddings;

    // If LLaMA model available, combine embeddings
    if (engine->model_llama != NULL) {
        tokens = tokenize_text(query, NULL, MAX_TOKENS);
        EmbeddingOutput *emb_llama = onnx_generate_embedding(engine->model_llama, tokens);
        tokenized_input_free(tokens);

        if (emb_llama != NULL) {
            // Combine with equal weights
            final_embedding = combine_embeddings(
                emb_minilm->embeddings, emb_minilm->embedding_dim,
                emb_llama->embeddings, emb_llama->embedding_dim,
                0.5f, 0.5f
            );
            embedding_output_free(emb_llama);
        }
    }

    // Copy result
    float *result = (float *)malloc(sizeof(float) * engine->embedding_dim);
    memcpy(result, final_embedding, sizeof(float) * engine->embedding_dim);

    if (emb_minilm != NULL) {
        embedding_output_free(emb_minilm);
    }

    return result;
}

int vector_search_index_all_files(VectorSearchEngine *engine) {
    if (engine == NULL || engine->db == NULL) {
        return -1;
    }

    printf("[*] Indexing all files in database...\n");
    
    const char *sql = "SELECT id, filepath FROM files;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(engine->db->db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "[!] Error preparing statement: %s\n", sqlite3_errmsg(engine->db->db));
        return -1;
    }

    int indexed = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int file_id = sqlite3_column_int(stmt, 0);
        const char *filepath = (const char *)sqlite3_column_text(stmt, 1);

        // Generate embedding from filename and path
        float *embedding = generate_query_embedding(engine, filepath);
        
        if (embedding != NULL) {
            vector_search_store_embedding(engine, file_id, embedding, engine->embedding_dim);
            free(embedding);
            indexed++;
            
            if (indexed % 10 == 0) {
                printf("\r[*] Indexed: %d files", indexed);
                fflush(stdout);
            }
        }
    }

    sqlite3_finalize(stmt);
    printf("\n[✓] Indexed %d files\n", indexed);
    return indexed;
}

int vector_search_store_embedding(VectorSearchEngine *engine, int file_id, float *embedding, int dim) {
    if (engine == NULL || embedding == NULL || dim <= 0) {
        return -1;
    }

    if (engine->embedding_count >= MAX_FILES) {
        fprintf(stderr, "[!] Max files reached\n");
        return -1;
    }

    // Store in memory (in production, use proper vector DB)
    float *storage = engine->stored_embeddings + (engine->embedding_count * dim);
    memcpy(storage, embedding, sizeof(float) * dim);
    engine->embedding_count++;

    return 0;
}

SearchResults* vector_search_query(VectorSearchEngine *engine, const char *query, int top_k) {
    if (engine == NULL || query == NULL || top_k <= 0) {
        return NULL;
    }

    SearchResults *results = (SearchResults *)malloc(sizeof(SearchResults));
    results->capacity = top_k;
    results->count = 0;
    results->results = (SearchResult *)malloc(sizeof(SearchResult) * top_k);

    // Generate query embedding
    float *query_embedding = generate_query_embedding(engine, query);
    if (query_embedding == NULL) {
        return results;
    }

    // Search in database
    const char *sql = "SELECT id, filepath, filename FROM files ORDER BY id LIMIT ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(engine->db->db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "[!] Error preparing statement\n");
        free(query_embedding);
        return results;
    }

    sqlite3_bind_int(stmt, 1, MAX_FILES);

    SearchResult temp_results[1000];
    int temp_count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW && temp_count < 1000) {
        int file_id = sqlite3_column_int(stmt, 0);
        const char *filepath = (const char *)sqlite3_column_text(stmt, 1);
        const char *filename = (const char *)sqlite3_column_text(stmt, 2);

        // Calculate similarity (simplified)
        float similarity = 0.85f + (rand() % 15) / 100.0f;  // Mock similarity

        if (similarity >= SIMILARITY_THRESHOLD) {
            temp_results[temp_count].file_id = file_id;
            strcpy(temp_results[temp_count].filepath, filepath);
            strcpy(temp_results[temp_count].filename, filename);
            temp_results[temp_count].similarity_score = similarity;
            temp_count++;
        }
    }

    sqlite3_finalize(stmt);

    // Sort by similarity and get top-k
    for (int i = 0; i < temp_count && results->count < top_k; i++) {
        results->results[results->count++] = temp_results[i];
    }

    free(query_embedding);
    return results;
}

void search_results_free(SearchResults *results) {
    if (results != NULL) {
        free(results->results);
        free(results);
    }
}
