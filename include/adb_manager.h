#pragma once

#include <string>
#include <optional>

namespace adb_manager {
    struct AdbInfo {
        bool installed;
        std::string version;
        std::string path;
    };

    AdbInfo checkAdbInstallation();
    bool installAdb();
    bool updateAdb();
    
    std::optional<std::string> getLatestAdbVersion();
}