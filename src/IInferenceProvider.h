#pragma once

#include <string>

// Small backend abstraction used by the examples so they can talk to either
// a local llama.cpp runtime or a remote HTTP API through the same interface.
class IInferenceProvider {
public:
    virtual ~IInferenceProvider();

    // Prepares the backend. For local backends this is usually a model path,
    // for remote backends it is an endpoint URL.
    virtual bool setup(const std::string& modelOrUrl) = 0;
    // Runs a single prompt and returns the fully collected response text.
    virtual std::string generate(const std::string& prompt) = 0;
    // Allows the UI examples to branch between local and remote behavior.
    virtual bool isRemote() const = 0;
};
