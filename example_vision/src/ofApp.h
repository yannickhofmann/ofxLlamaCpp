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

#pragma once

#include "ofMain.h"
#include "ofxLlamaCpp.h"

// Camera-based multimodal example that captures a frame, saves it to disk,
// and sends it to a vision-capable GGUF model plus mmproj file.
class ofApp : public ofBaseApp {
public:
    // Standard openFrameworks lifecycle callbacks.
    void setup() override;
    void update() override;
    void draw() override;
    void keyPressed(ofKeyEventArgs &args) override;
    void keyReleased(ofKeyEventArgs &args) override;

private:
    // Captures the latest camera frame and starts a multimodal generation run.
    void captureAndAnalyzeFrame();
    // Wraps long model output for the right-hand text column.
    std::string wrapString(const std::string &text, float width) const;

    // Core multimodal runtime objects.
    ofxLlamaCpp llama;
    ofVideoGrabber vidGrabber;
    ofImage snapshotImage;
    ofPixels latestFramePixels;

    // Data paths for the text model, projector model, and temporary snapshot.
    std::string modelPath;
    std::string mmprojPath;
    std::string snapshotPath;

    // UI-visible state.
    std::string output;
    std::string status;

    // Runtime flags and preferred capture resolution.
    bool ready = false;
    bool hasLiveFrame = false;
    int camWidth = 1280;
    int camHeight = 720;
};
