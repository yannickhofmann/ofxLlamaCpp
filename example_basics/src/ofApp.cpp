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

#include "ofApp.h"

//--------------------------------------------------------------
ofApp::~ofApp() {
    stopWorker();
}

//--------------------------------------------------------------
void ofApp::setup() {
    // Keep the example visually minimal so the backend switching logic stays
    // easy to inspect while testing local and remote inference.
    ofBackground(0);
    ofSetColor(255);
    ofSetFrameRate(60);

    loadUiFont();
    loadRemoteConfigFromFile();

    prompt.clear();
    setStatus("Press 1 for local llama.cpp, 2 for remote OpenAI-compatible API.");
    output.clear();
}

//--------------------------------------------------------------
void ofApp::update() {
}

//--------------------------------------------------------------
void ofApp::draw() {
    const float margin = 20.0f;
    const float textWidth = ofGetWidth() - margin * 2.0f;

    // Show the currently active backend and model first so switching feedback is immediate.
    ofSetColor(255);
    drawText("Backend: " + getBackendName(), margin, 30);
    float outputY = 130.0f;
    std::string modelLabel = "Model: n/a";

    if (backendType == BackendType::LOCAL && !localModels.empty()) {
        modelLabel = "Model: " + ofFilePath::getBaseName(localModels[selectedLocalModelIndex]);
    } else if (remoteProvider && !remoteModels.empty()) {
        modelLabel = "Model: " + remoteModels[selectedModelIndex];
    }

    drawText(modelLabel, margin, 55);

    if (provider) {
        // In remote mode the user can edit the system prompt directly in the example window.
        drawTextField("Systemprompt", remoteSystemPrompt, InputField::SYSTEM_PROMPT, 95.0f);
        drawTextField("Prompt", prompt, InputField::PROMPT, 125.0f);
        ofSetColor(170);
        if (backendType == BackendType::REMOTE) {
            drawText("Config is loaded from data/models/remote_api_config.json.", margin, 155);
            drawText("Hover over the model name to inspect endpoint details.", margin, 180);
        } else {
            drawText("Local models are loaded from data/models.", margin, 155);
        }
        outputY = 220.0f;
    } else {
        drawText("Prompt: " + prompt, margin, 80);
    }

    std::string clean = getOutput();
    if (startsWith(clean, "Assistant:")) {
        clean = clean.substr(10);
    }

    ofSetColor(220);
    drawText(wrapString(clean, static_cast<int>(textWidth)), margin, outputY);

    ofSetColor(170);
    drawText("1 local | 2 remote | shift+left/right model | space regenerate | esc quit", margin, ofGetHeight() - 55);
    drawText(getStatus(), margin, ofGetHeight() - 30);

    drawRemoteConfigOverlay();
}

//--------------------------------------------------------------
void ofApp::exit() {
    stopWorker();
}

//--------------------------------------------------------------
std::string ofApp::wrapString(std::string text, int width) {
    std::string wrapped;
    std::string line;

    std::replace(text.begin(), text.end(), '\n', ' ');

    const std::vector<std::string> words = ofSplitString(text, " ", true, true);

    for (const auto &word : words) {
        const std::string test = line.empty() ? word : line + " " + word;

        if (textWidth(test) > width) {
            wrapped += line + "\n";
            line = word;
        } else {
            line = test;
        }
    }

    wrapped += line;
    return wrapped;
}

//--------------------------------------------------------------
bool ofApp::startsWith(const std::string &text, const std::string &prefix) {
    return text.compare(0, prefix.size(), prefix) == 0;
}

//--------------------------------------------------------------
std::string ofApp::maskSecret(const std::string &value) const {
    if (value.empty()) {
        return "(empty)";
    }

    if (value.size() <= 8) {
        return std::string(value.size(), '*');
    }

    return value.substr(0, 4) + "..." + value.substr(value.size() - 4);
}

//--------------------------------------------------------------
void ofApp::drawText(const std::string &text, float x, float y) {
    if (uiFontLoaded) {
        uiFont.drawString(text, x, y);
    } else {
        ofDrawBitmapString(text, x, y);
    }
}

//--------------------------------------------------------------
float ofApp::textWidth(const std::string &text) const {
    if (uiFontLoaded) {
        return uiFont.getStringBoundingBox(text, 0, 0).width;
    }

    ofBitmapFont font;
    return font.getBoundingBox(text, 0, 0).width;
}

//--------------------------------------------------------------
ofRectangle ofApp::getModelLabelBounds() const {
    std::string label = "Model: n/a";

    if (backendType == BackendType::LOCAL && !localModels.empty()) {
        label = "Model: " + ofFilePath::getBaseName(localModels[selectedLocalModelIndex]);
    } else if (remoteProvider && !remoteModels.empty()) {
        label = "Model: " + remoteModels[selectedModelIndex];
    }

    return ofRectangle(20.0f, 38.0f, textWidth(label) + 12.0f, 24.0f);
}

//--------------------------------------------------------------
void ofApp::drawRemoteConfigOverlay() {
    if (backendType != BackendType::REMOTE || !provider) {
        return;
    }

    if (!getModelLabelBounds().inside(static_cast<float>(ofGetMouseX()), static_cast<float>(ofGetMouseY()))) {
        return;
    }

    const float x = 20.0f;
    const float y = 70.0f;
    const float w = std::min(820.0f, static_cast<float>(ofGetWidth()) - 40.0f);
    const float h = 124.0f;

    ofPushStyle();
    ofSetColor(20, 28, 36, 235);
    ofDrawRectangle(x, y, w, h);
    ofNoFill();
    ofSetColor(90, 180, 220);
    ofDrawRectangle(x, y, w, h);
    ofFill();

    ofSetColor(210, 235, 245);
    drawText("API type: " + remoteApiType, x + 12.0f, y + 24.0f);
    drawText("API endpoint: " + remoteEndpoint, x + 12.0f, y + 48.0f);
    drawText("Models URL: " + (remoteModelsUrl.empty() ? "(from deployment)" : remoteModelsUrl), x + 12.0f, y + 72.0f);
    drawText("API key: " + maskSecret(remoteApiKey), x + 12.0f, y + 96.0f);
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::loadUiFont() {
    const std::string fontPath = ofToDataPath("fonts/verdana.ttf");
    uiFontLoaded = uiFont.load(fontPath, 13, true, true);

    if (!uiFontLoaded) {
        ofLogWarning("example_basics") << "Failed to load font: " << fontPath;
    }
}

//--------------------------------------------------------------
void ofApp::loadRemoteConfigFromFile() {
    std::string configPath = ofToDataPath("models/remote_api_config.json");

    if (!ofFile::doesFileExist(configPath)) {
        configPath = ofToDataPath("remote_api_config.json");
    }

    if (!ofFile::doesFileExist(configPath)) {
        ofLogNotice("example_basics") << "Remote config file not found in data/models or data.";
        return;
    }

    try {
        const ofJson config = ofLoadJson(configPath);

        if (config.contains("api_type") && config["api_type"].is_string()) {
            remoteApiType = config["api_type"].get<std::string>();
        }

        if (config.contains("api_endpoint") && config["api_endpoint"].is_string()) {
            remoteEndpoint = config["api_endpoint"].get<std::string>();
        }

        if (config.contains("api_key") && config["api_key"].is_string()) {
            remoteApiKey = config["api_key"].get<std::string>();
        }

        if (config.contains("api_version") && config["api_version"].is_string()) {
            remoteApiVersion = config["api_version"].get<std::string>();
        }

        if (config.contains("models_url") && config["models_url"].is_string()) {
            remoteModelsUrl = config["models_url"].get<std::string>();
        }

        if (config.contains("system_prompt") && config["system_prompt"].is_string()) {
            remoteSystemPrompt = config["system_prompt"].get<std::string>();
        }

        if (config.contains("system_prompt_mode") && config["system_prompt_mode"].is_string()) {
            remoteSystemPromptAsSystemMessage = config["system_prompt_mode"].get<std::string>() == "system";
        }

        if (config.contains("preferred_model") && config["preferred_model"].is_string()) {
            preferredRemoteModel = config["preferred_model"].get<std::string>();
        }

        if (config.contains("extra_body") && config["extra_body"].is_object()) {
            remoteExtraBody = config["extra_body"];
        }

        if (config.contains("strip_reasoning") && config["strip_reasoning"].is_boolean()) {
            remoteStripReasoning = config["strip_reasoning"].get<bool>();
        }

        if ((remoteApiType == "azure" || remoteApiType == "azure_openai") && !config.contains("models_url")) {
            remoteModelsUrl.clear();
        }

        ofLogNotice("example_basics") << "Loaded remote config: " << configPath;
    } catch (const std::exception &exception) {
        ofLogError("example_basics") << "Failed to load remote config: " << exception.what();
    } catch (...) {
        ofLogError("example_basics") << "Failed to load remote config.";
    }
}

//--------------------------------------------------------------
void ofApp::loadLocalModelList() {
    localModels.clear();
    selectedLocalModelIndex = 0;

    const std::string modelsDirPath = ofToDataPath("models");
    ofDirectory modelsDir(modelsDirPath);
    modelsDir.allowExt("gguf");
    modelsDir.listDir();
    modelsDir.sort();

    if (modelsDir.size() == 0) {
        return;
    }

    for (std::size_t i = 0; i < modelsDir.size(); ++i) {
        localModels.push_back(modelsDir.getPath(i));
    }
}

//--------------------------------------------------------------
void ofApp::selectBackend(BackendType type) {
    if (generating) {
        setStatus("Generation is running. Wait until it finishes before switching backend.");
        return;
    }

    stopWorker();

    backendType = type;
    providerReady = false;
    provider = BackendSelector::create(type);
    remoteProvider.reset();
    remoteModels.clear();
    selectedModelIndex = 0;
    setOutput("");

    if (prompt.empty()) {
        prompt = "What is openFrameworks?";
    }

    if (!provider) {
        setStatus("Failed to create backend provider.");
        return;
    }

    if (type == BackendType::LOCAL) {
        // Local mode scans bin/data/models for GGUF files and loads one directly.
        loadLocalModelList();

        if (localModels.empty()) {
            setStatus("No local .gguf model found in data/models.");
            return;
        }

        loadSelectedLocalModel();
    } else {
        // Remote mode configures the HTTP provider and then loads the model list from the endpoint.
        remoteProvider = std::dynamic_pointer_cast<RemoteAPIProvider>(provider);
        applyRemoteConfig();
    }
}

//--------------------------------------------------------------
void ofApp::selectLocalModel(int direction) {
    if (backendType != BackendType::LOCAL || !provider) {
        setStatus("Local backend is not selected.");
        return;
    }

    if (localModels.empty()) {
        loadLocalModelList();
    }

    if (localModels.empty()) {
        setStatus("No local .gguf model found in data/models.");
        return;
    }

    if (generating) {
        setStatus("Generation is running. Wait until it finishes before switching model.");
        return;
    }

    const int count = static_cast<int>(localModels.size());
    int next = static_cast<int>(selectedLocalModelIndex) + direction;

    if (next < 0) {
        next = count - 1;
    } else if (next >= count) {
        next = 0;
    }

    selectedLocalModelIndex = static_cast<std::size_t>(next);
    loadSelectedLocalModel();
}

//--------------------------------------------------------------
void ofApp::selectRemoteModel(int direction) {
    if (!remoteProvider || remoteModels.empty()) {
        setStatus("No remote models loaded. Press 2 to load the remote backend.");
        return;
    }

    if (generating) {
        setStatus("Generation is running. Wait until it finishes before switching model.");
        return;
    }

    const int count = static_cast<int>(remoteModels.size());
    int next = static_cast<int>(selectedModelIndex) + direction;

    if (next < 0) {
        next = count - 1;
    } else if (next >= count) {
        next = 0;
    }

    selectedModelIndex = static_cast<std::size_t>(next);
    applySelectedRemoteModel();
}

//--------------------------------------------------------------
void ofApp::selectModel(int direction) {
    if (backendType == BackendType::LOCAL) {
        selectLocalModel(direction);
    } else {
        selectRemoteModel(direction);
    }
}

//--------------------------------------------------------------
bool ofApp::loadSelectedLocalModel() {
    if (!provider || localModels.empty()) {
        return false;
    }

    const std::string modelPath = localModels[selectedLocalModelIndex];
    const std::string modelName = ofFilePath::getBaseName(modelPath);

    providerReady = false;
    setOutput("");
    setStatus("Loading local model: " + modelName);

    providerReady = provider->setup(modelPath);
    setStatus(providerReady ? "Local backend ready." : "Local backend failed to load model.");
    return providerReady;
}

//--------------------------------------------------------------
void ofApp::applySelectedRemoteModel() {
    if (!remoteProvider || remoteModels.empty()) {
        return;
    }

    remoteProvider->setModel(remoteModels[selectedModelIndex]);
}

//--------------------------------------------------------------
void ofApp::applyRemoteConfig() {
    if (!provider || !remoteProvider) {
        setStatus("Remote backend is not selected.");
        return;
    }

    providerReady = false;
    remoteModels.clear();
    selectedModelIndex = 0;

    remoteProvider->setApiKey(remoteApiKey);
    remoteProvider->setApiType(remoteApiType);
    remoteProvider->setApiVersion(remoteApiVersion);
    remoteProvider->setModelsUrl(remoteModelsUrl);
    remoteProvider->setModel(preferredRemoteModel);
    remoteProvider->setSystemPrompt(remoteSystemPrompt);
    remoteProvider->setSystemPromptAsSystemMessage(remoteSystemPromptAsSystemMessage);
    remoteProvider->setExtraBody(remoteExtraBody);
    remoteProvider->setStripReasoning(remoteStripReasoning);

    providerReady = provider->setup(remoteEndpoint);
    if (!providerReady) {
        setStatus("Remote backend setup failed.");
        return;
    }

    if (remoteApiKey.empty()) {
        setStatus("Remote backend ready. Enter API key and press Enter to load models.");
        return;
    }

    remoteModels = remoteProvider->listModels();
    if (!remoteModels.empty()) {
        // Prefer the configured default when the endpoint exposes more than one model.
        for (std::size_t i = 0; i < remoteModels.size(); ++i) {
            if (remoteModels[i] == preferredRemoteModel) {
                selectedModelIndex = i;
                break;
            }
        }

        applySelectedRemoteModel();
        setStatus("Remote backend ready.");
    } else {
        remoteProvider->setModel(preferredRemoteModel);
        setStatus("Remote backend ready, but no models were loaded.");
    }
}

//--------------------------------------------------------------
void ofApp::startPrompt() {
    if (!provider || !providerReady) {
        setStatus("Select a backend first.");
        return;
    }

    if (provider->isRemote() && remoteApiKey.empty()) {
        setStatus("Enter API key and press Enter before generating.");
        return;
    }

    if (generating) {
        setStatus("Generation is already running.");
        return;
    }

    stopWorker();
    setOutput("");
    if (remoteProvider) {
        remoteProvider->setSystemPrompt(remoteSystemPrompt);
    }
    // Generation is executed on a worker thread so the OF window stays responsive.
    setStatus("Generating with " + getBackendName() + "...");
    generating = true;

    worker = std::thread([this]() {
        std::string requestPrompt = prompt;
        if (!provider->isRemote() && !remoteSystemPrompt.empty()) {
            requestPrompt = remoteSystemPrompt + "\n\n" + prompt;
        }

        const std::string result = provider->generate(requestPrompt);
        setOutput(result);
        setStatus(result.empty() ? "Generation finished with empty output." : "Generation finished.");
        generating = false;
    });
}

//--------------------------------------------------------------
void ofApp::stopWorker() {
    if (worker.joinable()) {
        worker.join();
    }

    generating = false;
}

//--------------------------------------------------------------
void ofApp::drawTextField(const std::string &label, const std::string &value, InputField field, float y, bool secret) {
    const float labelX = 20.0f;
    const float boxX = 160.0f;
    const float boxW = ofGetWidth() - boxX - 20.0f;
    const float boxH = 22.0f;

    ofSetColor(190);
    drawText(label, labelX, y);

    ofNoFill();
    ofSetColor(activeInputField == field ? 255 : 120);
    ofDrawRectangle(boxX, y - 15.0f, boxW, boxH);
    ofFill();

    std::string display = secret && !value.empty() ? std::string(value.size(), '*') : value;
    if (activeInputField == field) {
        display += "_";
    }

    ofSetColor(230);
    drawText(wrapString(display, static_cast<int>(boxW - 10.0f)), boxX + 5.0f, y);
}

//--------------------------------------------------------------
bool ofApp::isFieldAt(InputField field, int x, int y) const {
    float fieldY = 0.0f;
    switch (field) {
        case InputField::SYSTEM_PROMPT:
            fieldY = 95.0f;
            break;
        case InputField::PROMPT:
            fieldY = 125.0f;
            break;
        case InputField::NONE:
        default:
            return false;
    }

    const float boxX = 160.0f;
    const float boxW = ofGetWidth() - boxX - 20.0f;
    return ofRectangle(boxX, fieldY - 15.0f, boxW, 22.0f).inside(static_cast<float>(x), static_cast<float>(y));
}

//--------------------------------------------------------------
void ofApp::handleTextInput(const ofKeyEventArgs &args) {
    std::string value = getInputValue(activeInputField);

    if (args.key == OF_KEY_BACKSPACE || args.key == OF_KEY_DEL) {
        if (!value.empty()) {
            eraseLastUtf8Codepoint(value);
        }
    } else if (args.key == OF_KEY_RETURN) {
        if (activeInputField == InputField::SYSTEM_PROMPT && remoteProvider) {
            remoteProvider->setSystemPrompt(remoteSystemPrompt);
        }
        activeInputField = InputField::NONE;
    } else if (args.codepoint >= 32 && args.codepoint != 127) {
        value += codepointToUtf8(args.codepoint);
    }

    setInputValue(activeInputField, value);
}

//--------------------------------------------------------------
std::string ofApp::codepointToUtf8(uint32_t codepoint) const {
    std::string result;

    if (codepoint <= 0x7F) {
        result.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        result.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        result.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
        result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0x10FFFF) {
        result.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
        result.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }

    return result;
}

//--------------------------------------------------------------
void ofApp::eraseLastUtf8Codepoint(std::string &value) const {
    if (value.empty()) {
        return;
    }

    std::size_t pos = value.size() - 1;
    while (pos > 0 && (static_cast<unsigned char>(value[pos]) & 0xC0) == 0x80) {
        --pos;
    }

    value.erase(pos);
}

//--------------------------------------------------------------
std::string ofApp::getInputValue(InputField field) const {
    switch (field) {
        case InputField::SYSTEM_PROMPT:
            return remoteSystemPrompt;
        case InputField::PROMPT:
            return prompt;
        case InputField::NONE:
        default:
            return "";
    }
}

//--------------------------------------------------------------
void ofApp::setInputValue(InputField field, const std::string &value) {
    switch (field) {
        case InputField::SYSTEM_PROMPT:
            remoteSystemPrompt = value;
            break;
        case InputField::PROMPT:
            prompt = value;
            break;
        case InputField::NONE:
        default:
            break;
    }
}

//--------------------------------------------------------------
void ofApp::setOutput(const std::string &text) {
    std::lock_guard<std::mutex> lock(stateMutex);
    output = text;
}

//--------------------------------------------------------------
std::string ofApp::getOutput() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return output;
}

//--------------------------------------------------------------
void ofApp::setStatus(const std::string &text) {
    std::lock_guard<std::mutex> lock(stateMutex);
    status = text;
}

//--------------------------------------------------------------
std::string ofApp::getStatus() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return status;
}

//--------------------------------------------------------------
std::string ofApp::getBackendName() const {
    if (!provider) {
        return "none";
    }

    return provider->isRemote() ? "remote" : "local";
}

//--------------------------------------------------------------
void ofApp::keyPressed(ofKeyEventArgs &args) {
    if (activeInputField != InputField::NONE) {
        if (args.key == OF_KEY_ESC) {
            activeInputField = InputField::NONE;
        } else {
            handleTextInput(args);
        }
        return;
    }

    if (args.key == OF_KEY_ESC) {
        ofExit();
    } else if (args.key == '1') {
        selectBackend(BackendType::LOCAL);
    } else if (args.key == '2') {
        selectBackend(BackendType::REMOTE);
    } else if (ofGetKeyPressed(OF_KEY_SHIFT) && args.key == OF_KEY_LEFT) {
        selectModel(-1);
    } else if (ofGetKeyPressed(OF_KEY_SHIFT) && args.key == OF_KEY_RIGHT) {
        selectModel(1);
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(ofKeyEventArgs &args) {
    if (activeInputField != InputField::NONE) {
        return;
    }

    if (args.key == ' ') {
        startPrompt();
    }
}

//--------------------------------------------------------------
void ofApp::mousePressed(ofMouseEventArgs &args) {
    if (!provider) {
        activeInputField = InputField::NONE;
        return;
    }

    if (isFieldAt(InputField::SYSTEM_PROMPT, args.x, args.y)) {
        activeInputField = InputField::SYSTEM_PROMPT;
    } else if (isFieldAt(InputField::PROMPT, args.x, args.y)) {
        activeInputField = InputField::PROMPT;
    } else {
        activeInputField = InputField::NONE;
    }
}
