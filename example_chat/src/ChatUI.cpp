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

#include "ChatUI.h"

// --- CONSTRUCTOR & SETUP ---

ChatUI::ChatUI() {
    // Initialize all layout-related constants to default values.
    yOffset = 0;
    chatAreaOuterPadding = 20;
    chatAreaBottomOffset = 110;
    textInnerPadding = 30;
    scrollbarWidth = 6;
    scrollbarGap = 4;
    interMessageSpacing = 20;
}

void ChatUI::setup(const std::string& fontPath, int fontSize) {
    // Load the font for the UI. Anti-aliasing, full character set, and mipmaps are enabled for better rendering.
    font.load(fontPath, fontSize, true, true, true);
    ofLogNotice("ChatUI") << "UI setup complete, font loaded.";
}

// --- DRAWING LOGIC ---

void ChatUI::draw(const ofRectangle& viewport,
                  const std::vector<ChatMessage>& history,
                  const std::string& currentInput,
                  const AppState& appState,
                  bool isModelReady,
                  float contextFillRatio) {
    
    // The main GUI panel (ofxGui) is drawn in ofApp, not here.
    // We estimate its height to correctly position our chat window below it.
    float guiHeight = 100;
    float chatAreaTopOffset = guiHeight + 30;

    // Define the main rectangular area for displaying chat messages.
    ofRectangle chatArea(
        chatAreaOuterPadding,
        chatAreaTopOffset,
        viewport.width - 2 * chatAreaOuterPadding,
        viewport.height - chatAreaTopOffset - chatAreaBottomOffset
    );

    // --- Scrolling & Content Height Calculation ---
    // The height of all chat messages combined is calculated to determine if a scrollbar is necessary.
    // This is a two-pass process:
    // 1. Calculate a provisional height assuming no scrollbar.
    // 2. If the provisional height exceeds the viewport, recalculate with a narrower width to account for the scrollbar.

    // 1. Provisional calculation to determine if a scrollbar is needed.
    float provisionalTextMaxWidth = chatArea.width - (2 * textInnerPadding);
    float provisionalTotalContentHeight = textInnerPadding; // Start with top padding.
    if (!history.empty()) {
        for (size_t i = 0; i < history.size(); ++i) {
            const auto& msg = history[i];
            std::string role = msg.isUser ? "You: " : "LLM: ";
            if (msg.content.rfind("[Summarized", 0) == 0) role = ""; // Don't add a role for summary messages.
            std::string wrapped = wrapText(role + msg.content, provisionalTextMaxWidth);
            provisionalTotalContentHeight += font.stringHeight(wrapped);
            if (i < history.size() - 1) {
                provisionalTotalContentHeight += interMessageSpacing;
            }
        }
        provisionalTotalContentHeight += textInnerPadding; // Add bottom padding.
    }

    // 2. Based on the provisional height, determine the final text width.
    float viewportHeight = chatArea.height;
    bool scrollbarNeeded = (provisionalTotalContentHeight > viewportHeight);
    
    float effectiveChatAreaWidth = chatArea.width;
    if (scrollbarNeeded) {
        effectiveChatAreaWidth -= (scrollbarWidth + scrollbarGap);
    }
    float textMaxWidth = effectiveChatAreaWidth - (2 * textInnerPadding);

    // 3. Final calculation of total content height using the correct text width.
    float totalContentHeight = textInnerPadding;
    if (!history.empty()) {
        for (size_t i = 0; i < history.size(); ++i) {
            const auto& msg = history[i];
            std::string role = msg.isUser ? "You: " : "LLM: ";
            if (msg.content.rfind("[Summarized", 0) == 0) role = "";
            std::string wrapped = wrapText(role + msg.content, textMaxWidth);
            totalContentHeight += font.stringHeight(wrapped);
            if (i < history.size() - 1) {
                totalContentHeight += interMessageSpacing;
            }
        }
        totalContentHeight += textInnerPadding;
    }
    
    // If there's no history, the content height is just the viewport height.
    if (history.empty()) {
        totalContentHeight = viewportHeight;
    }

    // 4. Adjust and clamp the vertical scroll offset (yOffset).
    // Automatically scroll to the bottom when the AI is generating a reply.
    if (appState == GENERATING_REPLY && totalContentHeight > viewportHeight) {
        yOffset = viewportHeight - totalContentHeight;
    }
    // Clamp the offset to ensure the content doesn't scroll out of view.
    yOffset = ofClamp(yOffset, std::min(0.f, viewportHeight - totalContentHeight), 0);

    // --- Drawing the Chat Area ---

    // 1. Draw a white frame around the chat area.
    ofPushStyle();
    ofNoFill();
    ofSetColor(ofColor::white);
    ofSetLineWidth(1);
    ofDrawRectangle(chatArea);
    ofPopStyle();

    // 2. Use a scissor test to clip rendering to within the chat area. This prevents text
    // from drawing outside the designated box.
    glEnable(GL_SCISSOR_TEST);
    glScissor(chatArea.x + 1, ofGetHeight() - (chatArea.y + chatArea.height) + 1, chatArea.width - 2, chatArea.height - 2);

    ofPushMatrix();
    // Apply translation for scrolling.
    ofTranslate(chatArea.x + textInnerPadding, chatArea.y + yOffset);

    // 3. Draw all chat messages.
    float currentY = textInnerPadding;
    for (size_t i = 0; i < history.size(); ++i) {
        const auto& msg = history[i];
        // Set color based on whether the message is from the user, AI, or a system message.
        ofSetColor(msg.isUser ? ofColor::yellow : (msg.content.rfind("[Summarized", 0) == 0 ? ofColor::gray : ofColor::white));
        
        std::string role = msg.isUser ? "You: " : "LLM: ";
        if (msg.content.rfind("[Summarized", 0) == 0) role = "";
        
        std::string wrapped = wrapText(role + msg.content, textMaxWidth);
        font.drawString(wrapped, 0, currentY);
        // Move down for the next message.
        currentY += font.stringHeight(wrapped);
        if (i < history.size() - 1) {
            currentY += interMessageSpacing;
        }
    }

    ofPopMatrix();
    
    // Disable the scissor test after drawing chat content.
    glDisable(GL_SCISSOR_TEST);

    // 4. Draw the scrollbar if needed.
    if (scrollbarNeeded) {
        float scrollbarX = chatArea.getRight() - scrollbarGap - scrollbarWidth;
        float trackY = chatArea.getTop() + scrollbarGap;
        float trackHeight = chatArea.height - (2 * scrollbarGap);

        ofPushStyle();
        // Draw the scrollbar track.
        ofSetColor(50, 50, 50, 150);
        ofDrawRectangle(scrollbarX, trackY, scrollbarWidth, trackHeight);
        
        // Calculate and draw the scrollbar thumb. Its size and position indicate the visible portion of the content.
        float thumbHeight = std::max(20.f, trackHeight * (viewportHeight / totalContentHeight));
        float thumbY = ofMap(-yOffset, 0, totalContentHeight - viewportHeight, trackY, trackY + trackHeight - thumbHeight, true);
        ofSetColor(150, 150, 150, 200);
        ofDrawRectangle(scrollbarX, thumbY, scrollbarWidth, thumbHeight);
        ofPopStyle();
    }

    // --- Bottom UI Elements ---
    float bottomTextY = viewport.height - 80;
    ofSetColor(255);

    // Display the current application status.
    std::string status_text = "Status: CHATTING";
    if (appState == SUMMARIZING) status_text = "Status: SUMMARIZING...";
    if (appState == GENERATING_REPLY) status_text = "Status: GENERATING...";
    font.drawString(status_text, viewport.width - 250, bottomTextY);

    // Display the context fill ratio of the language model.
    if (isModelReady) {
        font.drawString("CTX fill: " + ofToString(contextFillRatio * 100, 1) + "%", 20, bottomTextY);
    }
    
    // Display the prompt for user input.
    font.drawString("Prompt (ENTER):", 20, viewport.height - 50);
    ofSetColor(ofColor::yellow);
    font.drawString("> " + currentInput, 20, viewport.height - 30);
}


// --- INTERACTION & HELPERS ---

void ChatUI::mouseScrolled(int x, int y, float scrollX, float scrollY, bool isGenerating) {
    // Update the vertical scroll offset based on mouse wheel movement.
    // Scrolling is disabled while the AI is generating a reply to prevent jumping.
    if (!isGenerating) {
        yOffset += scrollY * 15;
    }
}

std::string ChatUI::wrapText(const std::string &text, float maxWidth) {
    // A simple text wrapping implementation. It breaks text into lines based on a maximum width.
    std::stringstream ss(text);
    std::string word;
    std::string line;
    std::string result;

    while (ss >> word) {
        std::string testLine = line + word + " ";
        if (font.stringWidth(testLine) > maxWidth) {
            result += line + "\n";
            line = word + " ";
        } else {
            line = testLine;
        }
    }
    result += line;
    return result;
}
