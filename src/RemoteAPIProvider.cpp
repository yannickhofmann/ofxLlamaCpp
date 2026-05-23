#include "RemoteAPIProvider.h"

namespace {
    const char* const DEFAULT_OPENAI_ENDPOINT = "https://api.openai.com/v1/chat/completions";
    const char* const DEFAULT_MODEL = "gpt-4o-mini";

    bool endsWith(const std::string& value, const std::string& suffix) {
        if (suffix.size() > value.size()) {
            return false;
        }

        return value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    bool startsWith(const std::string& value, const std::string& prefix) {
        if (prefix.size() > value.size()) {
            return false;
        }

        return value.compare(0, prefix.size(), prefix) == 0;
    }

    std::string stripTrailingSlashes(const std::string& value) {
        std::string result = value;
        while (!result.empty() && result[result.size() - 1] == '/') {
            result.erase(result.size() - 1);
        }
        return result;
    }

    std::string trimWhitespace(const std::string& value) {
        const std::string whitespace = " \t\r\n";
        const std::size_t first = value.find_first_not_of(whitespace);
        if (first == std::string::npos) {
            return "";
        }

        const std::size_t last = value.find_last_not_of(whitespace);
        return value.substr(first, last - first + 1);
    }

    std::string buildAuthorizationHeader(const std::string& apiKey) {
        const std::string cleanApiKey = trimWhitespace(apiKey);

        if (cleanApiKey.empty()) {
            return "";
        }

        if (startsWith(cleanApiKey, "Bearer ") || startsWith(cleanApiKey, "Basic ")) {
            return cleanApiKey;
        }

        return "Bearer " + cleanApiKey;
    }

    bool containsText(const std::string& value, const std::string& needle) {
        return value.find(needle) != std::string::npos;
    }
}

RemoteAPIProvider::RemoteAPIProvider()
: endpointUrl(DEFAULT_OPENAI_ENDPOINT)
, model(DEFAULT_MODEL) {
}

RemoteAPIProvider::RemoteAPIProvider(const std::string& endpointUrl)
: endpointUrl(normalizeEndpointUrl(endpointUrl))
, model(DEFAULT_MODEL) {
}

RemoteAPIProvider::RemoteAPIProvider(const std::string& endpointUrl, const std::string& apiKey)
: endpointUrl(normalizeEndpointUrl(endpointUrl))
, apiKey(apiKey)
, model(DEFAULT_MODEL) {
}

bool RemoteAPIProvider::setup(const std::string& modelOrUrl) {
    endpointUrl = isAzureOpenAI() ? normalizeAzureEndpointUrl(modelOrUrl) : normalizeEndpointUrl(modelOrUrl);

    if (endpointUrl.empty()) {
        ofLogError("RemoteAPIProvider") << "Endpoint URL is empty.";
        return false;
    }

    return true;
}

std::string RemoteAPIProvider::generate(const std::string& prompt) {
    if (endpointUrl.empty()) {
        ofLogError("RemoteAPIProvider") << "Provider is not configured. Call setup() first.";
        return "";
    }

    if (model.empty()) {
        ofLogError("RemoteAPIProvider") << "Model name is empty.";
        return "";
    }

    const ofJson body = buildRequestBody(prompt);
    const ofHttpResponse response = performPost(body);

    if (response.status < 200 || response.status >= 300) {
        ofLogError("RemoteAPIProvider")
            << "HTTP error " << response.status << ": " << response.error
            << " Body: " << response.data.getText();
        return "";
    }

    const std::string responseText = response.data.getText();
    if (responseText.empty()) {
        ofLogError("RemoteAPIProvider") << "Received empty response body.";
        return "";
    }

    try {
        const ofJson json = ofJson::parse(responseText);
        return parseResponseContent(json);
    } catch (const std::exception& exception) {
        ofLogError("RemoteAPIProvider") << "Failed to parse JSON response: " << exception.what();
    } catch (...) {
        ofLogError("RemoteAPIProvider") << "Failed to parse JSON response.";
    }

    return "";
}

std::string RemoteAPIProvider::generateChat(const std::vector<RemoteChatMessage>& messages) {
    if (endpointUrl.empty()) {
        ofLogError("RemoteAPIProvider") << "Provider is not configured. Call setup() first.";
        return "";
    }

    if (model.empty()) {
        ofLogError("RemoteAPIProvider") << "Model name is empty.";
        return "";
    }

    if (messages.empty()) {
        ofLogError("RemoteAPIProvider") << "No chat messages provided.";
        return "";
    }

    const ofJson body = buildChatRequestBody(messages);
    const ofHttpResponse response = performPost(body);

    if (response.status < 200 || response.status >= 300) {
        ofLogError("RemoteAPIProvider")
            << "HTTP error " << response.status << ": " << response.error
            << " Body: " << response.data.getText();
        return "";
    }

    const std::string responseText = response.data.getText();
    if (responseText.empty()) {
        ofLogError("RemoteAPIProvider") << "Received empty response body.";
        return "";
    }

    try {
        const ofJson json = ofJson::parse(responseText);
        return parseResponseContent(json);
    } catch (const std::exception& exception) {
        ofLogError("RemoteAPIProvider") << "Failed to parse JSON response: " << exception.what();
    } catch (...) {
        ofLogError("RemoteAPIProvider") << "Failed to parse JSON response.";
    }

    return "";
}

bool RemoteAPIProvider::isRemote() const {
    return true;
}

void RemoteAPIProvider::setApiKey(const std::string& apiKey) {
    this->apiKey = trimWhitespace(apiKey);
}

void RemoteAPIProvider::setApiType(RemoteAPIType apiType) {
    this->apiType = apiType;
}

void RemoteAPIProvider::setApiType(const std::string& apiType) {
    const std::string value = trimWhitespace(apiType);
    if (value == "azure" || value == "azure_openai" || value == "AzureOpenAI") {
        this->apiType = RemoteAPIType::AZURE_OPENAI;
    } else {
        this->apiType = RemoteAPIType::OPENAI_COMPATIBLE;
    }
}

void RemoteAPIProvider::setApiVersion(const std::string& apiVersion) {
    this->apiVersion = trimWhitespace(apiVersion);
}

void RemoteAPIProvider::setModel(const std::string& model) {
    this->model = model;
}

void RemoteAPIProvider::setModelsUrl(const std::string& modelsUrl) {
    this->modelsUrl = stripTrailingSlashes(modelsUrl);
}

void RemoteAPIProvider::setSystemPrompt(const std::string& systemPrompt) {
    this->systemPrompt = systemPrompt;
}

void RemoteAPIProvider::setSystemPromptAsSystemMessage(bool enabled) {
    systemPromptAsSystemMessage = enabled;
}

void RemoteAPIProvider::setExtraBody(const ofJson& extraBody) {
    if (extraBody.is_object()) {
        this->extraBody = extraBody;
    } else {
        this->extraBody = ofJson::object();
    }
}

void RemoteAPIProvider::setStripReasoning(bool enabled) {
    stripReasoning = enabled;
}

std::vector<std::string> RemoteAPIProvider::listModels() const {
    std::vector<std::string> models;

    if (endpointUrl.empty()) {
        ofLogError("RemoteAPIProvider") << "Provider is not configured. Call setup() first.";
        return models;
    }

    if (isAzureOpenAI() && modelsUrl.empty()) {
        if (!model.empty()) {
            models.push_back(model);
        }
        return models;
    }

    const std::string requestUrl = modelsUrl.empty() ? modelsUrlFromEndpointUrl(endpointUrl) : modelsUrl;
    const ofHttpResponse response = performGet(requestUrl);

    if (response.status < 200 || response.status >= 300) {
        ofLogError("RemoteAPIProvider")
            << "HTTP error while loading models " << response.status << ": " << response.error
            << " Body: " << response.data.getText();
        return models;
    }

    const std::string responseText = response.data.getText();
    if (responseText.empty()) {
        ofLogError("RemoteAPIProvider") << "Received empty models response body.";
        return models;
    }

    try {
        const ofJson json = ofJson::parse(responseText);
        if (!json.contains("data") || !json["data"].is_array()) {
            ofLogError("RemoteAPIProvider") << "Models response does not contain a data array.";
            return models;
        }

        for (const auto& item : json["data"]) {
            if (item.contains("id") && item["id"].is_string()) {
                models.push_back(item["id"].get<std::string>());
            }
        }
    } catch (const std::exception& exception) {
        ofLogError("RemoteAPIProvider") << "Failed to parse models response: " << exception.what();
    } catch (...) {
        ofLogError("RemoteAPIProvider") << "Failed to parse models response.";
    }

    return models;
}

ofJson RemoteAPIProvider::buildRequestBody(const std::string& prompt) const {
    ofJson messages = ofJson::array();
    std::string userContent = prompt;

    if (!systemPrompt.empty() && systemPromptAsSystemMessage) {
        ofJson systemMessage;
        systemMessage["role"] = "system";
        systemMessage["content"] = systemPrompt;
        messages.push_back(systemMessage);
    } else if (!systemPrompt.empty()) {
        userContent = systemPrompt + "\n\n" + prompt;
    }

    ofJson userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = userContent;
    messages.push_back(userMessage);

    ofJson body;
    if (!isAzureOpenAI()) {
        body["model"] = model;
    }
    body["messages"] = messages;

    if (extraBody.is_object()) {
        for (ofJson::const_iterator it = extraBody.begin(); it != extraBody.end(); ++it) {
            if (it.key() != "messages" && (isAzureOpenAI() || it.key() != "model")) {
                body[it.key()] = it.value();
            }
        }
    }

    return body;
}

ofJson RemoteAPIProvider::buildChatRequestBody(const std::vector<RemoteChatMessage>& messages) const {
    ofJson requestMessages = ofJson::array();
    bool foldedSystemPrompt = false;

    if (!systemPrompt.empty() && systemPromptAsSystemMessage) {
        ofJson systemMessage;
        systemMessage["role"] = "system";
        systemMessage["content"] = systemPrompt;
        requestMessages.push_back(systemMessage);
    }

    for (const auto& message : messages) {
        if (message.role.empty() || message.content.empty()) {
            continue;
        }

        std::string content = message.content;
        if (!systemPrompt.empty() && !systemPromptAsSystemMessage && !foldedSystemPrompt && message.role == "user") {
            content = systemPrompt + "\n\n" + content;
            foldedSystemPrompt = true;
        }

        ofJson jsonMessage;
        jsonMessage["role"] = message.role;
        jsonMessage["content"] = content;
        requestMessages.push_back(jsonMessage);
    }

    ofJson body;
    if (!isAzureOpenAI()) {
        body["model"] = model;
    }
    body["messages"] = requestMessages;

    if (extraBody.is_object()) {
        for (ofJson::const_iterator it = extraBody.begin(); it != extraBody.end(); ++it) {
            if (it.key() != "messages" && (isAzureOpenAI() || it.key() != "model")) {
                body[it.key()] = it.value();
            }
        }
    }

    return body;
}

std::string RemoteAPIProvider::parseResponseContent(const ofJson& response) const {
    try {
        if (!response.contains("choices") || !response["choices"].is_array() || response["choices"].empty()) {
            ofLogError("RemoteAPIProvider") << "Response does not contain choices.";
            return "";
        }

        const ofJson& choice = response["choices"][0];
        if (!choice.contains("message") || !choice["message"].is_object()) {
            ofLogError("RemoteAPIProvider") << "Response choice does not contain a message.";
            return "";
        }

        const ofJson& message = choice["message"];
        if (!message.contains("content") || !message["content"].is_string()) {
            ofLogError("RemoteAPIProvider") << "Response message does not contain string content.";
            return "";
        }

        const std::string content = message["content"].get<std::string>();
        return stripReasoning ? stripReasoningBlocks(content) : content;
    } catch (const std::exception& exception) {
        ofLogError("RemoteAPIProvider") << "Failed to read response content: " << exception.what();
    } catch (...) {
        ofLogError("RemoteAPIProvider") << "Failed to read response content.";
    }

    return "";
}

std::string RemoteAPIProvider::stripReasoningBlocks(const std::string& text) const {
    std::string result = text;

    const std::vector<std::pair<std::string, std::string>> tags = {
        {"<think>", "</think>"},
        {"<thinking>", "</thinking>"},
        {"<reasoning>", "</reasoning>"}
    };

    for (const auto& tag : tags) {
        std::size_t start = result.find(tag.first);
        while (start != std::string::npos) {
            const std::size_t end = result.find(tag.second, start + tag.first.size());
            if (end == std::string::npos) {
                result.erase(start);
                break;
            }

            result.erase(start, end + tag.second.size() - start);
            start = result.find(tag.first);
        }
    }

    while (!result.empty() && (result[0] == '\n' || result[0] == '\r' || result[0] == ' ' || result[0] == '\t')) {
        result.erase(0, 1);
    }

    return result;
}

ofHttpResponse RemoteAPIProvider::performPost(const ofJson& body) const {
    ofHttpRequest request;
    request.url = getChatCompletionsUrl();
    request.method = ofHttpRequest::POST;
    request.body = body.dump();
    request.contentType = "application/json";

    if (!apiKey.empty()) {
        if (isAzureOpenAI()) {
            request.headers["api-key"] = apiKey;
        } else {
            request.headers["Authorization"] = buildAuthorizationHeader(apiKey);
        }
    }

    ofURLFileLoader loader;
    return loader.handleRequest(request);
}

ofHttpResponse RemoteAPIProvider::performGet(const std::string& url) const {
    ofHttpRequest request;
    request.url = url;
    request.method = ofHttpRequest::GET;
    request.headers["Accept"] = "application/json";

    if (!apiKey.empty()) {
        if (isAzureOpenAI()) {
            request.headers["api-key"] = apiKey;
        } else {
            request.headers["Authorization"] = buildAuthorizationHeader(apiKey);
        }
    }

    ofURLFileLoader loader;
    return loader.handleRequest(request);
}

std::string RemoteAPIProvider::getChatCompletionsUrl() const {
    if (!isAzureOpenAI()) {
        return endpointUrl;
    }

    if (containsText(endpointUrl, "/chat/completions")) {
        if (containsText(endpointUrl, "api-version=") || apiVersion.empty()) {
            return endpointUrl;
        }

        return endpointUrl + (containsText(endpointUrl, "?") ? "&api-version=" : "?api-version=") + apiVersion;
    }

    if (model.empty()) {
        return endpointUrl;
    }

    std::string url = stripTrailingSlashes(endpointUrl);
    url += "/openai/deployments/" + model + "/chat/completions";
    if (!apiVersion.empty()) {
        url += "?api-version=" + apiVersion;
    }
    return url;
}

bool RemoteAPIProvider::isAzureOpenAI() const {
    return apiType == RemoteAPIType::AZURE_OPENAI;
}

std::string RemoteAPIProvider::normalizeEndpointUrl(const std::string& url) {
    const std::string trimmed = stripTrailingSlashes(url);

    if (trimmed.empty()) {
        return "";
    }

    if (endsWith(trimmed, "/v1/chat/completions")) {
        return trimmed;
    }

    if (endsWith(trimmed, "/chat/completions")) {
        return trimmed;
    }

    if (endsWith(trimmed, "/v1")) {
        return trimmed + "/chat/completions";
    }

    return trimmed + "/v1/chat/completions";
}

std::string RemoteAPIProvider::normalizeAzureEndpointUrl(const std::string& url) {
    return stripTrailingSlashes(trimWhitespace(url));
}

std::string RemoteAPIProvider::modelsUrlFromEndpointUrl(const std::string& endpointUrl) {
    const std::string trimmed = stripTrailingSlashes(endpointUrl);

    if (endsWith(trimmed, "/v1/chat/completions")) {
        return trimmed.substr(0, trimmed.size() - std::string("/chat/completions").size()) + "/models";
    }

    if (endsWith(trimmed, "/chat/completions")) {
        return trimmed.substr(0, trimmed.size() - std::string("/chat/completions").size()) + "/models";
    }

    if (endsWith(trimmed, "/v1")) {
        return trimmed + "/models";
    }

    return trimmed + "/v1/models";
}
