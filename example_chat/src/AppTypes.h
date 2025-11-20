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

#include "ofColor.h"
#include <string>
#include <vector>

// Defines the data structure for a single chat message.
struct ChatMessage {
    // The text content of the message.
    std::string content;
    // A boolean indicating if the message is from the user (true) or the AI (false).
    bool isUser;
    // The color associated with the message, used for display.
    ofColor color;
};

// Defines the main states of the application's state machine.
enum AppState {
    // The state where the user is actively chatting with the AI.
    CHATTING,
    // The state where the application is summarizing the conversation.
    SUMMARIZING,
    // The state where the AI is generating a reply.
    GENERATING_REPLY
};
