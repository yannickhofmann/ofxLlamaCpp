/*
 * ofxLlamaCpp
 *
 * Copyright (c) 2025 Yannick Hofmann
 * <contact@yannickhofmann.de>
 *
 * BSD Simplified License.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
 
 // This is a header file for the ofxLlamaCpp addon for OpenFrameworks.
 #pragma once
#include "ofMain.h" // Includes core OpenFrameworks functionalities

#include "../libs/llama.cpp/include/llama.h" // Includes the Llama.cpp library headers
#include "../libs/llama.cpp/ggml/include/ggml-backend.h"

#include <thread>     // For multi-threading operations
#include <mutex>      // For protecting shared data in multi-threaded environments
#include <vector>     // For dynamic arrays
#include <string>     // For string manipulation
#include <functional> // For std::function callbacks

// The main class for interacting with Llama models in OpenFrameworks.
class ofxLlamaCpp {
public:
    // Constructor: Initializes the ofxLlamaCpp object.
    ofxLlamaCpp();
    // Destructor: Cleans up resources, unloads the model if loaded.
    ~ofxLlamaCpp();

    // -----------------------------
    // Model Management
    // -----------------------------
    // Loads a Llama model from the specified path.
    // n_ctx defines the context size (how many tokens the model "remembers").
    bool loadModel(const std::string &path, int n_ctx = 2048);
    // Unloads the currently loaded model and frees resources.
    void unload();
    // Checks if a model is currently loaded.
    bool isModelLoaded() const;

    // Sets the number of layers to offload to the GPU.
    void setN_GpuLayers(int n_gpu_layers_val);

    // Returns the vocabulary size of the loaded model.
    int getVocabSize() const;
    // Returns the context size of the loaded model.
    int getContextSize() const;
    int getNLayers() const; // Returns the total number of layers in the loaded model.

    // Sets whether to offload K, Q, V tensors to the GPU.
    void setOffloadKqv(bool offload_kqv_val);

    // Returns the number of layers to offload to the GPU.
    int getN_GpuLayers() const;
    // Returns whether K, Q, V tensors are offloaded to the GPU.
    bool getOffloadKqv() const;


    // -----------------------------
    // Generation Control
    // -----------------------------
    // Starts asynchronous text generation based on the given prompt.
    // maxTokens specifies the maximum number of new tokens to generate.
    void startGeneration(const std::string &prompt, int maxTokens = 200);
    // Requests to stop the current asynchronous generation.
    void stopGeneration();
    // Checks if text generation is currently in progress.
    bool isGenerating() const;
    // Retrieves newly generated output tokens. Useful for streaming results.
    std::string getNewOutput();

    // -----------------------------
    // Stop Sequences
    // -----------------------------
    // Adds a word or phrase that, when generated, will stop further text generation.
    void addStopWord(const std::string &s);
    // Clears all defined stop words.
    void clearStopWords();
    // Returns a constant reference to the list of stop words.
    const std::vector<std::string>& getStopWords() const;

    // -----------------------------
    // Token Helpers
    // -----------------------------
    // Converts a string of text into a vector of Llama tokens.
    std::vector<llama_token> tokenize(const std::string &text) const;
    // Converts a vector of Llama tokens back into a string of text.
    std::string detokenize(const std::vector<llama_token> &tokens) const;

    // -----------------------------
    // Context Reset (SAFE VERSION)
    // -----------------------------
    // Resets the model's internal context, effectively starting a new conversation.
    void resetContext(); 
    // Returns the ratio of how full the model's context window is (0.0 to 1.0).
    float getContextFillRatio() const;

    // -----------------------------
    // Sampler Settings
    // -----------------------------
    // Sets the temperature for text generation. Higher values mean more random output.
    void setTemperature(float t);
    // Sets the Top-P (nucleus sampling) parameter.
    void setTopP(float p);
    // Sets the Top-K sampling parameter.
    void setTopK(int k);

    // Sets the penalty for repeating tokens. Higher values discourage repetition.
    void setRepeatPenalty(float p);
    // Sets the presence penalty, influencing how likely new tokens are to appear.
    void setPresencePenalty(float p);
    // Sets the frequency penalty, influencing how often tokens that have already appeared are generated.
    void setFrequencyPenalty(float p);

    // Sets the minimum number of tokens to generate per turn.
    void setMinTokens(int n);
    // Sets the maximum number of tokens to generate per turn.
    void setMaxTokens(int n);

    // -----------------------------
    // Callbacks
    // -----------------------------
    // Sets a callback function that is called whenever a new token is generated.
    void setTokenCallback(std::function<void(const std::string &)> fn);
    // Sets a callback function that is called when text generation finishes.
    void setFinishCallback(std::function<void()> fn);

    // -----------------------------
    // Chat API
    // -----------------------------
    // Structure to represent a single message in a chat conversation.
    struct ChatMessage {
        std::string role;    // The role of the speaker (e.g., "user", "assistant", "system")
        std::string content; // The content of the message
    };

    // Adds a message to the internal chat history.
    void addMessage(const std::string& role, const std::string& content);
    // Clears the entire chat history.
    void clearMessages();
    // Generates a chat response based on the current chat history.
    std::string generateChat(int maxTokens);

private:
    // Internal function to build and configure the Llama sampler.
    void buildSampler();
    // The main loop for asynchronous text generation, run in a separate thread.
    void generationLoop();
    // Checks if any of the defined stop sequences have been generated.
    bool checkStopSequences(const std::string& s);

private:
    // Pointers to the Llama model and context, managed by the Llama.cpp library.
    llama_model *model = nullptr;
    llama_context *ctx = nullptr;

    // Path to the loaded model file.
    std::string modelPath;

    // The context size (maximum number of tokens the model can process at once).
    int contextSize = 2048;

    // Pointer to the Llama sampler, responsible for token selection during generation.
    llama_sampler *sampler = nullptr;

    // Worker thread for asynchronous operations like text generation.
    std::thread worker;
    // Mutex to protect shared resources when accessed from different threads.
    mutable std::mutex mtx;

    // Flag indicating if text generation is currently active.
    bool generating = false;
    // Flag to signal the worker thread to stop generation.
    bool requestStop = false;

    // Stores newly generated output before it's retrieved by getNewOutput().
    std::string pendingOut;
    // Stores the prompt currently being processed.
    std::string currentPrompt;

    // Sampler settings (parameters controlling how text is generated).
    float temperature = 0.8f;
    float top_p = 0.9f;
    int top_k = 40;

    float repeat_penalty = 1.1f;
    float presence_penalty = 0.0f;
    float frequency_penalty = 0.0f;

    // Minimum and maximum tokens to generate in a single call.
    int min_gen_tokens = 0;
    int max_gen_tokens = 200;

    // List of words or phrases that stop generation.
    std::vector<std::string> stopWords;

    // Stores the history of chat messages for conversational models.
    std::vector<ChatMessage> chatHistory;

    // Callback functions for token-by-token output and generation completion.
    std::function<void(const std::string&)> tokenCallback;
    std::function<void()> finishCallback;

    int n_gpu_layers = 0; // Number of layers to offload to the GPU
    bool offload_kqv = true; // Offload K, Q, V tensors to the GPU by default

    ggml_backend_t cpu_backend = nullptr;
    ggml_backend_t cuda_backend = nullptr;
};

