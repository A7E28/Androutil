#include "../include/path_manager.h"
#include <windows.h>
#include <iostream>
#include <string>

namespace path_manager {

bool isInPath(const std::string& dirPath) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    
    DWORD dataSize = 0;
    DWORD type = 0;
    RegQueryValueExA(hKey, "Path", NULL, &type, NULL, &dataSize);
    
    std::string existingPath(dataSize, '\0');
    if (RegQueryValueExA(hKey, "Path", NULL, &type, reinterpret_cast<LPBYTE>(&existingPath[0]), &dataSize) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return false;
    }
    
    if (!existingPath.empty() && existingPath.back() == '\0') {
        existingPath.resize(existingPath.size() - 1);
    }
    
    RegCloseKey(hKey);
    return existingPath.find(dirPath) != std::string::npos;
}



/* We wont use setx cause it has a limit of 1024 characters for the path
   and it will not work if the path is too long. 
   We will use the registry directly to add the path.
   This function will add the path to the end of the existing PATH variable.
*/
bool addToPath(const std::string& dirPath) {
    if (isInPath(dirPath)) {
        std::cout << "Path already exists. Skipping PATH update.\n";
        return true;
    }
    
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    DWORD dataSize = 0;
    DWORD type = 0;
    RegQueryValueExA(hKey, "Path", NULL, &type, NULL, &dataSize);
    
    std::string existingPath(dataSize, '\0');
    if (RegQueryValueExA(hKey, "Path", NULL, &type, reinterpret_cast<LPBYTE>(&existingPath[0]), &dataSize) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return false;
    }
    
    if (!existingPath.empty() && existingPath.back() == '\0') {
        existingPath.resize(existingPath.size() - 1);
    }

    std::string newFullPath;
    if (!existingPath.empty() && existingPath.back() != ';') {
        newFullPath = existingPath + ";" + dirPath;
    } else {
        newFullPath = existingPath + dirPath;
    }

    if (RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ,
        reinterpret_cast<const BYTE*>(newFullPath.c_str()), static_cast<DWORD>(newFullPath.size() + 1)) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return false;
    }

    RegCloseKey(hKey);
    SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"Environment", SMTO_ABORTIFHUNG, 5000, nullptr);
    return true;
}

} 