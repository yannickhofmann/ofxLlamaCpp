/*
 * ofxLlamaCpp
 *
 * Copyright (c) 2026 Yannick Hofmann
 * <contact@yannickhofmann.de>
 *
 * BSD Simplified License. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "ofApp.h"

#include <algorithm>

namespace {
std::string stripAssistantPrefix(const std::string &text) {
    const std::string prefix = "ASSISTANT:";
    if (text.rfind(prefix, 0) == 0) {
        return text.substr(prefix.size());
    }
    return text;
}
}

void ofApp::setup() {
    ofSetWindowTitle("ofxLlamaCpp example_vision");
    ofSetFrameRate(60);
    ofBackground(12);
    ofSetVerticalSync(true);

    modelPath = ofToDataPath("models/llava-v1.6-mistral-7b.Q4_K_M.gguf");
    mmprojPath = ofToDataPath("models/mmproj-model-f16.gguf");
    snapshotPath = ofToDataPath("snapshot.png", true);

    std::vector<ofVideoDevice> devices = vidGrabber.listDevices();
    for (const auto &device : devices) {
        if (device.bAvailable) {
            ofLogNotice("example_vision") << device.id << ": " << device.deviceName;
        } else {
            ofLogNotice("example_vision") << device.id << ": " << device.deviceName << " - unavailable";
        }
    }

    vidGrabber.setDeviceID(0);
    vidGrabber.setDesiredFrameRate(30);
    vidGrabber.setup(camWidth, camHeight);

    llama.setN_GpuLayers(99);
    llama.setOffloadKqv(true);

    if (!ofFile::doesFileExist(modelPath) || !ofFile::doesFileExist(mmprojPath)) {
        status = "Model or mmproj file missing in bin/data/models.";
        ofLogError("example_vision") << status;
        return;
    }

    status = "Loading vision model...";
    if (!llama.loadVisionModel(modelPath, mmprojPath, 4096)) {
        status = "Failed to load multimodal model.";
        ofLogError("example_vision") << status;
        return;
    }

    llama.setTemperature(0.1f);
    llama.setTopP(0.9f);
    llama.setTopK(40);
    llama.setRepeatPenalty(1.05f);

    ready = true;
    status = "Camera ready. Release space to capture a frame and ask the model.";
}

void ofApp::update() {
    vidGrabber.update();
    if (vidGrabber.isFrameNew()) {
        latestFramePixels = vidGrabber.getPixels();
        hasLiveFrame = latestFramePixels.isAllocated();
    }

    if (!ready) {
        return;
    }

    output += llama.getNewOutput();

    if (llama.isGenerating()) {
        status = "Generating...";
    } else if (snapshotImage.isAllocated()) {
        status = "Ready. Release space to capture a new frame.";
    }
}

void ofApp::draw() {
    const float margin = 24.0f;
    const float columnGap = 24.0f;
    const float leftW = (ofGetWidth() - margin * 2.0f - columnGap) * 0.42f;
    const float rightW = ofGetWidth() - margin * 2.0f - columnGap - leftW;

    ofSetColor(245);
    ofDrawBitmapStringHighlight("example_vision", margin, margin);

    ofSetColor(220);
    ofDrawBitmapString("Model: " + ofFilePath::getFileName(modelPath), margin, margin + 28.0f);
    ofDrawBitmapString("mmproj: " + ofFilePath::getFileName(mmprojPath), margin, margin + 46.0f);
    ofDrawBitmapString("Status: " + status, margin, margin + 64.0f);
    ofDrawBitmapString("Prompt: What do you see in the image?", margin, margin + 96.0f);
    ofDrawBitmapString("Keys: Space release=capture  c=clear output", margin, margin + 114.0f);

    const float imageTop = margin + 140.0f;
    const float imageBoxH = ofGetHeight() - imageTop - margin;
    const float liveH = imageBoxH * 0.58f;
    const float snapshotTop = imageTop + liveH + 16.0f;
    const float snapshotH = imageBoxH - liveH - 16.0f;

    ofNoFill();
    ofSetColor(90);
    ofDrawRectangle(margin, imageTop, leftW, liveH);
    ofDrawRectangle(margin, snapshotTop, leftW, snapshotH);
    ofDrawRectangle(margin + leftW + columnGap, imageTop, rightW, imageBoxH);

    if (hasLiveFrame) {
        ofRectangle fit = ofRectangle(0, 0, camWidth, camHeight);
        fit.scaleTo(ofRectangle(margin + 8.0f, imageTop + 8.0f, leftW - 16.0f, liveH - 16.0f), OF_SCALEMODE_FIT);
        ofSetColor(255);
        vidGrabber.draw(fit);
        ofSetColor(220);
        ofDrawBitmapString("Live camera", margin + 8.0f, imageTop + liveH - 8.0f);
    } else {
        ofSetColor(180);
        ofDrawBitmapString("Waiting for camera frames.", margin + 12.0f, imageTop + 24.0f);
    }

    if (snapshotImage.isAllocated()) {
        ofRectangle fit = ofRectangle(0, 0, snapshotImage.getWidth(), snapshotImage.getHeight());
        fit.scaleTo(ofRectangle(margin + 8.0f, snapshotTop + 8.0f, leftW - 16.0f, snapshotH - 16.0f), OF_SCALEMODE_FIT);
        ofSetColor(255);
        snapshotImage.draw(fit);

        ofNoFill();
        ofSetColor(255, 210, 80);
        ofSetLineWidth(2.0f);
        ofDrawRectangle(margin + 4.0f, snapshotTop + 4.0f, leftW - 8.0f, snapshotH - 8.0f);
        ofSetLineWidth(1.0f);

        ofSetColor(255, 210, 80);
        ofDrawBitmapString("Analyzed snapshot", margin + 8.0f, snapshotTop + snapshotH - 8.0f);
    } else {
        ofSetColor(180);
        ofDrawBitmapString("No captured snapshot yet.", margin + 12.0f, snapshotTop + 24.0f);
        ofDrawBitmapString("Release space to freeze the current frame.", margin + 12.0f, snapshotTop + 42.0f);
    }

    const std::string visibleOutput = stripAssistantPrefix(output);
    ofSetColor(235);
    const float textX = margin + leftW + columnGap + 12.0f;
    const float textY = imageTop + 24.0f;
    const std::string readableOutput = visibleOutput.empty() ? "No output yet." : visibleOutput;
    ofDrawBitmapStringHighlight(
        wrapString(readableOutput, rightW - 24.0f),
        textX,
        textY,
        ofColor(255, 255, 255, 20),
        ofColor(245)
    );
}

void ofApp::keyPressed(ofKeyEventArgs &args) {
    if (args.key == 'c') {
        output.clear();
        status = "Output cleared.";
    }
}

void ofApp::keyReleased(ofKeyEventArgs &args) {
    if (args.key == ' ') {
        captureAndAnalyzeFrame();
    }
}

void ofApp::captureAndAnalyzeFrame() {
    if (!ready) {
        status = "Model is not ready.";
        return;
    }

    if (!hasLiveFrame || !latestFramePixels.isAllocated()) {
        status = "No camera frame available yet.";
        return;
    }

    snapshotImage.setFromPixels(latestFramePixels);
    if (!ofSaveImage(latestFramePixels, snapshotPath)) {
        status = "Failed to save captured frame.";
        return;
    }

    output.clear();
    status = "Captured frame. Sending to model...";
    llama.startVisionGeneration("What do you see in the image?", snapshotPath, 512);
}

std::string ofApp::wrapString(const std::string &text, float width) const {
    ofBitmapFont font;
    std::string sanitized = text;
    std::replace(sanitized.begin(), sanitized.end(), '\t', ' ');

    std::string line;
    std::string wrapped;
    std::vector<std::string> paragraphs = ofSplitString(sanitized, "\n", false, false);

    for (size_t p = 0; p < paragraphs.size(); ++p) {
        std::vector<std::string> words = ofSplitString(paragraphs[p], " ", true, true);
        line.clear();
        for (const auto &word : words) {
            const std::string candidate = line.empty() ? word : line + " " + word;
            if (font.getBoundingBox(candidate, 0, 0).width > width) {
                if (!wrapped.empty()) {
                    wrapped += "\n";
                }
                wrapped += line;
                line = word;
            } else {
                line = candidate;
            }
        }

        if (!line.empty()) {
            if (!wrapped.empty()) {
                wrapped += "\n";
            }
            wrapped += line;
        }

        if (p + 1 < paragraphs.size()) {
            wrapped += "\n";
        }
    }

    return wrapped;
}
