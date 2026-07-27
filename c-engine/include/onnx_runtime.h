// onnx_runtime.h
#ifndef ONNX_RUNTIME_H
#define ONNX_RUNTIME_H

#include <onnxruntime_c_api.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_TOKENS 512
#define EMBEDDING_DIM_MINILM 384
#define EMBEDDING_DIM_LLAMA 4096
#define MAX_BATCH_SIZE 32

typedef struct {
    OrtSession *session;
    const OrtApi *api;
    OrtEnv *env;
    OrtMemoryInfo *meminfo;
    char model_path[1024];
} ONNXModel;

typedef struct {
    int *token_ids;
    int token_count;
    int *attention_mask;
} TokenizedInput;

typedef struct {
    float *embeddings;
    int embedding_dim;
    int batch_size;
} EmbeddingOutput;

// Model Management
ONNXModel* onnx_model_load(const char *model_path, const char *tokenizer_path);
void onnx_model_free(ONNXModel *model);

// Tokenization
TokenizedInput* tokenize_text(const char *text, const char *tokenizer_path, int max_length);
void tokenized_input_free(TokenizedInput *input);

// Inference
EmbeddingOutput* onnx_generate_embedding(ONNXModel *model, TokenizedInput *tokens);
void embedding_output_free(EmbeddingOutput *output);

// Utilities
float cosine_similarity(float *vec1, float *vec2, int dim);
float* combine_embeddings(float *emb1, int dim1, float *emb2, int dim2, float weight1, float weight2);

#endif
