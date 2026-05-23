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
#include "ofxGui.h"
#include <algorithm> // For std::min/max

// --- HELPER FUNCTIONS ---
namespace {
const std::string kNoModelFound = "NO MODEL FOUND";
const std::string kNoRemoteModels = "NO REMOTE MODELS";

void configureSingleChoiceDropdown(const std::shared_ptr<ofxDropdown>& dropdown,
                                   const std::vector<std::string>& options,
                                   const std::string& selectedOption) {
    if (!dropdown) {
        return;
    }

    dropdown->clear();
    dropdown->disableMultipleSelection();
    dropdown->enableCollapseOnSelection();
    dropdown->add(options);

    if (!options.empty()) {
        const auto it = std::find(options.begin(), options.end(), selectedOption);
        dropdown->setSelectedValueByName(it != options.end() ? *it : options.front(), false);
    }
}

float getStringWidth(const std::string& value) {
    return static_cast<float>(value.size()) * 8.0f;
}
}

// Removes a single UTF-8 character from the end of a string.
// param s The string to modify.
static void utf8_pop_back(std::string &s) {
    if (s.empty()) return;
    int i = s.size() - 1;
    // Backtrack until the start of a multi-byte character is found.
    while (i > 0 && ((unsigned char)s[i] & 0xC0) == 0x80) i--;
    s.erase(i);
}

//--------------------------------------------------------------
void ofApp::exit() {
    stopRemoteWorker();
}

//--------------------------------------------------------------
void ofApp::setup() {
    // Basic openFrameworks setup
    ofBackground(0);
    ofSetColor(255);
    ofSetFrameRate(60);
    
    // Initialize the chat UI
    mChatUI.setup("fonts/verdana.ttf", 12);
    
    // Define the default system prompt for the AI
    system_prompt =
        "You are an extremely efficient and helpful assistant. Always respond with precision, directly, and straight to the point, avoiding any unnecessary fluff or filler text.";
    loadRemoteConfigFromFile();

    // --- GUI Setup ---
    gui.setup("LLM Control");
    stopButton.setup("Stop Generation");
    stopButton.addListener(this, &ofApp::stopGeneration);
    stopButton.unregisterMouseEvents();

    // --- Dropdown Menus ---
    backendDropdown = std::make_shared<ofxDropdown>("Backend");
    backendDropdown->add(vector<string>{"Local", "Remote"});
    backendDropdown->disableMultipleSelection();
    backendDropdown->enableCollapseOnSelection();
    backendDropdown->addListener(this, &ofApp::onBackendChange);
    backendDropdown->unregisterMouseEvents();

    modelDropdown = std::make_shared<ofxDropdown>("Model");
    modelDropdown->disableMultipleSelection();
    modelDropdown->enableCollapseOnSelection();
    modelDropdown->addListener(this, &ofApp::onModelChange);
    modelDropdown->unregisterMouseEvents();

    templateDropdown = std::make_shared<ofxDropdown>("Template");
    templateDropdown->add(mTemplateManager.getTemplateNames());
    templateDropdown->disableMultipleSelection();
    templateDropdown->enableCollapseOnSelection(); 
    templateDropdown->addListener(this, &ofApp::onTemplateChange);
    templateDropdown->unregisterMouseEvents();

    // Add GPU status label to the GUI
    gpuStatusLabel.setup("GPU Layers", "N/A"); // Initialize with N/A, assuming ofxLabel adds a colon
    modelInfoLabel.setup("Model", "n/a");
    gui.setPosition(20, 20);


    // --- Default Selections ---
    backendDropdown->selectedValue = "Local";
    populateLocalModels();
    rebuildGuiForBackend();

    // Set a default template to ensure the application starts in a valid state
    string t = mTemplateManager.getTemplateNames().front();
    templateDropdown->selectedValue = t;
    onTemplateChange(t);

    // Set a default model
    if (modelDropdown->getNumOptions() > 0 && modelDropdown->selectedValue.get() != kNoModelFound) {
        string m = modelDropdown->selectedValue.get();
        modelDropdown->selectedValue = m;
        onModelChange(m);
    } else {
        string m = kNoModelFound;
        if(modelDropdown->getNumOptions() > 0) m = modelDropdown->selectedValue.get();
        onModelChange(m);
    }
}

//--------------------------------------------------------------
void ofApp::rebuildGuiForBackend() {
    gui.clear();
    gui.setPosition(20, 20);

    const float spacing = 4.0f;
    const float selectorHeight = 36.0f;
    float currentY = gui.getPosition().y + gui.getHeight() + spacing;

    backendSelectorRect.set(gui.getPosition().x, currentY, gui.getWidth(), selectorHeight);
    currentY += selectorHeight + spacing;

    modelSelectorRect.set(gui.getPosition().x, currentY, gui.getWidth(), selectorHeight);
    currentY += selectorHeight + spacing;

    if (backend == ChatBackend::LOCAL) {
        templateSelectorRect.set(gui.getPosition().x, currentY, gui.getWidth(), selectorHeight);
        currentY += selectorHeight + spacing;
    } else {
        templateSelectorRect.set(0, 0, 0, 0);
    }

    stopButton.setPosition(gui.getPosition().x, currentY);
    currentY += stopButton.getHeight() + spacing;

    gpuStatusLabel.setPosition(gui.getPosition().x, currentY);

    guiFixedX = gui.getPosition().x;
    guiFixedWidth = std::max({
        gui.getWidth(),
        backendSelectorRect.getWidth(),
        modelSelectorRect.getWidth(),
        templateSelectorRect.getWidth(),
        stopButton.getWidth(),
        gpuStatusLabel.getWidth(),
        modelInfoLabel.getWidth()
    });
}

//--------------------------------------------------------------
void ofApp::drawSelector(const std::string& label, const std::string& value, const ofRectangle& rect, bool open) const {
    ofPushStyle();
    ofSetColor(160);
    ofDrawRectangle(rect);

    ofNoFill();
    ofSetColor(open ? 255 : 180);
    ofDrawRectangle(rect);
    ofFill();

    ofSetColor(255);
    ofDrawBitmapString(label, rect.x + 6.0f, rect.y + 13.0f);
    ofDrawBitmapString(fitSelectorText(value, rect.getWidth() - 26.0f), rect.x + 6.0f, rect.y + rect.height - 8.0f);
    ofDrawBitmapString(">", rect.getRight() - 14.0f, rect.y + rect.height * 0.5f + 4.0f);
    ofPopStyle();
}

//--------------------------------------------------------------
float ofApp::getSelectorMenuWidth(const std::vector<std::string>& options, float minWidth) const {
    float width = minWidth;
    for (const auto& option : options) {
        width = std::max(width, getStringWidth(option) + 18.0f);
    }
    return width;
}

//--------------------------------------------------------------
std::string ofApp::fitSelectorText(const std::string& value, float maxWidth) const {
    if (maxWidth <= 0.0f || getStringWidth(value) <= maxWidth) {
        return value;
    }

    const std::string ellipsis = "...";
    if (getStringWidth(ellipsis) > maxWidth) {
        return "";
    }

    std::size_t prefixLength = value.size() / 2;
    std::size_t suffixLength = value.size() - prefixLength;
    std::string fitted = value.substr(0, prefixLength) + ellipsis + value.substr(value.size() - suffixLength);

    while (getStringWidth(fitted) > maxWidth && (prefixLength > 0 || suffixLength > 0)) {
        if (prefixLength >= suffixLength && prefixLength > 0) {
            --prefixLength;
        } else if (suffixLength > 0) {
            --suffixLength;
        }

        fitted = value.substr(0, prefixLength) + ellipsis + value.substr(value.size() - suffixLength);
    }

    return fitted;
}

//--------------------------------------------------------------
void ofApp::drawSelectorMenu(const std::vector<std::string>& options, const std::string& selectedValue, const ofRectangle& anchorRect) const {
    if (options.empty()) {
        return;
    }

    const float rowHeight = 19.0f;
    const float menuWidth = getSelectorMenuWidth(options, anchorRect.getWidth());
    ofRectangle menuRect(anchorRect.getRight(), anchorRect.getTop(), menuWidth, rowHeight * static_cast<float>(options.size()));

    ofPushStyle();
    ofSetColor(0);
    ofDrawRectangle(menuRect);
    ofNoFill();
    ofSetColor(200);
    ofDrawRectangle(menuRect);

    for (std::size_t i = 0; i < options.size(); ++i) {
        const ofRectangle row(menuRect.x, menuRect.y + rowHeight * static_cast<float>(i), menuRect.width, rowHeight);
        const bool selected = options[i] == selectedValue;

        if (selected) {
            ofFill();
            ofSetColor(100);
            ofDrawRectangle(row);
            ofNoFill();
            ofSetColor(200);
            ofDrawRectangle(row);
        } else {
            ofSetColor(120);
            ofDrawLine(row.getTopLeft(), row.getTopRight());
        }

        ofSetColor(255);
        ofDrawBitmapString(fitSelectorText(options[i], row.getWidth() - 16.0f), row.x + 8.0f, row.y + 13.0f);
    }
    ofPopStyle();
}

//--------------------------------------------------------------
std::vector<std::string> ofApp::getOptionsForSelector(OpenSelector selector) const {
    switch (selector) {
        case OpenSelector::BACKEND:
            return backendDropdown ? backendDropdown->getOptions() : std::vector<std::string>{};
        case OpenSelector::MODEL:
            return modelDropdown ? modelDropdown->getOptions() : std::vector<std::string>{};
        case OpenSelector::TEMPLATE:
            return templateDropdown ? templateDropdown->getOptions() : std::vector<std::string>{};
        case OpenSelector::NONE:
        default:
            return {};
    }
}

//--------------------------------------------------------------
std::string ofApp::getSelectedValueForSelector(OpenSelector selector) const {
    switch (selector) {
        case OpenSelector::BACKEND:
            return backendDropdown ? backendDropdown->selectedValue.get() : "";
        case OpenSelector::MODEL:
            return modelDropdown ? modelDropdown->selectedValue.get() : "";
        case OpenSelector::TEMPLATE:
            return templateDropdown ? templateDropdown->selectedValue.get() : "";
        case OpenSelector::NONE:
        default:
            return "";
    }
}

//--------------------------------------------------------------
void ofApp::selectOption(OpenSelector selector, const std::string& value) {
    switch (selector) {
        case OpenSelector::BACKEND:
            if (backendDropdown) {
                backendDropdown->selectedValue = value;
            }
            break;
        case OpenSelector::MODEL:
            if (modelDropdown) {
                modelDropdown->selectedValue = value;
            }
            break;
        case OpenSelector::TEMPLATE:
            if (templateDropdown) {
                templateDropdown->selectedValue = value;
            }
            break;
        case OpenSelector::NONE:
        default:
            break;
    }
}

//--------------------------------------------------------------
bool ofApp::isAzureRemote() const {
    return remoteApiType == "azure" || remoteApiType == "azure_openai";
}

//--------------------------------------------------------------
void ofApp::loadRemoteConfigFromFile() {
    std::string configPath = ofToDataPath("models/remote_api_config.json");

    if (!ofFile::doesFileExist(configPath)) {
        configPath = ofToDataPath("remote_api_config.json");
    }

    if (!ofFile::doesFileExist(configPath)) {
        ofLogNotice("example_chat") << "Remote config file not found in data/models or data.";
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
        if (config.contains("preferred_model") && config["preferred_model"].is_string()) {
            preferredRemoteModel = config["preferred_model"].get<std::string>();
        }
        if (config.contains("system_prompt") && config["system_prompt"].is_string()) {
            system_prompt = config["system_prompt"].get<std::string>();
        }
        if (config.contains("system_prompt_mode") && config["system_prompt_mode"].is_string()) {
            remoteSystemPromptAsSystemMessage = config["system_prompt_mode"].get<std::string>() == "system";
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

        ofLogNotice("example_chat") << "Loaded remote config: " << configPath;
    } catch (const std::exception& e) {
        ofLogError("example_chat") << "Failed to load remote config: " << e.what();
    } catch (...) {
        ofLogError("example_chat") << "Failed to load remote config.";
    }
}

//--------------------------------------------------------------
void ofApp::populateLocalModels() {
    ofDirectory dir(ofToDataPath("models"));
    dir.allowExt("gguf");
    dir.listDir();
    dir.sort();

    vector<string> modelDisplayNames;
    displayNameToFullFileName.clear();
    int max_len = 20;

    for (auto &f : dir.getFiles()) {
        string filename = f.getFileName();
        string displayName = filename;
        if (filename.length() > max_len) {
            displayName = filename.substr(0, max_len - 3) + "...";
        }
        modelDisplayNames.push_back(displayName);
        displayNameToFullFileName[displayName] = filename;
    }

    if (modelDisplayNames.empty()) {
        modelDisplayNames.push_back(kNoModelFound);
    }

    configureSingleChoiceDropdown(modelDropdown, modelDisplayNames, modelDisplayNames.front());
}

//--------------------------------------------------------------
void ofApp::populateRemoteModels() {
    displayNameToFullFileName.clear();

    remoteProvider = std::make_shared<RemoteAPIProvider>();
    applyRemoteConfig();

    vector<string> remoteModelNames = remoteProvider->listModels();
    vector<string> uniqueRemoteModelNames;
    uniqueRemoteModelNames.reserve(remoteModelNames.size());
    for (const auto& modelName : remoteModelNames) {
        if (std::find(uniqueRemoteModelNames.begin(), uniqueRemoteModelNames.end(), modelName) == uniqueRemoteModelNames.end()) {
            uniqueRemoteModelNames.push_back(modelName);
        }
    }
    remoteModelNames = std::move(uniqueRemoteModelNames);

    if (remoteModelNames.empty()) {
        remoteModelNames.push_back(kNoRemoteModels);
    }

    string selected = remoteModelNames.front();
    for (const auto& modelName : remoteModelNames) {
        if (modelName == preferredRemoteModel) {
            selected = modelName;
            break;
        }
    }

    configureSingleChoiceDropdown(modelDropdown, remoteModelNames, selected);
    selectedRemoteModel = selected;
    if (selectedRemoteModel != kNoRemoteModels) {
        remoteProvider->setModel(selectedRemoteModel);
    }
    modelInfoLabel.setup(isAzureRemote() ? "Deployment" : "Model", selectedRemoteModel);
}

//--------------------------------------------------------------
void ofApp::applyRemoteConfig() {
    if (!remoteProvider) {
        return;
    }

    remoteProvider->setApiKey(remoteApiKey);
    remoteProvider->setApiType(remoteApiType);
    remoteProvider->setApiVersion(remoteApiVersion);
    remoteProvider->setModelsUrl(remoteModelsUrl);
    const std::string remoteModel =
        (!selectedRemoteModel.empty() && selectedRemoteModel != kNoRemoteModels)
            ? selectedRemoteModel
            : preferredRemoteModel;
    remoteProvider->setModel(remoteModel);
    remoteProvider->setSystemPrompt(system_prompt);
    remoteProvider->setSystemPromptAsSystemMessage(remoteSystemPromptAsSystemMessage);
    remoteProvider->setExtraBody(remoteExtraBody);
    remoteProvider->setStripReasoning(remoteStripReasoning);
    remoteProvider->setup(remoteEndpoint);
}

//--------------------------------------------------------------
void ofApp::onBackendChange(string &displayName) {
    ready = false;
    stopGeneration();
    chatHistory.clear();
    conversationSummary.clear();
    input.clear();
    currentState = CHATTING;

    if (displayName == "Remote") {
        backend = ChatBackend::REMOTE;
        populateRemoteModels();
        ready = remoteProvider != nullptr && !remoteApiKey.empty() && selectedRemoteModel != kNoRemoteModels;
        gpuStatusLabel.setup("GPU Layers", "Remote");
        rebuildGuiForBackend();
    } else {
        backend = ChatBackend::LOCAL;
        remoteProvider.reset();
        populateLocalModels();
        rebuildGuiForBackend();

        if (modelDropdown->getNumOptions() > 0) {
            string m = modelDropdown->selectedValue.get();
            onModelChange(m);
        }
    }
}

//--------------------------------------------------------------
void ofApp::onModelChange(string &displayName) {
    if (backend == ChatBackend::REMOTE) {
        selectedRemoteModel = displayName;
        if (remoteProvider && selectedRemoteModel != kNoRemoteModels) {
            remoteProvider->setModel(selectedRemoteModel);
            ready = !remoteApiKey.empty();
            modelInfoLabel.setup(isAzureRemote() ? "Deployment" : "Model", selectedRemoteModel);
        } else {
            ready = false;
            modelInfoLabel.setup(isAzureRemote() ? "Deployment" : "Model", "n/a");
        }

        chatHistory.clear();
        conversationSummary.clear();
        currentState = CHATTING;
        return;
    }

    ready = false;
    // Clear chat history and summary when changing models to reset the context
    chatHistory.clear();
    conversationSummary.clear();
    currentState = CHATTING;

    // Retrieve the full model filename from the display name
    string model = displayName;
    if (displayNameToFullFileName.count(displayName)) {
        model = displayNameToFullFileName[displayName];
    }
    string fullPath = ofToDataPath("models/" + model);
    ofLogNotice() << "Loading model: " << fullPath;

    // Load the model using ofxLlamaCpp
    if (llama.loadModel(fullPath, 2048)) { // 2048 context size
        ready = true;
        
        
        // Set the number of layers to offload to the GPU.
        // By default, offload all layers.
        llama.setN_GpuLayers(llama.getNLayers());
        llama.setOffloadKqv(true);
        
        // Update the GPU status label
        gpuStatusLabel.setup("GPU Layers", ofToString(llama.getN_GpuLayers()));
        
    // Set generation parameters
    llama.setTemperature(0.8f);
    llama.setTopP(0.9f);
    llama.setTopK(40);
    llama.setRepeatPenalty(1.1f);
    
    // Re-apply stop words based on the currently selected template
    string t = templateDropdown->selectedValue.get();
    onTemplateChange(t);

            ofLogNotice() << "Model loaded successfully.";
        } else { // Added this else block for completeness, assuming model load failure
            ofLogError() << "Model load failed!";
        }
    }
//--------------------------------------------------------------
void ofApp::onTemplateChange(string &t) {
    // Clear existing stop words before applying new ones
    llama.clearStopWords(); 
    template_string = mTemplateManager.getTemplate(t);
    
    // Add specific stop words based on the selected template's format
    // This is crucial to prevent the model from generating conversational turns for both user and assistant
    if (t == "DeepSeek") {
        llama.addStopWord("<｜User｜>");
        llama.addStopWord("<｜Assistant｜>");
        llama.addStopWord("<｜End｜>");
        llama.addStopWord("\n<｜User｜>");
        llama.addStopWord("\n<｜Assistant｜>");
    }
    else if (t == "Phi4") {
        llama.addStopWord("<|im_end|>");
        llama.addStopWord("<|end|>");
    }
    else if (t == "Teuken") {
        llama.addStopWord("User:");
        llama.addStopWord("Assistant:");
    }

#if defined(OFX_LLAMACPP_USE_MINJA)
    // Re-create the chat template object with the new template string
    try {
        chat_template = std::make_unique<minja::chat_template>(template_string, "", "");
        ofLogNotice() << "Template switched to: " << t;
    } catch (const std::exception& e) {
        ofLogError() << "Failed to create chat template: " << e.what();
        chat_template = nullptr; // Invalidate the template if parsing fails
    }
#else
    ofLogNotice() << "Template switched to: " << t << " (MSVC fallback formatter)";
#endif
}


//--------------------------------------------------------------
void ofApp::startSummarization() {
    if (backend == ChatBackend::REMOTE) {
        currentState = GENERATING_REPLY;
        startRemoteReplyGeneration();
        return;
    }

    ofLogNotice("ofApp") << "Starting conversation summarization...";

    bool isDeepSeek = (templateDropdown->selectedValue.get() == "DeepSeek");
    nlohmann::json messages_to_summarize = nlohmann::json::array();

    // 1. If a previous summary exists, add it as context for the new summary
    if (!conversationSummary.empty()) {
        // DeepSeek models require system-level instructions to be in the 'user' role
        messages_to_summarize.push_back({
            {"role", isDeepSeek ? "user" : "system"},
            {"content", "[PREVIOUS SUMMARY]\n" + conversationSummary}
        });
    }

    // 2. Add the oldest messages from the history to be summarized and pruned
    int messages_to_prune_count = chatHistory.size() - CHAT_HISTORY_LIMIT;
    for (int i = 0; i < messages_to_prune_count; ++i) {
        const auto& msg = chatHistory[i];
        if (!msg.content.empty()) {
            messages_to_summarize.push_back({
                {"role", msg.isUser ? "user" : "assistant"},
                {"content", msg.content}
            });
        }
    }

    // 3. Add the instruction for the AI to perform the summarization
    std::string summarization_instruction = "Summarize the essence of the above conversation in 4-6 concise bullet points. Your summary will be used as a memory for a large language model.";
    messages_to_summarize.push_back({
        {"role", isDeepSeek ? "user" : "system"},
        {"content", summarization_instruction}
    });

    // 4. Apply the template and start the generation process
    std::string summary_prompt = formatLocalPrompt(messages_to_summarize, true);
    temp_summary_output.clear(); // Clear temporary storage for the new summary

    llama.startGeneration(summary_prompt, 512); // Limit summary length to 512 tokens
    wasGenerating = true;
}

//--------------------------------------------------------------
void ofApp::startReplyGeneration() {
    ofLogNotice("ofApp") << "Starting reply generation...";

    if (backend == ChatBackend::REMOTE) {
        startRemoteReplyGeneration();
        return;
    }
    
    bool isDeepSeek = (templateDropdown->selectedValue.get() == "DeepSeek");
    nlohmann::json messages = nlohmann::json::array();

    // 1. Add the main system prompt (AI's personality/instructions)
    messages.push_back({
        {"role", isDeepSeek ? "user" : "system"},
        {"content", system_prompt}
    });

    // 2. If a conversation summary exists, add it as context
    if (!conversationSummary.empty()) {
        messages.push_back({
            {"role", isDeepSeek ? "user" : "system"},
            {"content", "[CONTEXT SUMMARY]\n" + conversationSummary}
        });
    }

    // 3. Add the recent chat history (a sliding window of the conversation)
    int history_size = chatHistory.size();
    int start_index = std::max(0, history_size - CHAT_HISTORY_LIMIT);
    for (int i = start_index; i < history_size; ++i) {
        const auto& msg = chatHistory[i];
        if (!msg.content.empty()) {
            messages.push_back({
                {"role", msg.isUser ? "user" : "assistant"},
                {"content", msg.content}
            });
        }
    }

    // 4. Apply the template to format the final prompt and start generation
    mPrompt = formatLocalPrompt(messages, true);
    
    ofLogNotice("ofApp PROMPT") << mPrompt;

    llama.startGeneration(mPrompt, 1024); // Limit reply length to 1024 tokens
    wasGenerating = true; 
}

//--------------------------------------------------------------
std::string ofApp::formatLocalPrompt(const nlohmann::json& messages, bool addGenerationPrompt) const {
#if defined(OFX_LLAMACPP_USE_MINJA)
    if (chat_template) {
        minja::chat_template_inputs tmplInputs;
        tmplInputs.messages = messages;
        tmplInputs.add_generation_prompt = addGenerationPrompt;
        return chat_template->apply(tmplInputs);
    }
#endif

    const std::string templateName = templateDropdown ? templateDropdown->selectedValue.get() : "Teuken";
    return formatLocalPromptFallback(messages, templateName, addGenerationPrompt);
}

//--------------------------------------------------------------
std::string ofApp::formatLocalPromptFallback(const nlohmann::json& messages, const std::string& templateName, bool addGenerationPrompt) const {
    std::string prompt;

    for (const auto& message : messages) {
        const std::string role = message.value("role", "");
        const std::string content = message.value("content", "");

        if (templateName == "DeepSeek") {
            if (role == "system") {
                prompt += "<｜System｜>" + content + "\n";
            } else if (role == "user") {
                prompt += "<｜User｜>" + content + "\n";
            } else if (role == "assistant") {
                prompt += "<｜Assistant｜>" + content + "\n";
            }
        } else if (templateName == "Phi4") {
            prompt += "<|im_start|>" + role + "<|im_sep|>\n";
            prompt += content + "<|im_end|>\n";
        } else {
            if (role == "user") {
                prompt += "User: " + content + "\n";
            } else if (role == "assistant") {
                prompt += "Assistant: " + content + "\n";
            }
        }
    }

    if (addGenerationPrompt) {
        if (templateName == "DeepSeek") {
            prompt += "<｜Assistant｜>\n";
        } else if (templateName == "Phi4") {
            prompt += "<|im_start|>assistant<|im_sep|>\n";
        } else {
            prompt += "Assistant: ";
        }
    }

    return prompt;
}

//--------------------------------------------------------------
std::vector<RemoteChatMessage> ofApp::buildRemoteMessages() const {
    std::vector<RemoteChatMessage> messages;
    int history_size = chatHistory.size();
    int start_index = std::max(0, history_size - CHAT_HISTORY_LIMIT);

    for (int i = start_index; i < history_size; ++i) {
        const auto& msg = chatHistory[i];
        if (msg.content.empty()) {
            continue;
        }

        RemoteChatMessage remoteMessage;
        remoteMessage.role = msg.isUser ? "user" : "assistant";
        remoteMessage.content = msg.content;
        messages.push_back(remoteMessage);
    }

    if (!conversationSummary.empty()) {
        const std::string summary = "[CONTEXT SUMMARY]\n" + conversationSummary + "\n\n";
        for (auto& message : messages) {
            if (message.role == "user") {
                message.content = summary + message.content;
                return messages;
            }
        }

        messages.push_back({"user", summary});
    }

    return messages;
}

//--------------------------------------------------------------
void ofApp::startRemoteReplyGeneration() {
    if (!remoteProvider || remoteApiKey.empty()) {
        currentState = CHATTING;
        ofLogError("example_chat") << "Remote backend is not configured.";
        return;
    }

    stopRemoteWorker();
    applyRemoteConfig();

    remoteGenerating = true;
    wasGenerating = true;

    const std::vector<RemoteChatMessage> requestMessages = buildRemoteMessages();
    remoteWorker = std::thread([this, requestMessages]() {
        const std::string reply = remoteProvider->generateChat(requestMessages);
        {
            std::lock_guard<std::mutex> lock(remoteMutex);
            remotePendingReply = reply;
        }
        remoteGenerating = false;
    });
}

//--------------------------------------------------------------
void ofApp::stopRemoteWorker() {
    if (remoteWorker.joinable()) {
        remoteWorker.join();
    }
    remoteGenerating = false;
}


//--------------------------------------------------------------
void ofApp::update() {
    if (!ready) return; // Don't do anything if the model isn't loaded

    if (backend == ChatBackend::REMOTE) {
        if (currentState == GENERATING_REPLY && !remoteGenerating) {
            stopRemoteWorker();

            std::string reply;
            {
                std::lock_guard<std::mutex> lock(remoteMutex);
                reply = remotePendingReply;
                remotePendingReply.clear();
            }

            if (!reply.empty()) {
                chatHistory.push_back({reply, false, ofColor::white});
            }

            currentState = CHATTING;
            wasGenerating = false;
        }
        return;
    }

    // State machine for handling model generation (replying vs. summarizing)
    std::string chunk = llama.getNewOutput();
    if (!chunk.empty()) {
        // Append the new text chunk to the appropriate variable based on the current state
        switch (currentState) {
            case SUMMARIZING:
                temp_summary_output += chunk;
                break;
            case GENERATING_REPLY:
                // If this is the first chunk, create a new message; otherwise, append to the last one
                if (chatHistory.empty() || chatHistory.back().isUser) {
                    chatHistory.push_back({chunk, false, ofColor::white});
                } else {
                    chatHistory.back().content += chunk;
                }
                break;
            default:
                break;
        }
    }

    // Check if generation has just finished
    if (wasGenerating && !llama.isGenerating()) {
        wasGenerating = false; // Reset the flag

        switch (currentState) {
            case SUMMARIZING:
            {
                ofLogNotice("ofApp") << "Summarization finished.";
                conversationSummary = temp_summary_output; // Save the new summary

                // Prune the old part of the chat history that has now been summarized
                int messages_to_prune_count = chatHistory.size() - CHAT_HISTORY_LIMIT;
                if (messages_to_prune_count > 0) {
                    chatHistory.erase(chatHistory.begin(), chatHistory.begin() + messages_to_prune_count);
                }
                
                // Transition to generating the reply to the user's last message
                currentState = GENERATING_REPLY;
                startReplyGeneration();
                break;
            }

            case GENERATING_REPLY:
                ofLogNotice("ofApp") << "Reply finished.";
                // Clean up any trailing stop words from the generated text
                if (!chatHistory.empty() && !chatHistory.back().isUser) {
                    const auto& stopWords = llama.getStopWords();
                    for (const auto& stopWord : stopWords) {
                        size_t pos = chatHistory.back().content.rfind(stopWord);
                        if (pos != std::string::npos) {
                            chatHistory.back().content.erase(pos);
                        }
                    }
                }
                // Transition back to the idle chatting state
                currentState = CHATTING;
                break;
            default:
                break;
        }
    }
    wasGenerating = llama.isGenerating();
}


//--------------------------------------------------------------
void ofApp::draw() {
    // Draw the main GUI panel
    gui.draw();
    drawSelector("Backend", backendDropdown->selectedValue.get(), backendSelectorRect, openSelector == OpenSelector::BACKEND);
    drawSelector("Model", modelDropdown->selectedValue.get(), modelSelectorRect, openSelector == OpenSelector::MODEL);
    if (backend == ChatBackend::LOCAL) {
        drawSelector("Template", templateDropdown->selectedValue.get(), templateSelectorRect, openSelector == OpenSelector::TEMPLATE);
    }
    stopButton.draw();
    gpuStatusLabel.draw();

    if (openSelector != OpenSelector::NONE) {
        const auto options = getOptionsForSelector(openSelector);
        const auto selectedValue = getSelectedValueForSelector(openSelector);
        switch (openSelector) {
            case OpenSelector::BACKEND:
                drawSelectorMenu(options, selectedValue, backendSelectorRect);
                break;
            case OpenSelector::MODEL:
                drawSelectorMenu(options, selectedValue, modelSelectorRect);
                break;
            case OpenSelector::TEMPLATE:
                drawSelectorMenu(options, selectedValue, templateSelectorRect);
                break;
            case OpenSelector::NONE:
            default:
                break;
        }
    }

    // Use fixed GUI dimensions and position for chat window calculation reference
    float horizontalGap = 60; // Desired gap between GUI and chat window
    float verticalPadding = 20; // Top and bottom padding for chat window and overall UI

    ofRectangle chatViewport = ofGetCurrentViewport();

    // Calculate chat window position and dimensions based on fixed GUI area
    chatViewport.x = guiFixedX + guiFixedWidth + horizontalGap; // Start to the right of fixed GUI + gap
    chatViewport.y = verticalPadding; // Start from top, with padding
    chatViewport.width = ofGetWidth() - chatViewport.x - verticalPadding; // Extend to right edge, with padding
    chatViewport.height = ofGetHeight() - 2 * verticalPadding; // Extend full height, with top/bottom padding

    mChatUI.draw(
        chatViewport, // Pass the adjusted viewport
        chatHistory,
        input,
        currentState,
        ready,
        (ready && backend == ChatBackend::LOCAL) ? llama.getContextFillRatio() : 0.0f // Pass context fill ratio for display
    );


}


//--------------------------------------------------------------
void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY) {
    // Delegate scrolling events to the ChatUI class
    const bool isGenerating = backend == ChatBackend::REMOTE ? remoteGenerating.load() : llama.isGenerating();
    mChatUI.mouseScrolled(x, y, scrollX, scrollY, isGenerating);
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
    ofMouseEventArgs args;
    args.x = x;
    args.y = y;
    args.button = button;
    stopButton.mousePressed(args);

    auto handleSelectorClick = [&](OpenSelector selector, const ofRectangle& rect) {
        if (rect.inside(static_cast<float>(x), static_cast<float>(y))) {
            openSelector = openSelector == selector ? OpenSelector::NONE : selector;
            return true;
        }

        if (openSelector != selector) {
            return false;
        }

        const auto options = getOptionsForSelector(selector);
        if (options.empty()) {
            return false;
        }

        const float rowHeight = 19.0f;
        const float menuWidth = getSelectorMenuWidth(options, rect.getWidth());
        const ofRectangle menuRect(rect.getRight(), rect.getTop(), menuWidth, rowHeight * static_cast<float>(options.size()));
        if (!menuRect.inside(static_cast<float>(x), static_cast<float>(y))) {
            return false;
        }

        const int index = static_cast<int>((y - menuRect.y) / rowHeight);
        if (index >= 0 && index < static_cast<int>(options.size())) {
            selectOption(selector, options[static_cast<std::size_t>(index)]);
            openSelector = OpenSelector::NONE;
        }
        return true;
    };

    if (handleSelectorClick(OpenSelector::BACKEND, backendSelectorRect)) {
        return;
    }
    if (handleSelectorClick(OpenSelector::MODEL, modelSelectorRect)) {
        return;
    }
    if (backend == ChatBackend::LOCAL && handleSelectorClick(OpenSelector::TEMPLATE, templateSelectorRect)) {
        return;
    }

    openSelector = OpenSelector::NONE;
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
    ofMouseEventArgs args;
    args.x = x;
    args.y = y;
    args.button = button;
    stopButton.mouseReleased(args);
}

//--------------------------------------------------------------
void ofApp::keyPressed(ofKeyEventArgs &args) {
    if (!ready) return;

    if (args.key == OF_KEY_RETURN) {
        // Only process input if the model is not busy
        if (input.empty() || currentState != CHATTING) return;

        // Add the user's message to the history and clear the input field
        chatHistory.push_back({input, true, ofColor::yellow});
        input.clear();
        
        // Determine whether to summarize or just generate a reply
        if (chatHistory.size() >= static_cast<size_t>(CHAT_HISTORY_LIMIT + SUMMARY_INTERVAL)) {
            currentState = backend == ChatBackend::REMOTE ? GENERATING_REPLY : SUMMARIZING;
            if (backend == ChatBackend::REMOTE) {
                startReplyGeneration();
            } else {
                startSummarization();
            }
        } else {
            currentState = GENERATING_REPLY;
            startReplyGeneration();
        }
        return;
    }

    // Handle backspace for UTF-8 strings
    if (args.key == OF_KEY_BACKSPACE) {
        if (!input.empty()) {
            utf8_pop_back(input);
        }
        return;
    }

    // Append typed characters to the input string
    if (args.codepoint >= 32) { // Ignore control characters
        ofUTF8Append(input, args.codepoint);
    }
}

//--------------------------------------------------------------
void ofApp::stopGeneration() {
    if (backend == ChatBackend::REMOTE) {
        ofLogNotice("example_chat") << "Remote requests cannot be cancelled once sent.";
        return;
    }

    llama.stopGeneration();
    wasGenerating = false;
    
    // Reset the state machine to idle
    if (currentState != CHATTING) {
        currentState = CHATTING;
        
        // Add a marker to indicate that generation was manually stopped
        if (!chatHistory.empty() && !chatHistory.back().isUser) {
            chatHistory.back().content += " [...] (Stopped)";
            chatHistory.back().color = ofColor::orange;
        }
    }
}
