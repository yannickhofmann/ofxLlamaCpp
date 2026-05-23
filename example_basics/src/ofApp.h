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
#include "BackendSelector.h"
#include "RemoteAPIProvider.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

// Minimal openFrameworks example that can switch between a local llama.cpp model
// and a remote OpenAI-compatible endpoint.
class ofApp : public ofBaseApp {
public:
    ~ofApp();

    // Standard openFrameworks lifecycle callbacks.
    void setup();
    void update();
    void draw();
    void exit();
    void keyPressed(ofKeyEventArgs &args);
    void keyReleased(ofKeyEventArgs &args);
    void mousePressed(ofMouseEventArgs &args);

private:
    // Tracks which text field should currently receive keyboard input.
    enum class InputField {
        NONE,
        SYSTEM_PROMPT,
        PROMPT
    };

    // Small UI helpers for text wrapping, layout, and UTF-8 aware editing.
    std::string wrapString(std::string text, int width);
    bool startsWith(const std::string &text, const std::string &prefix);
    std::string maskSecret(const std::string &value) const;
    void drawText(const std::string &text, float x, float y);
    float textWidth(const std::string &text) const;
    ofRectangle getModelLabelBounds() const;
    void drawRemoteConfigOverlay();

    // Setup and backend-selection helpers.
    void loadUiFont();
    void loadRemoteConfigFromFile();
    void loadLocalModelList();
    void selectBackend(BackendType type);
    void selectLocalModel(int direction);
    void selectRemoteModel(int direction);
    void selectModel(int direction);
    bool loadSelectedLocalModel();
    void applySelectedRemoteModel();
    void applyRemoteConfig();
    void startPrompt();
    void stopWorker();
    void drawTextField(const std::string &label, const std::string &value, InputField field, float y, bool secret = false);
    bool isFieldAt(InputField field, int x, int y) const;
    void handleTextInput(const ofKeyEventArgs &args);
    std::string codepointToUtf8(uint32_t codepoint) const;
    void eraseLastUtf8Codepoint(std::string &value) const;
    std::string getInputValue(InputField field) const;
    void setInputValue(InputField field, const std::string &value);
    void setOutput(const std::string &text);
    std::string getOutput() const;
    void setStatus(const std::string &text);
    std::string getStatus() const;
    std::string getBackendName() const;

    // The active inference backend. For remote mode the provider is also
    // exposed as a RemoteAPIProvider so API-specific settings can be applied.
    std::shared_ptr<IInferenceProvider> provider;
    std::shared_ptr<RemoteAPIProvider> remoteProvider;
    BackendType backendType = BackendType::LOCAL;
    bool providerReady = false;
    std::atomic<bool> generating{false};
    std::vector<std::string> remoteModels;
    std::size_t selectedModelIndex = 0;
    std::vector<std::string> localModels;
    std::size_t selectedLocalModelIndex = 0;
    InputField activeInputField = InputField::NONE;

    // UI-visible prompt/output state.
    std::string prompt;
    std::string output;
    std::string status;
    ofTrueTypeFont uiFont;
    bool uiFontLoaded = false;

    // Remote backend settings loaded from remote_api_config.json.
    std::string remoteApiType = "openai_compatible";
    std::string remoteEndpoint = "https://fhgenie.fraunhofer.de/v1";
    std::string remoteApiKey;
    std::string remoteApiVersion = "2024-10-21";
    std::string remoteModelsUrl = "https://fhgenie.fraunhofer.de/v1/models";
    std::string remoteSystemPrompt = "Du bist ein hilfreicher Assistent.";
    std::string preferredRemoteModel = "openGPT-X/Teuken-7B-instruct-v0.6";
    bool remoteSystemPromptAsSystemMessage = false;
    ofJson remoteExtraBody = ofJson::object();
    bool remoteStripReasoning = true;

    // The generation worker keeps blocking inference off the main OF thread.
    std::thread worker;
    mutable std::mutex stateMutex;
};
