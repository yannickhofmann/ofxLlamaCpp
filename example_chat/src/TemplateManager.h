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
#include <string>
#include <vector>
#include <map>

// class TemplateManager
// Manages hardcoded chat templates for different AI models.
//
// This class provides a simple way to store and retrieve different
// chat template formats (e.g., for DeepSeek, Phi4) that are required
// to correctly structure the prompt for the language model.
class TemplateManager {
public:
    // Default constructor. Initializes the available templates.
    TemplateManager();

    // Retrieves the template string for a given template name.
    // param name The name of the desired template (e.g., "DeepSeek").
    // return The chat template string. Returns an empty string if the name is not found.
    std::string getTemplate(const std::string& name);

    // Gets a list of all available template names.
    // return A vector of strings containing the names of all templates.
    std::vector<std::string> getTemplateNames();

private:
    // A map storing the template names and their corresponding template strings.
    std::map<std::string, std::string> templates;
};
