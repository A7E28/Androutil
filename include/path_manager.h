#pragma once

#include <string>

namespace path_manager {
    bool addToPath(const std::string& dirPath);
    bool isInPath(const std::string& dirPath);
}