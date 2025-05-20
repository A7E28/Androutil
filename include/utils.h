#pragma once

#include <string>
#include <vector>
#include <optional>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <array>

namespace utils {
    std::string getUserHomeDir();

    bool downloadFile(const std::string& url, const std::string& outputPath);
    bool extractZip(const std::string& zipPath, const std::string& destPath);
    std::string calculateChecksum(const std::string& filePath);
    std::optional<std::string> executeCommand(const std::string& command);
    
    std::string calculateMD5(const std::string& filePath);
    std::string calculateSHA1(const std::string& filePath);
    
    std::string openFileDialog(const std::string& title = "Select a file", 
                               const std::string& filter = "All Files\0*.*\0");
}