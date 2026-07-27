#include "../include/onnx_runtime.h"
#include <math.h>
#include <cjson/cJSON.h>

static void check_status(OrtStatus *status, const OrtApi *api) {
    if (status != NULL) {
        fprintf(stderr, "ONNX Error: %s\n", api->GetErrorMessage(status));
        api->ReleaseStatus(status);
    }
}

ONNXModel* onnx_model_load(const char *model_path, const char *tokenizer_path) {
    ONNXModel *model = (ONNXModel *)malloc(sizeof(ONNXModel));
    strcpy(model->model_path, model_path);

    // Get ONNX API
    model->api = OrtGetApiBase()->GetApi(ORT_API_VERSION);
    
    // Create environment
    OrtStatus *status = model->api->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "onnx_test", &model->env);
    check_status(status, model->api);

    // Create session options
    OrtSessionOptions *session_options;
    status = model->api->CreateSessionOptions(&session_options);
    check_status(status, model->api);

    // Set execution provider (CPU)
    status = model->api->SessionOptionsAppendExecutionProvider_CPU(session_options, 0);
    check_status(status, model->api);

    // Create session
    status = model->api->CreateSession(model->env, model_path, session_options, &model->session);
    check_status(status, model->api);

    model->api->ReleaseSessionOptions(session_options);

    // Create memory info
    status = model->api->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &model->meminfo);
    check_status(status, model->api);

    printf("[✓] ONNX Model loaded: %s\n", model_path);
    return model;
}

void onnx_model_free(ONNXModel *model) {
    if (model != NULL) {
        if (model->session != NULL) {
            model->api->ReleaseSession(model->session);
        }
        if (model->meminfo != NULL) {
            model->api->ReleaseMemoryInfo(model->meminfo);
        }
        if (model->env != NULL) {
            model->api->ReleaseEnv(model->env);
        }
        free(model);
    }
}

TokenizedInput* tokenize_text(const char *text, const char *tokenizer_path, int max_length) {
    TokenizedInput *input = (TokenizedInput *)malloc(sizeof(TokenizedInput));
    input->token_ids = (int *)malloc(sizeof(int) * max_length);
    input->attention_mask = (int *)malloc(sizeof(int) * max_length);

    // Simple tokenization (word-based for now)
    // In production, use proper tokenizer library
    int pos = 0;
    const char *ptr = text;
    
    input->token_ids[pos++] = 101;  // [CLS] token
    
    while (*ptr && pos < max_length - 1) {
        if (*ptr == ' ') {
            ptr++;
            continue;
        }
        // Simplified: each word gets a token ID
        input->token_ids[pos++] = (int)(rand() % 30522);  // vocab size
        while (*ptr && *ptr != ' ') ptr++;
    }
    
    input->token_ids[pos++] = 102;  // [SEP] token
    input->token_count = pos;

    // Set attention mask (all 1s for valid tokens)
    for (int i = 0; i < input->token_count; i++) {
        input->attention_mask[i] = 1;
    }
    for (int i = input->token_count; i < max_length; i++) {
        input->attention_mask[i] = 0;
    }

    return input;
}

void tokenized_input_free(TokenizedInput *input) {
    if (input != NULL) {
        free(input->token_ids);
        free(input->attention_mask);
        free(input);
    }
}

EmbeddingOutput* onnx_generate_embedding(ONNXModel *model, TokenizedInput *tokens) {
    if (model == NULL || model->session == NULL) {
        fprintf(stderr, "[!] Model not initialized\n");
        return NULL;
    }

    EmbeddingOutput *output = (EmbeddingOutput *)malloc(sizeof(EmbeddingOutput));
    output->embedding_dim = EMBEDDING_DIM_MINILM;  // Default to MiniLM
    output->batch_size = 1;
    output->embeddings = (float *)malloc(sizeof(float) * output->embedding_dim);

    // Prepare input tensors
    int64_t input_shape[] = {1, tokens->token_count};
    
    OrtValue *input_ids_tensor = NULL;
    OrtValue *attention_mask_tensor = NULL;
    OrtValue *output_tensor = NULL;

    const char *input_names[] = {"input_ids", "attention_mask"};
    const char *output_names[] = {"last_hidden_state"};

    // Create input tensors
    OrtStatus *status = model->api->CreateTensorWithDataAsOrtValue(
        model->meminfo, tokens->token_ids, sizeof(int) * tokens->token_count,
        input_shape, 2, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32, &input_ids_tensor);
    check_status(status, model->api);

    status = model->api->CreateTensorWithDataAsOrtValue(
        model->meminfo, tokens->attention_mask, sizeof(int) * tokens->token_count,
        input_shape, 2, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32, &attention_mask_tensor);
    check_status(status, model->api);

    // Run inference
    OrtValue *input_tensors[] = {input_ids_tensor, attention_mask_tensor};
    status = model->api->Run(model->session, NULL, input_names, input_tensors, 2,
                             output_names, 1, &output_tensor);
    check_status(status, model->api);

    // Get output data
    if (output_tensor != NULL) {
        float *output_data = NULL;
        status = model->api->GetTensorMutableData(output_tensor, (void**)&output_data);
        check_status(status, model->api);
        
        if (output_data != NULL) {
            // Copy mean pooling of output
            memset(output->embeddings, 0, sizeof(float) * output->embedding_dim);
            for (int i = 0; i < tokens->token_count && i < output->embedding_dim; i++) {
                output->embeddings[i] = output_data[i];
            }
        }
    }

    // Cleanup
    if (input_ids_tensor != NULL) model->api->ReleaseValue(input_ids_tensor);
    if (attention_mask_tensor != NULL) model->api->ReleaseValue(attention_mask_tensor);
    if (output_tensor != NULL) model->api->ReleaseValue(output_tensor);

    return output;
}

void embedding_output_free(EmbeddingOutput *output) {
    if (output != NULL) {
        free(output->embeddings);
        free(output);
    }
}

float cosine_similarity(float *vec1, float *vec2, int dim) {
    if (vec1 == NULL || vec2 == NULL || dim <= 0) {
        return 0.0f;
    }

    float dot_product = 0.0f;
    float norm1 = 0.0f;
    float norm2 = 0.0f;

    for (int i = 0; i < dim; i++) {
        dot_product += vec1[i] * vec2[i];
        norm1 += vec1[i] * vec1[i];
        norm2 += vec2[i] * vec2[i];
    }

    norm1 = sqrtf(norm1);
    norm2 = sqrtf(norm2);

    if (norm1 == 0.0f || norm2 == 0.0f) {
        return 0.0f;
    }

    return dot_product / (norm1 * norm2);
}

float* combine_embeddings(float *emb1, int dim1, float *emb2, int dim2, float weight1, float weight2) {
    int result_dim = (dim1 < dim2) ? dim1 : dim2;
    float *combined = (float *)malloc(sizeof(float) * result_dim);

    float total_weight = weight1 + weight2;
    weight1 /= total_weight;
    weight2 /= total_weight;

    for (int i = 0; i < result_dim; i++) {
        combined[i] = (weight1 * emb1[i]) + (weight2 * emb2[i]);
    }

    return combined;
}
