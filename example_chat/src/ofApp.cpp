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

    // --- GUI Setup ---
    gui.setup("LLM Control");
    stopButton.addListener(this, &ofApp::stopGeneration);

    // --- Model Loading ---
    // Scan the 'data/models' directory for .gguf files
    ofDirectory dir(ofToDataPath("models"));
    dir.allowExt("gguf");
    dir.listDir();
    dir.sort();

    vector<string> modelDisplayNames;
    displayNameToFullFileName.clear();
    int max_len = 20; // Max length for display names in the dropdown

    // Populate the dropdown list with truncated model names for readability
    for (auto &f : dir.getFiles()) {
        string filename = f.getFileName();
        string displayName = filename;
        if (filename.length() > max_len) {
            displayName = filename.substr(0, max_len - 3) + "...";
        }
        modelDisplayNames.push_back(displayName);
        displayNameToFullFileName[displayName] = filename; // Map display name to full filename
    }

    if (modelDisplayNames.empty()) {
        modelDisplayNames.push_back("NO MODEL FOUND");
    }

    // --- Dropdown Menus ---
    modelDropdown = std::make_shared<ofxDropdown>("Model");
    modelDropdown->add(modelDisplayNames);
    modelDropdown->disableMultipleSelection();
    modelDropdown->enableCollapseOnSelection();
    modelDropdown->addListener(this, &ofApp::onModelChange);
    gui.add(modelDropdown.get());

    templateDropdown = std::make_shared<ofxDropdown>("Template");
    templateDropdown->add(mTemplateManager.getTemplateNames());
    templateDropdown->disableMultipleSelection();
    templateDropdown->enableCollapseOnSelection(); 
    templateDropdown->addListener(this, &ofApp::onTemplateChange);
    gui.add(templateDropdown.get());

    gui.add(stopButton.setup("Stop Generation"));

    // Add GPU status label to the GUI
    gpuStatusLabel.setup("GPU Layers", "N/A"); // Initialize with N/A, assuming ofxLabel adds a colon
    gui.add(&gpuStatusLabel); // Add the label to the GUI panel
    gui.setPosition(20, 20);

    // Store fixed GUI dimensions after setup for stable layout
    guiFixedX = gui.getPosition().x;
    guiFixedWidth = gui.getWidth();


    // --- Default Selections ---
    // Set a default template to ensure the application starts in a valid state
    string t = mTemplateManager.getTemplateNames().front();
    templateDropdown->selectedValue = t;
    onTemplateChange(t);

    // Set a default model
    if (!modelDisplayNames.empty() && modelDisplayNames[0] != "NO MODEL FOUND") {
        string m = modelDisplayNames[0];
        modelDropdown->selectedValue = m;
        onModelChange(m);
    } else {
        string m = "NO MODEL FOUND";
        if(modelDropdown->getNumOptions() > 0) m = modelDropdown->selectedValue.get();
        onModelChange(m);
    }
}

//--------------------------------------------------------------
void ofApp::onModelChange(string &displayName) {
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

    // Re-create the chat template object with the new template string
    try {
        chat_template = std::make_unique<minja::chat_template>(template_string, "", "");
        ofLogNotice() << "Template switched to: " << t;
    } catch (const std::exception& e) {
        ofLogError() << "Failed to create chat template: " << e.what();
        chat_template = nullptr; // Invalidate the template if parsing fails
    }
}


//--------------------------------------------------------------
void ofApp::startSummarization() {
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
    minja::chat_template_inputs tmplInputs;
    tmplInputs.messages = messages_to_summarize;
    tmplInputs.add_generation_prompt = true;

    std::string summary_prompt = chat_template->apply(tmplInputs);
    temp_summary_output.clear(); // Clear temporary storage for the new summary

    llama.startGeneration(summary_prompt, 512); // Limit summary length to 512 tokens
    wasGenerating = true;
}

//--------------------------------------------------------------
void ofApp::startReplyGeneration() {
    ofLogNotice("ofApp") << "Starting reply generation...";
    
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
    minja::chat_template_inputs tmplInputs;
    tmplInputs.messages = messages;
    tmplInputs.add_generation_prompt = true;

    mPrompt = chat_template->apply(tmplInputs);
    
    ofLogNotice("ofApp PROMPT") << mPrompt;

    llama.startGeneration(mPrompt, 1024); // Limit reply length to 1024 tokens
    wasGenerating = true; 
}


//--------------------------------------------------------------
void ofApp::update() {
    if (!ready) return; // Don't do anything if the model isn't loaded

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
        ready ? llama.getContextFillRatio() : 0.0f // Pass context fill ratio for display
    );


}


//--------------------------------------------------------------
void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY) {
    // Delegate scrolling events to the ChatUI class
    mChatUI.mouseScrolled(x, y, scrollX, scrollY, llama.isGenerating());
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
            currentState = SUMMARIZING;
            startSummarization();
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


