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

class ofApp : public ofBaseApp {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void keyPressed(ofKeyEventArgs &args) override;
    void keyReleased(ofKeyEventArgs &args) override;

private:
    void captureAndAnalyzeFrame();
    std::string wrapString(const std::string &text, float width) const;

    ofxLlamaCpp llama;
    ofVideoGrabber vidGrabber;
    ofImage snapshotImage;
    ofPixels latestFramePixels;

    std::string modelPath;
    std::string mmprojPath;
    std::string snapshotPath;

    std::string output;
    std::string status;

    bool ready = false;
    bool hasLiveFrame = false;
    int camWidth = 1280;
    int camHeight = 720;
};
