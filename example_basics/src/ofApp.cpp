#include "ofApp.h"
#include "ofxLlamaCpp.h"

//--------------------------------------------------------------
void ofApp::setup() {

    ofBackground(0);
    ofSetColor(255);
    ofSetFrameRate(60);

    modelLoaded = false;
    output.clear();

    //--------------------------------------------------------------
    // MODEL PATH + CONTEXT
    //--------------------------------------------------------------
    const std::string modelPath =
        ofToDataPath("Teuken-7B-instruct-commercial-v0.4.Q4_K_M.gguf");
    const int contextSize = 2048;

    ofLogNotice() << "Loading model (CPU pass #1): " << modelPath;

    //--------------------------------------------------------------
    //  PASS 1 – CPU LOAD (STABLE ON M2)
    //--------------------------------------------------------------
    if (!llama.loadModel(modelPath, contextSize)) {
        ofLogError() << "CPU load failed.";
        output = "Failed to load model.";
        return;
    }

    ofLogNotice() << "CPU load OK.";

    //--------------------------------------------------------------
    // ENABLE METAL GPU OFFLOAD
    //--------------------------------------------------------------
    llama.setN_GpuLayers(llama.getNLayers());  // offload all layers that are supported
    llama.setOffloadKqv(true);

    ofLogNotice() << "Enabling Metal GPU offload...";
    ofLogNotice() << "n_gpu_layers = " << llama.getN_GpuLayers();

    //--------------------------------------------------------------
    // PASS 2 – GPU LOAD (WORKS ON M2)
    //--------------------------------------------------------------
    if (!llama.loadModel(modelPath, contextSize)) {
        ofLogError() << "GPU load failed.";
        output = "Failed to load model with GPU offload.";
        return;
    }

    ofLogNotice() << "Model successfully loaded with Metal offload.";
    modelLoaded = true;

    //--------------------------------------------------------------
    // SAMPLER SETTINGS
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

    ofLogNotice() << "Generation started.";
}

//--------------------------------------------------------------
void ofApp::update() {
    if (modelLoaded) {
        output += llama.getNewOutput();
    }
}

//--------------------------------------------------------------
void ofApp::draw() {

    float margin = 20;
    float textWidth = ofGetWidth() - margin * 2;

    ofSetColor(255);
    ofDrawBitmapString("Prompt: " + prompt, margin, 50);

    // Strip “Assistant:” prefix if model emits it
    std::string clean = output;
    if (startsWith(clean, "Assistant:")) {
        clean = clean.substr(10);
    }

    // wrap text
    std::string wrapped = wrapString(clean, textWidth);
    ofDrawBitmapString(wrapped, margin, 100);

    // Status
    ofSetColor(200);
    ofDrawBitmapString(
        llama.isGenerating() ? "Generating…" : "Finished.",
        margin,
        ofGetHeight() - 30
    );
}

//--------------------------------------------------------------
std::string ofApp::wrapString(std::string text, int width) {

    ofBitmapFont font;
    std::string wrapped, line;

    std::replace(text.begin(), text.end(), '\n', ' ');

    std::vector<std::string> words = ofSplitString(text, " ", true, true);

    for (const auto &w : words) {
        std::string test = line.empty() ? w : line + " " + w;

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
    return text.compare(0, prefix.size(), prefix) == 0;
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

        llama.stopGeneration();
        output.clear();

        llama.startGeneration(prompt, 1024);
        ofLogNotice() << "Restarted generation.";
    }
}

    
    
