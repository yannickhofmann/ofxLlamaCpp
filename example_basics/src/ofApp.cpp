#include "ofApp.h"
#include "ofxLlamaCpp.h"

//--------------------------------------------------------------
void ofApp::setup() {

    // Basic window setup
    ofBackground(0);
    ofSetColor(255);
    ofSetFrameRate(60);

    modelLoaded = false;

    // Path to GGUF file
    const std::string modelPath = ofToDataPath("Teuken-7B-instruct-commercial-v0.4.Q4_K_M.gguf");
    const int contextSize = 2048;

    // Configure GPU offloading (optional, safe across platforms)
    llama.setN_GpuLayers(9999);   // "Maximum" as wrapper will clamp internally
    llama.setOffloadKqv(true);    // Recommended for speed

    // Load the model once (CPU/CUDA/Metal auto-detect)
    ofLogNotice() << "Loading model: " << modelPath;

    if (!llama.loadModel(modelPath, contextSize)) {
        ofLogError() << "Model load failed.";
        output = "Failed to load model.";
        return;
    }

    modelLoaded = true;
    ofLogNotice() << "Model loaded. GPU offload auto-selected.";

    // Configure sampler and start the first generation
    llama.setTemperature(0.8f);
    llama.setTopK(40);

    // Stop sequences (useful for chat-like outputs)
    llama.addStopWord("User:");
    llama.addStopWord("Assistant:");

    // Initial question
    prompt = "What is openFrameworks?\n\nAssistant:";

    // Start async generation
    llama.startGeneration(prompt, 1024);
}

//--------------------------------------------------------------
void ofApp::update() {

    // Collect newly generated text (streamed token-by-token)
    if (modelLoaded) {
        output += llama.getNewOutput();
    }
}

//--------------------------------------------------------------
void ofApp::draw() {

    float margin = ofGetWidth() * 0.05f;
    float textWidth = ofGetWidth() - margin * 2;

    // Draw prompt
    ofSetColor(255);
    ofDrawBitmapString("Prompt: " + prompt, margin, 50);

    // Clean assistant prefix if the model outputs it
    std::string text = output;
    if (startsWith(text, "Assistant:")) {
        text = text.substr(10);
    }

    // Wrap output text to window width
    std::string wrapped = wrapString(text, textWidth);
    ofDrawBitmapString(wrapped, margin, 100);

    // Status indicator
    ofDrawBitmapString(
        llama.isGenerating() ? "Generating…" : "Finished.",
        margin, ofGetHeight() - 30
    );
}

//--------------------------------------------------------------
std::string ofApp::wrapString(std::string text, int width) {

    ofBitmapFont font;
    std::string wrapped, line;

    // Replace newlines with spaces (makes wrapping easier)
    for (char &c : text) {
        if (c == '\n') c = ' ';
    }

    // Split into words
    std::vector<std::string> words;
    std::string temp;

    for (char c : text) {
        if (c == ' ') {
            if (!temp.empty()) {
                words.push_back(temp);
                temp.clear();
            }
        } else {
            temp += c;
        }
    }
    if (!temp.empty()) words.push_back(temp);

    // Merge words into wrapped lines
    for (const std::string &w : words) {
        const std::string test = line.empty() ? w : line + " " + w;

        if (font.getBoundingBox(test, 0, 0).width > width) {
            wrapped += line + "\n";
            line = w;
        } else {
            line = test;
        }
    }

    wrapped += line;
    return wrapped;
}

//--------------------------------------------------------------
bool ofApp::startsWith(const std::string &text, const std::string &prefix) {
    return text.size() >= prefix.size() &&
           text.compare(0, prefix.size(), prefix) == 0;
}

//--------------------------------------------------------------
void ofApp::keyPressed(ofKeyEventArgs &args) {
    if (args.key == OF_KEY_ESC) {
        std::_Exit(0);
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(ofKeyEventArgs &args) {

    if (args.key == ' ') {

        // Stop current generation thread
        llama.stopGeneration();

        // Clear previous text
        output.clear();

        // Restart generation
        llama.startGeneration(prompt, 1024);

        ofLogNotice() << "Re-sent prompt: " << prompt;
    }
}

