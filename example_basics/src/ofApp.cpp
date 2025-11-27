#include "ofApp.h"
#include "ofUtils.h"
#include <algorithm>
#include <iomanip>

//--------------------------------------------------------------
void ofApp::setup() {
    ofBackground(0);
    ofSetColor(255);
    ofSetFrameRate(60);

    modelLoaded = false;

    std::string modelPath = ofToDataPath("Teuken-7B-instruct-commercial-v0.4.Q4_K_M.gguf");
    int contextSize = 2048;

    ofLogNotice() << "Loading model (CPU-pass #1): " << modelPath;

    //--------------------------------------------------------------
    // FIRST LOAD: CPU ONLY
    // This matches what your WORKING EXAMPLE does.
    //--------------------------------------------------------------
    if (!llama.loadModel(modelPath, contextSize)) {
        modelLoaded = false;
        output = "Failed to load model on first pass.";
        ofLogError() << "Model load (CPU) failed!";
        return;
    }

    ofLogNotice() << "Model loaded (CPU) successfully.";

    //--------------------------------------------------------------
    // APPLY GPU SETTINGS NOW (after CPU-load)
    //--------------------------------------------------------------
    llama.setN_GpuLayers(llama.getNLayers());   // offload all layers
    llama.setOffloadKqv(true);

    ofLogNotice() << "ofxLlamaCpp: n_gpu_layers set to: " << llama.getN_GpuLayers();
    ofLogNotice() << "ofxLlamaCpp: offload_kqv set to: 1";

    //--------------------------------------------------------------
    // SECOND LOAD: GPU OFFLOAD
    // Now that Metal settings are active, llama.cpp will offload.
    //--------------------------------------------------------------
    ofLogNotice() << "Reloading model (GPU-pass #2)...";

    if (!llama.loadModel(modelPath, contextSize)) {
        modelLoaded = false;
        output = "Failed to load model on second pass (GPU).";
        ofLogError() << "Model reload (GPU) failed!";
        return;
    }

    ofLogNotice() << "Model loaded successfully with GPU offload.";
    modelLoaded = true;

    //--------------------------------------------------------------
    // SET GENERATION PARAMETERS
    //--------------------------------------------------------------
    llama.setTemperature(0.8f);
    llama.setTopK(40);

    llama.addStopWord("User:");
    llama.addStopWord("Assistant:");

    //--------------------------------------------------------------
    // START GENERATION
    //--------------------------------------------------------------
    prompt = "What is openFrameworks?\n\nAssistant:";
    llama.startGeneration(prompt, 1024);

    ofLogNotice() << "Started generation for prompt: " << prompt;
}

//--------------------------------------------------------------
void ofApp::update() {
    if (!modelLoaded) return;
    output += llama.getNewOutput();
}

//--------------------------------------------------------------
void ofApp::draw() {
    ofSetColor(255);
    ofDrawBitmapString("Prompt: " + prompt, 20, 50);

    if (modelLoaded) {
        ofSetColor(173, 255, 47);
    } else {
        ofSetColor(255, 0, 0);
    }

    std::string textToDraw = output;
    if (startsWith(textToDraw, "Assistant:")) {
        textToDraw = textToDraw.substr(std::string("Assistant:").length());
    }

    std::string wrappedOutput = wrapString(textToDraw, ofGetWidth() / 2);
    ofDrawBitmapString(wrappedOutput, 20, 100);

    ofSetColor(255);

    if (llama.isGenerating()) {
        ofDrawBitmapString("Generating...", 20, ofGetHeight() - 30);
    } else if (modelLoaded) {
        ofDrawBitmapString("Generation finished.", 20, ofGetHeight() - 30);
    }
}

//--------------------------------------------------------------
std::string ofApp::wrapString(std::string text, int width) {
    std::string wrapped, currentLine;
    ofBitmapFont font;

    std::replace(text.begin(), text.end(), '\n', ' ');
    std::vector<std::string> words = ofSplitString(text, " ", true, true);

    for (auto &word : words) {
        std::string test = currentLine.empty() ? word : currentLine + " " + word;
        if (font.getBoundingBox(test, 0, 0).width > width) {
            wrapped += currentLine + "\n";
            currentLine = word;
        } else {
            currentLine = test;
        }
    }

    wrapped += currentLine;
    return wrapped;
}

//--------------------------------------------------------------
bool ofApp::startsWith(const std::string &text, const std::string &prefix) {
    return text.rfind(prefix, 0) == 0;
}
