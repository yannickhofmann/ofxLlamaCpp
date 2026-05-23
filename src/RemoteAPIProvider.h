#pragma once

#include "IInferenceProvider.h"

#include "ofMain.h"
#include "ofJson.h"

#include <string>
#include <vector>

// Represents a single chat message in the OpenAI-compatible "messages" array.
struct RemoteChatMessage {
    std::string role;
    std::string content;
};

// Supported HTTP API families.
enum class RemoteAPIType {
    OPENAI_COMPATIBLE,
    AZURE_OPENAI
};

// Inference provider that talks to OpenAI-compatible chat completion endpoints.
class RemoteAPIProvider : public IInferenceProvider {
public:
    RemoteAPIProvider();
    explicit RemoteAPIProvider(const std::string& endpointUrl);
    RemoteAPIProvider(const std::string& endpointUrl, const std::string& apiKey);

    // Configures the base endpoint URL used for subsequent requests.
    bool setup(const std::string& modelOrUrl) override;
    // Sends a plain prompt through the chat-completions API and returns the text reply.
    std::string generate(const std::string& prompt) override;
    // Sends a prebuilt chat history instead of a single user prompt.
    std::string generateChat(const std::vector<RemoteChatMessage>& messages);
    bool isRemote() const override;

    // Configuration setters used by the examples before setup()/generate().
    void setApiKey(const std::string& apiKey);
    void setApiType(RemoteAPIType apiType);
    void setApiType(const std::string& apiType);
    void setApiVersion(const std::string& apiVersion);
    void setModel(const std::string& model);
    void setModelsUrl(const std::string& modelsUrl);
    void setSystemPrompt(const std::string& systemPrompt);
    void setSystemPromptAsSystemMessage(bool enabled);
    void setExtraBody(const ofJson& extraBody);
    void setStripReasoning(bool enabled);
    // Queries the endpoint's model listing endpoint when available.
    std::vector<std::string> listModels() const;

private:
    // Connection and request configuration.
    std::string endpointUrl;
    std::string apiKey;
    RemoteAPIType apiType = RemoteAPIType::OPENAI_COMPATIBLE;
    std::string apiVersion = "2024-10-21";
    std::string model;
    std::string modelsUrl;
    std::string systemPrompt;
    bool systemPromptAsSystemMessage = false;
    ofJson extraBody;
    bool stripReasoning = false;

    // Request/response helpers.
    ofJson buildRequestBody(const std::string& prompt) const;
    ofJson buildChatRequestBody(const std::vector<RemoteChatMessage>& messages) const;
    std::string parseResponseContent(const ofJson& response) const;
    std::string stripReasoningBlocks(const std::string& text) const;
    ofHttpResponse performPost(const ofJson& body) const;
    ofHttpResponse performGet(const std::string& url) const;
    std::string getChatCompletionsUrl() const;
    bool isAzureOpenAI() const;

    // URL normalization helpers for the endpoint styles supported by the provider.
    static std::string normalizeEndpointUrl(const std::string& url);
    static std::string normalizeAzureEndpointUrl(const std::string& url);
    static std::string modelsUrlFromEndpointUrl(const std::string& endpointUrl);
};
