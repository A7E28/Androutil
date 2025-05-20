#include <iostream>
#include <string>
#include <limits> 
#include <algorithm> 
#include "../include/utils.h"
#include "../include/path_manager.h"
#include "../include/adb_manager.h"

void displayMenu() {
    std::cout << "\n=== Android Utility Tool ===\n";
    std::cout << "1. Check ADB installation\n";
    std::cout << "2. Install ADB\n";
    std::cout << "3. Update ADB\n";
    std::cout << "4. Get file checksum\n";
    std::cout << "5. Exit\n";
    std::cout << "Enter your choice: ";
}

void checkAdbInstallation() {
    auto adbInfo = adb_manager::checkAdbInstallation();
    
    if (adbInfo.installed) {
        std::cout << "ADB is installed:\n";
        std::cout << "Version: " << (adbInfo.version.empty() ? "Unknown" : adbInfo.version) << "\n";
        std::cout << "Path: " << (adbInfo.path.empty() ? "Unknown" : adbInfo.path) << "\n";
    } else {
        std::cout << "ADB is not installed or not in PATH.\n";
    }
}

void verifyChecksum() {
    std::string filePath;
    
    std::cout << "Select a file to verify checksum..." << std::endl;
    filePath = utils::openFileDialog("Select file to verify checksum");
    
    if (filePath.empty()) {
        std::cout << "File selection canceled." << std::endl;
        return;
    }
    
    std::cout << "Selected file: " << filePath << std::endl;
    
    // Hash algorithm selection
    int hashChoice = 0;
    std::cout << "\nSelect hash algorithm:\n";
    std::cout << "1. MD5\n";
    std::cout << "2. SHA-1\n";
    std::cout << "3. SHA-256\n";
    std::cout << "Enter choice (default: SHA-256): ";
    
    std::string choiceInput;
    std::getline(std::cin, choiceInput);
    
    if (!choiceInput.empty()) {
        try {
            hashChoice = std::stoi(choiceInput);
            if (hashChoice < 1 || hashChoice > 3) {
                hashChoice = 3; 
            }
        } catch (...) {
            hashChoice = 3; 
        }
    } else {
        hashChoice = 3; 
    }
    
    std::string calculatedChecksum;
    std::string hashName;
    
    switch (hashChoice) {
        case 1:
            calculatedChecksum = utils::calculateMD5(filePath);
            hashName = "MD5";
            break;
        case 2:
            calculatedChecksum = utils::calculateSHA1(filePath);
            hashName = "SHA-1";
            break;
        case 3:
        default:
            calculatedChecksum = utils::calculateChecksum(filePath);
            hashName = "SHA-256";
            break;
    }
    
    if (calculatedChecksum.empty()) {
        std::cout << "Failed to calculate checksum. File may be inaccessible." << std::endl;
        return;
    }
    
    std::cout << hashName << " checksum: " << calculatedChecksum << std::endl;
    std::cout << "You can compare this with the expected checksum to verify the file integrity." << std::endl;
}

int main() {
    int choice = 0;
    bool exit = false;
    
    while (!exit) {
        displayMenu();
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
        
        switch (choice) {
            case 1:
                checkAdbInstallation();
                break;
            case 2: {
                auto adbInfo = adb_manager::checkAdbInstallation();
                if (adbInfo.installed) {
                    std::cout << "ADB is already installed. Version: " << adbInfo.version << "\n";
                    std::cout << "Do you want to reinstall? (y/n): ";
                    char confirm;
                    std::cin >> confirm;
                    if (confirm != 'y' && confirm != 'Y') {
                        break;
                    }
                }
                adb_manager::installAdb();
                break;
            }
            case 3:
                adb_manager::updateAdb();
                break;
            case 4:
                verifyChecksum();
                break;
            case 5:
                exit = true;
                break;
            default:
                std::cout << "Invalid choice. Please try again.\n";
                break;
        }
    }
    
    return 0;
}