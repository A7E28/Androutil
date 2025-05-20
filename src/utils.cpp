#include "../include/utils.h"
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

#include <shldisp.h>    
#include <shobjidl.h>   
#include <exdisp.h>    
#include <objbase.h>   

//COM interface IDs
#include <initguid.h>
#include <shlguid.h>      


#ifndef CLSID_Shell
DEFINE_GUID(CLSID_Shell, 
0x13709620, 0xc279, 0x11ce, 0xa4, 0x9e, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
#endif

#ifndef IID_IShellDispatch
DEFINE_GUID(IID_IShellDispatch,
0xd8f015c0, 0xc278, 0x11ce, 0xa4, 0x9e, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
#endif

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
    std::filesystem::create_directories(destPath);
    
    bool success = false;
    
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) return false;
    
    wchar_t wszZipPath[MAX_PATH];
    wchar_t wszDestPath[MAX_PATH];
    
    MultiByteToWideChar(CP_ACP, 0, zipPath.c_str(), -1, wszZipPath, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, destPath.c_str(), -1, wszDestPath, MAX_PATH);
    
    IShellDispatch* pShell = NULL;
    hr = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void**)&pShell);
    
    if (SUCCEEDED(hr)) {
        Folder* pZipFolder = NULL;
        VARIANT vZipPath;
        VariantInit(&vZipPath);
        vZipPath.vt = VT_BSTR;
        vZipPath.bstrVal = SysAllocString(wszZipPath);
        
        hr = pShell->NameSpace(vZipPath, &pZipFolder);
        
        if (SUCCEEDED(hr) && pZipFolder) {
            Folder* pDestFolder = NULL;
            VARIANT vDestPath;
            VariantInit(&vDestPath);
            vDestPath.vt = VT_BSTR;
            vDestPath.bstrVal = SysAllocString(wszDestPath);
            
            hr = pShell->NameSpace(vDestPath, &pDestFolder);
            
            if (SUCCEEDED(hr) && pDestFolder) {
                FolderItems* pZipItems = NULL;
                hr = pZipFolder->Items(&pZipItems);
                
                if (SUCCEEDED(hr) && pZipItems) {
                    VARIANT vOptions, vItems;
                    VariantInit(&vOptions);
                    VariantInit(&vItems);
                    
                    vOptions.vt = VT_I4;
                    vOptions.lVal = 16; // FOF_NO_UI = 16
                    
                    vItems.vt = VT_DISPATCH;
                    vItems.pdispVal = pZipItems;
                    
                    hr = pDestFolder->CopyHere(vItems, vOptions);
                    
                    success = SUCCEEDED(hr);
                    
                    VariantClear(&vItems);
                    VariantClear(&vOptions);
                    pZipItems->Release();
                }
                
                pDestFolder->Release();
                VariantClear(&vDestPath);
            }
            
            pZipFolder->Release();
            VariantClear(&vZipPath);
        }
        
        pShell->Release();
    }
    
    CoUninitialize();
    return success;
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
    std::vector<BYTE> buffer(8192); // 8KB chunks

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
        // Convert to hex string
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