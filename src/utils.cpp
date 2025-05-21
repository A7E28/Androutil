#include "../include/utils.h"
#include "../include/miniz.h"
#include <iostream>
#include <windows.h>
#include <urlmon.h>
#include <shlobj.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <array>
#include <wincrypt.h>
#include <shlwapi.h>
#include <comdef.h>
#include <algorithm>

namespace utils {

std::string getUserHomeDir() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path))) {
        return std::string(path);
    }
    return "";
}

bool downloadFile(const std::string& url, const std::string& outputPath) {
    return URLDownloadToFileA(NULL, url.c_str(), outputPath.c_str(), 0, NULL) == S_OK;
}

bool extractZip(const std::string& zipPath, const std::string& destPath) {
    try {
        std::filesystem::create_directories(destPath);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create directory: " << e.what() << std::endl;
        return false;
    }
    
    mz_zip_archive zip_archive = {};

    if (!mz_zip_reader_init_file(&zip_archive, zipPath.c_str(), 0)) {
        std::cerr << "Failed to open zip file: " << zipPath << std::endl;
        return false;
    }

    mz_uint numFiles = mz_zip_reader_get_num_files(&zip_archive);
    
    for (mz_uint i = 0; i < numFiles; i++) {
        mz_zip_archive_file_stat file_stat = {};
        
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
            std::cerr << "Failed to get file stats for file #" << i << std::endl;
            mz_zip_reader_end(&zip_archive);
            return false;
        }
        

        if (file_stat.m_is_directory) {
            std::string dirPath = destPath + "/" + file_stat.m_filename;
            std::replace(dirPath.begin(), dirPath.end(), '/', '\\'); 
            try {
                std::filesystem::create_directories(dirPath);
            } catch (const std::exception& e) {
                std::cerr << "Failed to create directory " << dirPath << ": " << e.what() << std::endl;
                mz_zip_reader_end(&zip_archive);
                return false;
            }
            continue;
        }
        

        std::string outPath = destPath + "/" + file_stat.m_filename;
        std::replace(outPath.begin(), outPath.end(), '/', '\\'); 
        
        std::string parent = outPath.substr(0, outPath.find_last_of("/\\"));
        try {
            std::filesystem::create_directories(parent);
        } catch (const std::exception& e) {
            std::cerr << "Failed to create parent directory " << parent << ": " << e.what() << std::endl;
            mz_zip_reader_end(&zip_archive);
            return false;
        }
        
        if (!mz_zip_reader_extract_to_file(&zip_archive, i, outPath.c_str(), 0)) {
            std::cerr << "Failed to extract file: " << outPath << std::endl;
            mz_zip_reader_end(&zip_archive);
            return false;
        }
    }
    
    mz_zip_reader_end(&zip_archive);
    return true;
}

std::optional<std::string> executeCommand(const std::string& command) {
    std::array<char, 128> buffer;
    std::string result;
    
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) {
        return std::nullopt;
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    
    int exitCode = _pclose(pipe);
    if (exitCode != 0) {
        return std::nullopt;
    }
    
    while (!result.empty() && isspace(result.back())) {
        result.pop_back();
    }
    
    return result;
}

enum class HashAlgorithm {
    MD5,
    SHA1,
    SHA256
};

std::string calculateFileHash(const std::string& filePath, HashAlgorithm algorithm) {
    ALG_ID algId;
    switch (algorithm) {
        case HashAlgorithm::MD5:
            algId = CALG_MD5;
            break;
        case HashAlgorithm::SHA1:
            algId = CALG_SHA1;
            break;
        case HashAlgorithm::SHA256:
            algId = CALG_SHA_256;
            break;
        default:
            return "";
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return "";
    }

    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BYTE hashBuffer[64] = {0}; 
    DWORD hashSize = 0;
    std::string result = "";

    file.seekg(0, std::ios::beg);
    std::vector<BYTE> buffer(4096);

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        return "";
    }

    if (!CryptCreateHash(hProv, algId, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return "";
    }

    while (file) {
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        std::streamsize bytesRead = file.gcount();
        
        if (bytesRead <= 0) break;
        
        if (!CryptHashData(hHash, buffer.data(), static_cast<DWORD>(bytesRead), 0)) {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return "";
        }
    }

    hashSize = sizeof(hashBuffer);
    if (CryptGetHashParam(hHash, HP_HASHVAL, hashBuffer, &hashSize, 0)) {
        std::stringstream ss;
        for (DWORD i = 0; i < hashSize; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hashBuffer[i]);
        }
        result = ss.str();
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    
    return result;
}

std::string calculateMD5(const std::string& filePath) {
    return calculateFileHash(filePath, HashAlgorithm::MD5);
}

std::string calculateSHA1(const std::string& filePath) {
    return calculateFileHash(filePath, HashAlgorithm::SHA1);
}

std::string calculateChecksum(const std::string& filePath) {
    return calculateFileHash(filePath, HashAlgorithm::SHA256);
}

std::string openFileDialog(const std::string& title, const std::string& filter) {
    OPENFILENAMEA ofn;
    char szFile[MAX_PATH] = {0};
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    
    std::string filterStr = filter;
    filterStr.push_back('\0'); 
    ofn.lpstrFilter = filterStr.c_str();
    
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = title.c_str();
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    
    if (GetOpenFileNameA(&ofn)) {
        return ofn.lpstrFile; 
    }
    
    return "";
}

}