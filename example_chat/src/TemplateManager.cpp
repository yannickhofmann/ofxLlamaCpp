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

#include "TemplateManager.h"

TemplateManager::TemplateManager() {
    // =========== Initialize all chat templates here ===========
    // Templates are defined using a syntax compatible with the 'minja' library,
    // which is a Jinja2-like templating engine for C++.

    // --- DEEPSEEK TEMPLATE ---
    // This template uses custom tokens like <｜User｜> and <｜Assistant｜>.
    templates["DeepSeek"] = R"(
{% for message in messages %}
{% if message['role'] == 'system' %}
<｜System｜>{{ message['content'] }}
{% elif message['role'] == 'user' %}
<｜User｜>{{ message['content'] }}
{% elif message['role'] == 'assistant' %}
<｜Assistant｜>{{ message['content'] }}
{% endif %}
{% endfor %}
{% if add_generation_prompt %}
<｜Assistant｜>
{% endif %}
)";

    // --- TEUKEN TEMPLATE ---
    // A simpler format that uses "User:" and "Assistant:" prefixes.
    // It does not have a dedicated system prompt role.
    templates["Teuken"] = R"(
{%- for message in messages %}
{%- if message['role']|lower == 'user' %}
{{- 'User: ' + message['content'] + '\n' }}
{%- elif message['role']|lower == 'assistant' %}
{{- 'Assistant: ' + message['content'] + eos_token + '\n' }}
{%- endif %}
{%- endfor %}
{%-if add_generation_prompt %}
{{- 'Assistant: '}}
{%- endif %}
)";

    // --- PHI-4 (Microsoft) TEMPLATE ---
    // Uses <|im_start|> and <|im_end|> tokens to delimit roles and content.
    templates["Phi4"] = R"(
{% for message in messages %}
<|im_start|>{{ message['role'] }}<|im_sep|>
{{ message['content'] }}<|im_end|>
{% endfor %}
{% if add_generation_prompt %}
<|im_start|>assistant<|im_sep|>
{% endif %}
)";
}

std::string TemplateManager::getTemplate(const std::string& name) {
    // Look for the template by name in the map.
    if (templates.count(name)) {
        return templates.at(name);
    }
    // Log a warning if the template is not found.
    ofLogWarning("TemplateManager") << "Template '" << name << "' not found. Returning empty string.";
    return "";
}

std::vector<std::string> TemplateManager::getTemplateNames() {
    std::vector<std::string> names;
    // Iterate over the map and collect all the keys (template names).
    for (const auto& pair : templates) {
        names.push_back(pair.first);
    }
    return names;
}
