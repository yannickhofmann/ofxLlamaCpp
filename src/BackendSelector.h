#pragma once

#include "IInferenceProvider.h"

#include <memory>

// Identifies which backend implementation should be created for an example.
enum class BackendType {
    LOCAL,
    REMOTE
};

// Factory that hides the concrete provider classes from the example apps.
class BackendSelector {
public:
    // Returns a ready-to-configure provider for the requested backend family.
    static std::shared_ptr<IInferenceProvider> create(BackendType type);
};
