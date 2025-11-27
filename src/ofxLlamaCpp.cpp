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

#include "ofxLlamaCpp.h" // Include the main header for ofxLlama
#include "ofLog.h"    // For OpenFrameworks logging utilities
#include "../libs/llama.cpp/ggml/include/ggml-cpu.h"

#ifdef __APPLE__
#include "../libs/llama.cpp/ggml/include/ggml-metal.h"
#else
#include "../libs/llama.cpp/ggml/include/ggml-cuda.h"
#endif


// --------------------------------------------------------------
// Constructor for ofxLlamaCpp.
// Initializes the Llama.cpp backend. This must be called once before using Llama.cpp.
ofxLlamaCpp::ofxLlamaCpp() {
    llama_backend_init();

    // Explicitly register CPU backend
    ggml_backend_register(ggml_backend_cpu_reg());

#ifdef __APPLE__
    ggml_backend_register(ggml_backend_metal_reg());
#else
    // Conditionally register CUDA backend if GPU offload is supported
    // Temporarily removing the conditional check to ensure registration happens
    // if CUDA is compiled in.
    ggml_backend_register(ggml_backend_cuda_reg());
    ggml_backend_dev_count(); // Force enumeration of CUDA devices
#endif
}

// --------------------------------------------------------------
// Destructor for ofxLlamaCpp.
// Ensures any ongoing generation is stopped, the model is unloaded,
// and the Llama.cpp backend resources are freed.
ofxLlamaCpp::~ofxLlamaCpp() {
    stopGeneration();    // Stop any active generation thread
    unload();            // Unload the model and free its resources
    llama_backend_free(); // Free Llama.cpp backend resources
}

// --------------------------------------------------------------
// Loads a Llama model from the specified file path.
// If a model is already loaded, it will be unloaded first.
// `n_ctx_req` is the requested context size for the model.
bool ofxLlamaCpp::loadModel(const std::string& path, int n_ctx_req) {
    unload(); // Always unload any previously loaded model

    // Set default model parameters, then load the model.
    // n_gpu_layers = 0 means no GPU offloading by default.
    llama_model_params mp = llama_model_default_params();
    mp.n_gpu_layers = this->n_gpu_layers;

#ifdef __APPLE__
    ggml_backend_dev_t cuda_dev = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_GPU);
    if (cuda_dev != nullptr) {
        static std::vector<ggml_backend_dev_t> devices_list;
        devices_list.clear();
        devices_list.push_back(cuda_dev);
        devices_list.push_back(nullptr); // Null-terminate the list
        mp.devices = devices_list.data();
    } else {
        if (this->n_gpu_layers > 0) {
            ofLogWarning("ofxLlamaCpp") << "GPU offloading requested but no CUDA device found. Falling back to CPU.";
        }
    }
#endif

    model = llama_model_load_from_file(path.c_str(), mp);
    if (!model) {
        ofLogError("ofxLlamaCpp") << "Failed to load model: " << path;
        return false;
    }

    modelPath = path; // Store the path of the loaded model

    // Set default context parameters.
    llama_context_params cp = llama_context_default_params();
    cp.n_ctx = n_ctx_req; // Set the requested context size
    cp.flash_attn_type = LLAMA_FLASH_ATTN_TYPE_AUTO; // Use auto flash attention type
    cp.n_batch = 512; // Batch size for parallel processing
    cp.n_ubatch = 1; // Unbatch size
    cp.n_threads = std::thread::hardware_concurrency(); // Use all available hardware threads
    cp.offload_kqv = this->offload_kqv; // Set offload_kqv from member variable

    // Initialize the Llama context from the loaded model.
    ctx = llama_init_from_model(model, cp);

    if (!ctx) {
        ofLogError("ofxLlamaCpp") << "Failed creating context.";
        llama_model_free(model); // Free model if context creation fails
        model = nullptr;
        return false;
    }

    contextSize = n_ctx_req; // Store the actual context size

    buildSampler(); // Initialize the token sampler with current settings
    return true; // Model loaded successfully
}

// --------------------------------------------------------------
// Unloads the current Llama model and frees associated resources.
void ofxLlamaCpp::unload() {
    if (sampler) {
        llama_sampler_free(sampler); // Free the sampler if it exists
        sampler = nullptr;
    }
    if (ctx)   { llama_free(ctx);   ctx = nullptr; }     // Free the context
    if (model) { llama_model_free(model); model = nullptr; } // Free the model
}

// --------------------------------------------------------------
// Checks if a Llama model and its context are currently loaded.
bool ofxLlamaCpp::isModelLoaded() const {
    return model != nullptr && ctx != nullptr;
}

// --------------------------------------------------------------
// Returns the vocabulary size of the loaded model.
int ofxLlamaCpp::getVocabSize() const {
    if (!model) return 0;
    const llama_vocab* vocab = llama_model_get_vocab(model);
    if (!vocab) return 0;
    return llama_vocab_n_tokens(vocab);
}

// --------------------------------------------------------------
// Returns the context size of the loaded model.
int ofxLlamaCpp::getContextSize() const {
    return ctx ? llama_n_ctx(ctx) : 0;
}

// --------------------------------------------------------------
// Returns the total number of layers in the loaded model.
int ofxLlamaCpp::getNLayers() const {
    return model ? llama_model_n_layer(model) : 0;
}

// --------------------------------------------------------------
// Sets the number of layers to offload to the GPU.
void ofxLlamaCpp::setN_GpuLayers(int n_gpu_layers_val) {
    n_gpu_layers = n_gpu_layers_val;
    ofLogNotice("ofxLlamaCpp") << "n_gpu_layers set to: " << n_gpu_layers;
}

// --------------------------------------------------------------
// Sets whether to offload K, Q, V tensors to the GPU.
void ofxLlamaCpp::setOffloadKqv(bool offload_kqv_val) {
    offload_kqv = offload_kqv_val;
    ofLogNotice("ofxLlamaCpp") << "offload_kqv set to: " << offload_kqv;
}

// --------------------------------------------------------------
// Returns the number of layers to offload to the GPU.
int ofxLlamaCpp::getN_GpuLayers() const {
    return n_gpu_layers;
}

// --------------------------------------------------------------
// Returns whether K, Q, V tensors are offloaded to the GPU.
bool ofxLlamaCpp::getOffloadKqv() const {
    return offload_kqv;
}

// --------------------------------------------------------------
// Sampler Settings: These functions update parameters for text generation
// and then rebuild the sampler to apply the changes.
void ofxLlamaCpp::setTemperature(float t)       { temperature = t; buildSampler(); }
void ofxLlamaCpp::setTopP(float p)              { top_p = p;       buildSampler(); }
void ofxLlamaCpp::setTopK(int k)                { top_k = k;       buildSampler(); }
void ofxLlamaCpp::setRepeatPenalty(float p)     { repeat_penalty = p; buildSampler(); }
void ofxLlamaCpp::setPresencePenalty(float p)   { presence_penalty = p; buildSampler(); }
void ofxLlamaCpp::setFrequencyPenalty(float p)  { frequency_penalty = p; buildSampler(); }
void ofxLlamaCpp::setMinTokens(int n)           { min_gen_tokens = n; } // Set minimum tokens to generate
void ofxLlamaCpp::setMaxTokens(int n)           { max_gen_tokens = n; } // Set maximum tokens to generate

// --------------------------------------------------------------
// Adds a word or phrase to the list of stop sequences.
// If the model generates any of these, generation will stop.
void ofxLlamaCpp::addStopWord(const std::string& s) {
    stopWords.push_back(s);
}

// --------------------------------------------------------------
// Clears all currently set stop words.
void ofxLlamaCpp::clearStopWords() {
    stopWords.clear();
}

// --------------------------------------------------------------
// Returns a constant reference to the vector of stop words.
const std::vector<std::string>& ofxLlamaCpp::getStopWords() const {
    return stopWords;
}

// --------------------------------------------------------------
// Adds a message to the internal chat history.
// Used for conversational models where context builds up over turns.
void ofxLlamaCpp::addMessage(const std::string& role, const std::string& content) {
    chatHistory.push_back({role, content});
}

// --------------------------------------------------------------
// Clears the entire chat history.
void ofxLlamaCpp::clearMessages() {
    chatHistory.clear();
}

// --------------------------------------------------------------
// Generates a chat response based on the current chat history.
// It constructs a prompt from the history, starts generation, waits for it to complete,
// and then returns the generated text.
std::string ofxLlamaCpp::generateChat(int maxTokens) {
    std::string prompt;

    // Format chat history into a single prompt string.
    for (auto& m : chatHistory) {
        prompt += "<|" + m.role + "|>" + m.content + "\n";
    }
    prompt += "<|assistant|>"; // Indicate that the assistant should respond next

    startGeneration(prompt, maxTokens); // Start the generation process

    // Wait for the generation to finish (blocking call).
    while (isGenerating()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Sleep briefly to avoid busy-waiting
    }

    return pendingOut; // Return the full generated output
}

// --------------------------------------------------------------
// Converts a given string into a vector of Llama tokens.
std::vector<llama_token> ofxLlamaCpp::tokenize(const std::string& text) const {
    const llama_vocab* vocab = llama_model_get_vocab(model); // Get the model's vocabulary

    // Allocate a buffer for tokens (text size + some extra for safety).
    std::vector<llama_token> out(text.size() + 8);
    int n = llama_tokenize(
        vocab,
        text.c_str(),
        (int)text.size(),
        out.data(),
        (int)out.size(),
        false, // Add bos (beginning of sentence) token
        false  // Add eos (end of sentence) token
    );

    out.resize(std::max(0, n)); // Resize to actual number of tokens
    return out;
}

// --------------------------------------------------------------
// Converts a vector of Llama tokens back into a string.
std::string ofxLlamaCpp::detokenize(const std::vector<llama_token>& toks) const {
    std::string out;

    out.reserve(toks.size() * 6); // Pre-allocate memory for efficiency

    const llama_vocab* vocab = llama_model_get_vocab(model); // Get the model's vocabulary
    char buf[32]; // Small buffer for token pieces

    for (auto tok : toks) {
        int n = llama_token_to_piece(
            vocab,
            tok,
            buf,
            sizeof(buf),
            0, // index of the piece in the token
            false // special token
        );
        if (n > 0) out.append(buf, n); // Append the token piece to the output string
    }

    return out;
}

// --------------------------------------------------------------
// Resets the model's context. This is crucial for starting new conversations
// without interference from previous ones.
void ofxLlamaCpp::resetContext() {
    if (ctx) {
        // Reset the memory for sequence 0 from position 0 to -1 (entire sequence).
        llama_memory_seq_rm(llama_get_memory(ctx), 0, 0, -1);
    }
}

// --------------------------------------------------------------
// Calculates and returns the ratio of the context window that is currently filled.
// A value of 1.0 means the context is full.
float ofxLlamaCpp::getContextFillRatio() const {
    llama_memory_t mem = llama_get_memory(ctx);
    llama_pos mx = llama_memory_seq_pos_max(mem, 0); // Get max position for sequence 0
    return float(mx) / float(getContextSize()); // Ratio of used context to total context size
}

// --------------------------------------------------------------
// Sets a callback function to be invoked whenever a new token is generated.
// This is useful for streaming output to the user as it's generated.
void ofxLlamaCpp::setTokenCallback(std::function<void(const std::string&)> fn) {
    tokenCallback = fn;
}

// --------------------------------------------------------------
// Sets a callback function to be invoked when the text generation process finishes.
void ofxLlamaCpp::setFinishCallback(std::function<void()> fn) {
    finishCallback = fn;
}

// --------------------------------------------------------------
// Builds or rebuilds the Llama sampler with the current generation parameters.
// This defines how tokens are selected during the generation process (e.g., top-k, top-p, temperature).
void ofxLlamaCpp::buildSampler() {
    if (sampler) {
        llama_sampler_free(sampler); // Free existing sampler before rebuilding
    }

    auto params = llama_sampler_chain_default_params(); // Get default sampler chain parameters
    sampler = llama_sampler_chain_init(params); // Initialize a new sampler chain

    // Add various sampling methods to the chain. Order matters.
    llama_sampler_chain_add(sampler, llama_sampler_init_top_k(top_k));       // Top-K sampling
    llama_sampler_chain_add(sampler, llama_sampler_init_top_p(top_p, 1));     // Top-P (nucleus) sampling
    llama_sampler_chain_add(sampler, llama_sampler_init_temp(temperature));  // Temperature scaling

    // Add penalties for repetition, frequency, and presence.
    llama_sampler_chain_add(
        sampler,
        llama_sampler_init_penalties(
            -1, // TODO: Check if this token_idx should be 0 or -1 (llama.cpp example uses 0 for last token)
            repeat_penalty,
            frequency_penalty,
            presence_penalty
        )
    );

    llama_sampler_chain_add(sampler, llama_sampler_init_greedy()); // Greedy sampling (selects the most likely token)
}

// --------------------------------------------------------------
// Checks if the generated string `s` ends with any of the defined stop words.
// Returns true if a stop word is found at the end of the string, false otherwise.
bool ofxLlamaCpp::checkStopSequences(const std::string& s) {
    for (auto& w : stopWords) {
        if (!w.empty() && 
            s.size() >= w.size() &&
            s.compare(s.size() - w.size(), w.size(), w) == 0) {
            return true; // A stop word was found
        }
    }
    return false; // No stop word found
}

// --------------------------------------------------------------
// Initiates asynchronous text generation in a separate thread.
// The generated text will be available via getNewOutput() or through callbacks.
void ofxLlamaCpp::startGeneration(const std::string& prompt, int maxTokens) {
    if (!ctx) return; // Cannot generate if no context is loaded

    stopGeneration(); // Stop any existing generation before starting a new one

    {
        std::lock_guard<std::mutex> lock(mtx); // Protect shared variables with a mutex
        currentPrompt = prompt;              // Store the prompt for the worker thread
        pendingOut.clear();                  // Clear any previous output
        max_gen_tokens = maxTokens;          // Set the maximum tokens for this generation
        generating = true;                   // Mark generation as active
        requestStop = false;                 // Reset stop request flag
    }

    // Launch the generation loop in a new thread.
    worker = std::thread(&ofxLlamaCpp::generationLoop, this);
}

// --------------------------------------------------------------
// Requests the ongoing asynchronous generation to stop.
// It sets a flag and waits for the worker thread to finish.
void ofxLlamaCpp::stopGeneration() {
    requestStop = true; // Signal the worker thread to stop

    if (worker.joinable()) {
        worker.join(); // Wait for the worker thread to complete its current task and terminate
    }
}

// --------------------------------------------------------------
// Returns true if text generation is currently in progress, false otherwise.
bool ofxLlamaCpp::isGenerating() const {
    return generating;
}

// --------------------------------------------------------------
// Retrieves any newly generated output from the model.
// This method is thread-safe and clears the internal buffer after returning the output.
std::string ofxLlamaCpp::getNewOutput() {
    std::lock_guard<std::mutex> lock(mtx); // Protect shared 'pendingOut'
    std::string out = pendingOut;         // Copy the pending output
    pendingOut.clear();                   // Clear the buffer
    return out;                           // Return the copied output
}

// --------------------------------------------------------------
// The main generation loop, executed in a separate thread.
// This function handles tokenizing the prompt, processing it, and then
// iteratively generating new tokens until maxTokens is reached or a stop word is encountered.
void ofxLlamaCpp::generationLoop() {

    // Reset the context to clear previous conversation state.
    resetContext();

    std::string prompt;
    {
        std::lock_guard<std::mutex> lock(mtx); // Access currentPrompt safely
        prompt = currentPrompt;
    }

    auto tokens = tokenize(prompt); // Tokenize the input prompt

    int n_past = 0; // Tracks the number of tokens processed so far
    // Process the prompt tokens in batches.
    while (n_past < tokens.size()) {
        int n_eval = std::min((int)tokens.size() - n_past, (int)llama_n_batch(ctx)); // Determine batch size

        // Prepare a llama_batch for evaluation.
        llama_batch batch = llama_batch_init(n_eval, 0, 1);
        
        for (int i = 0; i < n_eval; ++i) {
            batch.token[i]      = tokens[n_past + i]; // Add token
            batch.pos[i]        = n_past + i;        // Set position in context
            batch.n_seq_id[i]   = 1;
            batch.seq_id[i][0]  = 0;                 // Sequence ID
            batch.logits[i]     = (n_past + i == tokens.size() - 1); // Request logits for the last token in batch
        }
        batch.n_tokens = n_eval; // Number of tokens in this batch

        // Decode the batch of tokens.
        if (llama_decode(ctx, batch) != 0) {
            ofLogError("ofxLlamaCpp") << "llama_decode failed during prompt processing";
            llama_batch_free(batch);
            generating = false;
            return;
        }
        
        n_past += n_eval; // Update past token count
        llama_batch_free(batch); // Free batch resources
    }

    std::string generated; // String to accumulate all generated tokens

    // Generate new tokens one by one.
    for (int t = 0; t < max_gen_tokens; t++) {

        if (requestStop) break; // Check if a stop request has been made

        // Sample a new token using the configured sampler.
        llama_token tok = llama_sampler_sample(sampler, ctx, -1);

        const llama_vocab* vocab = llama_model_get_vocab(model); // Get vocabulary
        if (tok == llama_vocab_eos(vocab)) break; // Stop if End-Of-Sentence token is generated

        char buf[32]; // Buffer for the token piece
        int n = llama_token_to_piece(
            vocab,
            tok,
            buf,
            sizeof(buf),
            0,
            false
        );

        std::string piece;
        if (n > 0) piece.assign(buf, n); // Convert token piece to string

        {
            std::lock_guard<std::mutex> lock(mtx);
            pendingOut += piece; // Append to pending output (for streaming)
        }

        generated += piece; // Append to full generated string

        if (checkStopSequences(generated)) break; // Check for stop words

        // Prepare a new batch for the newly generated token for subsequent decoding.
        llama_batch bx = llama_batch_init(1, 0, 1);

        bx.n_tokens = 1;
        bx.token[0] = tok;      // The new token
        bx.pos[0] = n_past;     // Its position in the context
        bx.n_seq_id[0] = 1;
        bx.seq_id[0][0] = 0;
        bx.logits[0] = true;    // Request logits for this token

        // Decode the new token.
        if (llama_decode(ctx, bx) != 0) {
            ofLogError("ofxLlamaCpp") << "llama_decode failed during token processing";
            llama_batch_free(bx);
            generating = false;
            return;
        }
        
        llama_batch_free(bx); // Free batch resources
        n_past++;             // Increment past token count
    }

    generating = false; // Mark generation as complete
    if (finishCallback) finishCallback(); // Call the finish callback if set
}
