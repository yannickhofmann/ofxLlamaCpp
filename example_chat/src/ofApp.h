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
 
#pragma once
#include "ofMain.h"
#include "ofxGui.h"
#include "ofxLlamaCpp.h"
#include "RemoteAPIProvider.h"
#if !defined(_MSC_VER)
#define OFX_LLAMACPP_USE_MINJA 1
#include "minja/chat-template.hpp"
#endif
#include "ofxDropdown.h"
#include "nlohmann/json.hpp"
#include "ChatUI.h"
#include "AppTypes.h"
#include "TemplateManager.h"
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

// class ofApp
// The main application class that orchestrates the entire chat application.
//
// This class handles the application's state, user input, the Llama language model,
// and the user interface. It follows the openFrameworks application structure.
class ofApp : public ofBaseApp {
public:
    // Called once at the beginning of the application's lifecycle.
    void setup();
    
    // Called repeatedly to update the application's state.
    void update();
    
    // Called repeatedly to draw the application's visuals.
    void draw();
    
    // Handles key press events for user input.
    // param args The key event arguments.
    void keyPressed(ofKeyEventArgs &args);
    
    // Stops the Llama model from generating a response.
    void stopGeneration();
    
    // Handles mouse scroll events, passed to the ChatUI.
    // param x The x-coordinate of the mouse.
    // param y The y-coordinate of the mouse.
    // param scrollX The amount of horizontal scroll.
    // param scrollY The amount of vertical scroll.
    void mouseScrolled(int x, int y, float scrollX, float scrollY);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void exit();

private:
    enum class ChatBackend {
        LOCAL,
        REMOTE
    };
    enum class OpenSelector {
        NONE,
        BACKEND,
        MODEL,
        TEMPLATE
    };

    // --- State Machine ---
    AppState currentState = CHATTING; // The current state of the application (e.g., chatting, summarizing).
    void startReplyGeneration(); // Initiates the process of the AI generating a reply.
    void startSummarization();   // Initiates the process of summarizing the conversation.
    std::string temp_summary_output; // Temporary storage for the summary while it's being generated.

    // --- GUI ---
    ofxPanel gui; // The main GUI panel for controls.
    ofxButton stopButton; // A button to stop AI generation.
    std::shared_ptr<ofxDropdown> backendDropdown; // Dropdown for local vs remote backend.
    std::shared_ptr<ofxDropdown> modelDropdown; // Dropdown for selecting the AI model.
    std::shared_ptr<ofxDropdown> templateDropdown; // Dropdown for selecting the chat template.
    ofxLabel modelInfoLabel; // Fixed label for Azure deployments without model lists.
    std::map<std::string, std::string> displayNameToFullFileName; // Maps user-friendly model names to their file paths.
    float guiFixedX; // Stores the initial X position of the GUI for stable layout.
    float guiFixedWidth; // Stores the initial width of the GUI for stable layout.
    ofxLabel gpuStatusLabel; // Label to display GPU offload status within the GUI.
    OpenSelector openSelector = OpenSelector::NONE;
    ofRectangle backendSelectorRect;
    ofRectangle modelSelectorRect;
    ofRectangle templateSelectorRect;


    // Callback for when the AI model is changed via the dropdown.
    // param displayName The user-friendly name of the selected model.
    void onModelChange(string &displayName);
    void onBackendChange(string &displayName);
    void rebuildGuiForBackend();
    
    // Callback for when the chat template is changed via the dropdown.
    // param t The name of the selected template.
    void onTemplateChange(string &t);

    // --- Llama Engine ---
    ofxLlamaCpp llama; // The core Llama language model object.
    std::shared_ptr<RemoteAPIProvider> remoteProvider;
    ChatBackend backend = ChatBackend::LOCAL;
    bool ready = false; // Flag indicating if the model is loaded and ready.
    bool wasGenerating = false; // Flag to track if the model was generating in the previous frame.
    std::atomic<bool> remoteGenerating{false};
    std::thread remoteWorker;
    std::mutex remoteMutex;
    std::string remotePendingReply;

    // --- Templates ---
    std::string system_prompt; // The system prompt, defining the AI's role or persona.
    std::string template_string; // The raw string for the chat template.
#if defined(OFX_LLAMACPP_USE_MINJA)
    std::unique_ptr<minja::chat_template> chat_template; // The parsed chat template object.
#endif
    std::string remoteApiType = "openai_compatible";
    std::string remoteEndpoint = "https://fhgenie.fraunhofer.de/v1";
    std::string remoteApiKey;
    std::string remoteApiVersion = "2024-10-21";
    std::string remoteModelsUrl = "https://fhgenie.fraunhofer.de/v1/models";
    std::string preferredRemoteModel = "openGPT-X/Teuken-7B-instruct-v0.6";
    std::string selectedRemoteModel;
    bool remoteSystemPromptAsSystemMessage = false;
    ofJson remoteExtraBody = ofJson::object();
    bool remoteStripReasoning = true;

    // --- Chat/I/O ---
    std::string input;   // The current string of user input.
    std::string mPrompt; // The fully formatted prompt sent to the Llama model.
    
    // --- Memory ---
    std::vector<ChatMessage> chatHistory; // A vector storing the history of the conversation.
    int CHAT_HISTORY_LIMIT = 8; // The maximum number of messages to keep in active history before summarizing.
    int SUMMARY_INTERVAL = 4;   // The number of messages to process in each summarization step.
    std::string conversationSummary; // A running summary of the conversation.
    
    // --- UI ---
    ChatUI mChatUI; // The object that manages the chat user interface.
    TemplateManager mTemplateManager; // The object that manages loading and selecting chat templates.
    void drawSelector(const std::string& label, const std::string& value, const ofRectangle& rect, bool open) const;
    void drawSelectorMenu(const std::vector<std::string>& options, const std::string& selectedValue, const ofRectangle& anchorRect) const;
    std::vector<std::string> getOptionsForSelector(OpenSelector selector) const;
    std::string getSelectedValueForSelector(OpenSelector selector) const;
    float getSelectorMenuWidth(const std::vector<std::string>& options, float minWidth) const;
    std::string fitSelectorText(const std::string& value, float maxWidth) const;
    void selectOption(OpenSelector selector, const std::string& value);
    void loadRemoteConfigFromFile();
    void populateLocalModels();
    void populateRemoteModels();
    void applyRemoteConfig();
    bool isAzureRemote() const;
    std::vector<RemoteChatMessage> buildRemoteMessages() const;
    std::string formatLocalPrompt(const nlohmann::json& messages, bool addGenerationPrompt) const;
    std::string formatLocalPromptFallback(const nlohmann::json& messages, const std::string& templateName, bool addGenerationPrompt) const;
    void startRemoteReplyGeneration();
    void stopRemoteWorker();
};
