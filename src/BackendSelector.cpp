#include "BackendSelector.h"

#include "RemoteAPIProvider.h"
#include "ofxLlamaCpp.h"

#include <chrono>
#include <thread>

namespace {
    // Lightweight adapter that lets the example apps use the same provider
    // interface for local llama.cpp inference as for remote HTTP backends.
    class LocalLlamaProvider : public IInferenceProvider {
    public:
        bool setup(const std::string& modelOrUrl) override {
            const int contextSize = 2048;

            if (!llama.loadModel(modelOrUrl, contextSize)) {
                ofLogError("LocalLlamaProvider") << "Failed to load local model: " << modelOrUrl;
                return false;
            }

            llama.setTemperature(0.8f);
            llama.setTopK(40);
            llama.addStopWord("User:");
            llama.addStopWord("Assistant:");

            return true;
        }

        std::string generate(const std::string& prompt) override {
            if (!llama.isModelLoaded()) {
                ofLogError("LocalLlamaProvider") << "No local model loaded.";
                return "";
            }

            std::string output;

            llama.stopGeneration();
            llama.startGeneration(prompt, 1024);

            while (llama.isGenerating()) {
                output += llama.getNewOutput();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            output += llama.getNewOutput();
            return output;
        }

        bool isRemote() const override {
            return false;
        }

    private:
        ofxLlamaCpp llama;
    };
}

std::shared_ptr<IInferenceProvider> BackendSelector::create(BackendType type) {
    switch (type) {
        case BackendType::REMOTE:
            return std::make_shared<RemoteAPIProvider>();

        case BackendType::LOCAL:
            return std::make_shared<LocalLlamaProvider>();

        default:
            break;
    }

    return nullptr;
}
