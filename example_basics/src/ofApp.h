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
#include "ofxLlamaCpp.h"

class ofApp : public ofBaseApp {
public:
    void setup();
    void update();
    void draw();
    void keyPressed(ofKeyEventArgs &args);
    void keyReleased(ofKeyEventArgs &args);

private:
    std::string wrapString(std::string text, int width);
    bool startsWith(const std::string &text, const std::string &prefix);

    ofxLlamaCpp llama;
    bool modelLoaded = false;

    std::string prompt;
    std::string output;
};

