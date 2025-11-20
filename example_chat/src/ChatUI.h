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
#include "AppTypes.h" // Use the new shared types header

// class ChatUI
// Manages the user interface for the chat application.
//
// This class is responsible for drawing the entire chat interface, including
// the chat history, the current user input, and status information.
// It also handles scrolling functionality.
class ChatUI {
public:
    // Default constructor for the ChatUI.
    ChatUI();

    // Sets up the font for the UI.
    // param fontPath The file path to the font.
    // param fontSize The size of the font.
    void setup(const std::string& fontPath, int fontSize);
    
    // Draws the chat interface.
    // param viewport The rectangular area in which to draw the UI.
    // param history A vector of ChatMessage structs representing the chat history.
    // param currentInput The string currently being typed by the user.
    // param appState The current state of the application.
    // param isModelReady A boolean indicating if the Llama model is ready to chat.
    // param contextFillRatio A float representing how much of the model's context is filled.
    void draw(const ofRectangle& viewport,
              const std::vector<ChatMessage>& history,
              const std::string& currentInput,
              const AppState& appState,
              bool isModelReady,
              float contextFillRatio);

    // Handles mouse scroll events.
    // param x The x-coordinate of the mouse.
    // param y The y-coordinate of the mouse.
    // param scrollX The amount of horizontal scroll.
    // param scrollY The amount of vertical scroll.
    // param isGenerating A boolean indicating if the AI is currently generating a reply.
    void mouseScrolled(int x, int y, float scrollX, float scrollY, bool isGenerating);

private:
    // Wraps text to fit within a specified width.
    // param text The text to wrap.
    // param maxWidth The maximum width for the text lines.
    // return The wrapped text as a single string with newlines.
    std::string wrapText(const std::string &text, float maxWidth);
    
    ofTrueTypeFont font; // The font used for rendering all text in the UI.
    float yOffset;       // The current vertical scroll offset.
    
    // Layout constants
    float chatAreaOuterPadding; // Padding around the main chat area.
    float chatAreaBottomOffset; // Offset from the bottom of the screen for the chat area.
    float textInnerPadding;     // Padding inside the message bubbles.
    float scrollbarWidth;       // Width of the scrollbar.
    float scrollbarGap;         // Gap between the scrollbar and the chat content.
    float interMessageSpacing;  // Vertical spacing between consecutive messages.
};
