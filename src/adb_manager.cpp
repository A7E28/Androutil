#include "../include/adb_manager.h"
#include "../include/utils.h"
#include "../include/path_manager.h"
#include <iostream>
#include <filesystem>
#include <string>
#include <regex>

namespace adb_manager {

AdbInfo checkAdbInstallation() {
    AdbInfo info = {false, "", ""};
    
    auto result = utils::executeCommand("adb version");
    if (result.has_value()) {
        info.installed = true;
        
        std::regex protocolVersionRegex("Android Debug Bridge version (\\d+\\.\\d+\\.\\d+)");
        std::regex buildVersionRegex("Version ([\\d\\.\\-]+)");
        std::smatch match;
        
        std::string versionOutput = result.value();
        std::string protocolVersion;
        std::string buildVersion;
        
        if (std::regex_search(versionOutput, match, protocolVersionRegex) && match.size() > 1) {
            protocolVersion = match[1].str();
        }
        
        if (std::regex_search(versionOutput, match, buildVersionRegex) && match.size() > 1) {
            buildVersion = match[1].str();
        }
        
        if (!protocolVersion.empty() && !buildVersion.empty()) {
            info.version = protocolVersion + " (build " + buildVersion + ")";
        } else if (!protocolVersion.empty()) {
            info.version = protocolVersion;
        }
        
        auto pathResult = utils::executeCommand("where adb");
        if (pathResult.has_value()) {
            info.path = pathResult.value();
        }
    }
    
    return info;
}

std::optional<std::string> getLatestAdbVersion() {
    const std::string repoUrl = "https://dl.google.com/android/repository/repository2-1.xml";
    
    std::string tempDir = std::filesystem::temp_directory_path().string();
    std::string tempFile = tempDir + "\\android_repo.xml";
    
    if (!utils::downloadFile(repoUrl, tempFile)) {
        std::cout << "Failed to download repository information.\n";
        return std::nullopt;
    }
    
    std::ifstream file(tempFile);
    if (!file.is_open()) {
        std::cout << "Failed to open repository file.\n";
        return std::nullopt;
    }
    
    std::string content;
    std::string line;
    
    while (std::getline(file, line)) {
        content += line + "\n";
    }
    file.close();
    
    std::regex packageRegex("<remotePackage\\s+path=\"platform-tools\"[^>]*>([\\s\\S]*?)</remotePackage>");
    std::regex revisionRegex("<revision>\\s*<major>(\\d+)</major>\\s*<minor>(\\d+)</minor>\\s*<micro>(\\d+)</micro>\\s*</revision>");
    
    std::smatch packageMatch;
    if (std::regex_search(content, packageMatch, packageRegex) && packageMatch.size() > 1) {
        std::string packageContent = packageMatch[1].str();
        
        std::smatch revisionMatch;
        if (std::regex_search(packageContent, revisionMatch, revisionRegex) && revisionMatch.size() > 3) {
            std::string major = revisionMatch[1].str();
            std::string minor = revisionMatch[2].str();
            std::string micro = revisionMatch[3].str();
            
            std::string version = major + "." + minor + "." + micro;
            
            std::filesystem::remove(tempFile);
            
            return version;
        }
    }
    
    std::filesystem::remove(tempFile);
    
    std::cout << "Failed to parse version information from repository.\n";
    return std::nullopt;
}

bool installAdb() {
    std::string url = "https://dl.google.com/android/repository/platform-tools-latest-windows.zip";
    std::string home = utils::getUserHomeDir();
    if (home.empty()) {
        std::cerr << "Unable to locate home directory.\n";
        return false;
    }

    std::string downloadPath = home + "\\Downloads\\platform-tools.zip";
    std::string extractPath = home;
    
    std::cout << "Downloading ADB platform-tools...\n";
    if (!utils::downloadFile(url, downloadPath)) {
        std::cerr << "Download failed.\n";
        return false;
    }

    if (!std::filesystem::exists(downloadPath)) {
        std::cerr << "Downloaded file doesn't exist at path: " << downloadPath << "\n";
        return false;
    }
    
    std::cout << "Extracting ADB tools...\n";
    if (!utils::extractZip(downloadPath, extractPath)) {
        std::cerr << "Extraction failed.\n";
        return false;
    }

    std::string platformToolsPath = home + "\\platform-tools";
    
    std::cout << "Updating system PATH...\n";
    if (!path_manager::addToPath(platformToolsPath)) {
        std::cerr << "Failed to update PATH in registry.\n";
        return false;
    }

    try {
        std::cout << "Cleaning up temporary files...\n";
        std::filesystem::remove(downloadPath);
    } catch (const std::exception& e) {
        std::cerr << "Warning: Could not delete temporary file " << downloadPath << ": " << e.what() << "\n";
    }

    std::cout << "ADB platform-tools installed successfully.\n";
    std::cout << "Note: You may need to restart your terminal or re-login to use ADB commands.\n";
    return true;
}

bool updateAdb() {
    AdbInfo currentAdb = checkAdbInstallation();
    if (!currentAdb.installed) {
        std::cout << "ADB not found. Installing...\n";
        return installAdb();
    }
    
    auto latestVersion = getLatestAdbVersion();
    if (!latestVersion.has_value()) {
        std::cout << "Could not determine latest ADB version.\n";
        return false;
    }
    
    std::string currentVersion = currentAdb.version;
    std::regex buildRegex("build (\\d+\\.\\d+\\.\\d+)");
    std::smatch match;
    
    std::string currentBuildVersion;
    if (std::regex_search(currentVersion, match, buildRegex) && match.size() > 1) {
        currentBuildVersion = match[1].str();
    } else {
        currentBuildVersion = currentVersion;
    }
    
    if (currentBuildVersion == latestVersion.value()) {
        std::cout << "ADB is already up to date (version " << currentAdb.version << ").\n";
        return true;
    }
    
    std::cout << "Current ADB version: " << currentAdb.version << "\n";
    std::cout << "Latest ADB version: " << latestVersion.value() << "\n";
    std::cout << "Updating ADB...\n";
    return installAdb();
}

}