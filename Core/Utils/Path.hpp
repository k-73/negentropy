#pragma once

#include <filesystem>

#ifndef PROJECT_SOURCE_DIR
#define PROJECT_SOURCE_DIR "."
#endif

namespace Utils {
    inline std::filesystem::path GetWorkspacePath() {
#ifdef __EMSCRIPTEN__
        // For WASM, use preloaded virtual filesystem path
        return std::filesystem::path("Workspace");
#else
        // For native builds, use source directory
        return std::filesystem::path(PROJECT_SOURCE_DIR) / "Workspace";
#endif
    }
}
