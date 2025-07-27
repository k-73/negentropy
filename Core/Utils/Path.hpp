#pragma once

#include <filesystem>

#ifndef PROJECT_SOURCE_DIR
#define PROJECT_SOURCE_DIR "."
#endif

namespace Utils {
    inline std::filesystem::path GetWorkspacePath() {
        return std::filesystem::path(PROJECT_SOURCE_DIR) / "Workspace";
    }
}
