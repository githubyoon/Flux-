#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <wininet.h>
#include <winhttp.h>
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "winhttp.lib")
#endif

namespace fs = std::filesystem;

const std::string DEFAULT_REGISTRY = "https://flux-registry.example.com/api/v1";

std::string getRegistryUrl() {
    const char* env = std::getenv("FLUX_REGISTRY");
    return env ? std::string(env) : DEFAULT_REGISTRY;
}

std::string getModulesDir() {
    // Resolve modules directory relative to the executable
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    fs::path exeDir = fs::path(path).parent_path();
    return (exeDir / "modules").string();
}

bool downloadFile(const std::string& url, const std::string& outputPath) {
#ifdef _WIN32
    // Use URLDownloadToFile for simplicity
    std::wstring wurl(url.begin(), url.end());
    std::wstring wout(outputPath.begin(), outputPath.end());
    
    HRESULT hr = URLDownloadToFileW(NULL, wurl.c_str(), wout.c_str(), 0, NULL);
    if (SUCCEEDED(hr) && fs::exists(outputPath) && fs::file_size(outputPath) > 0) {
        return true;
    }
    
    // Fallback: use WinHTTP for more control
    HINTERNET hSession = WinHttpOpen(L"Flux-Pkg/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if (!hSession) return false;

    std::wstring host, path;
    size_t slashPos = url.find("://");
    size_t hostStart = (slashPos == std::string::npos) ? 0 : slashPos + 3;
    size_t pathStart = url.find('/', hostStart);
    
    if (pathStart == std::string::npos) {
        host = std::wstring(url.begin() + hostStart, url.end());
        path = L"/";
    } else {
        host = std::wstring(url.begin() + hostStart, url.begin() + pathStart);
        path = std::wstring(url.begin() + pathStart, url.end());
    }

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), NULL, NULL, NULL, WINHTTP_FLAG_SECURE);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return false;
    }
    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return false;
    }

    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    DWORD bytesAvailable = 0;
    std::vector<char> buffer(8192);
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
        DWORD bytesRead = 0;
        if (WinHttpReadData(hRequest, buffer.data(), std::min<DWORD>(bytesAvailable, (DWORD)buffer.size()), &bytesRead)) {
            outFile.write(buffer.data(), bytesRead);
        }
    }

    outFile.close();
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return fs::exists(outputPath) && fs::file_size(outputPath) > 0;
#else
    // Fallback: use system curl
    std::string cmd = "curl -s -o \"" + outputPath + "\" \"" + url + "\"";
    return system(cmd.c_str()) == 0;
#endif
}

bool verifyModule(const std::string& dllPath) {
#ifdef _WIN32
    HMODULE hMod = LoadLibraryExA(dllPath.c_str(), NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (!hMod) return false;
    
    auto apiVer = (int(*)())GetProcAddress(hMod, "flux_module_api_version");
    auto modName = (const char*(*)())GetProcAddress(hMod, "flux_module_name");
    auto funcCnt = (int(*)())GetProcAddress(hMod, "flux_module_function_count");
    auto getFunc = (void(*)(int, const char**, int*, void**))GetProcAddress(hMod, "flux_module_get_function");
    
    bool valid = (apiVer && modName && funcCnt && getFunc);
    FreeLibrary(hMod);
    return valid;
#else
    return true;
#endif
}

void cmdInstall(const std::vector<std::string>& args) {
    // Ensure modules directory exists before install
    fs::create_directories(getModulesDir());
    if (args.empty()) {
        std::cerr << "Usage: flux-pkg install <module_name>" << std::endl;
        return;
    }
    
    std::string name = args[0];
    std::string modulesDir = getModulesDir();
    fs::create_directories(modulesDir);
    std::string dllPath = modulesDir + "\\" + name + ".dll";
    
    // Check if already installed
    if (fs::exists(dllPath)) {
        std::cout << "Module '" << name << "' is already installed." << std::endl;
        return;
    }
    
    std::string registryUrl = getRegistryUrl();
    std::string downloadUrl = registryUrl + "/packages/" + name + "/download";
    
    std::cout << "Downloading module '" << name << "'..." << std::endl;
    
    if (!downloadFile(downloadUrl, dllPath)) {
        std::cerr << "Error: Failed to download module '" << name << "'." << std::endl;
        std::cerr << "  Tried: " << downloadUrl << std::endl;
        std::cerr << "  Make sure FLUX_REGISTRY is set correctly." << std::endl;
        return;
    }
    
    if (!verifyModule(dllPath)) {
        fs::remove(dllPath);
        std::cerr << "Error: Downloaded file is not a valid Flux module." << std::endl;
        return;
    }
    
    std::cout << "Successfully installed '" << name << "'." << std::endl;
}

void cmdUninstall(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: flux-pkg uninstall <module_name>" << std::endl;
        return;
    }
    
    std::string name = args[0];
    std::string modulesDir = getModulesDir();
    std::string dllPath = modulesDir + "\\" + name + ".dll";
    
    if (!fs::exists(dllPath)) {
        std::cout << "Module '" << name << "' is not installed." << std::endl;
        return;
    }
    
    fs::remove(dllPath);
    std::cout << "Uninstalled '" << name << "'." << std::endl;
}

void cmdList() {
    std::string modulesDir = getModulesDir();
    
    if (!fs::exists(modulesDir)) {
        std::cout << "No modules installed." << std::endl;
        return;
    }
    
    std::vector<std::string> modules;
    for (auto& entry : fs::directory_iterator(modulesDir)) {
        if (entry.path().extension() == ".dll") {
            std::string name = entry.path().stem().string();
            
#ifdef _WIN32
            // Try to get module info
            HMODULE hMod = LoadLibraryExA(entry.path().string().c_str(), NULL, LOAD_LIBRARY_AS_DATAFILE);
            if (hMod) {
                auto modName = (const char*(*)())GetProcAddress(hMod, "flux_module_name");
                auto funcCnt = (int(*)())GetProcAddress(hMod, "flux_module_function_count");
                if (modName && funcCnt) {
                    std::cout << "  " << name << " (" << modName() << ", " << funcCnt() << " functions)" << std::endl;
                } else {
                    std::cout << "  " << name << " (invalid module)" << std::endl;
                }
                FreeLibrary(hMod);
            } else {
                std::cout << "  " << name << std::endl;
            }
#else
            std::cout << "  " << name << std::endl;
#endif
        }
    }
    
    if (modules.empty()) {
        std::cout << "No modules installed." << std::endl;
    }
}

void cmdInfo(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: flux-pkg info <module_name>" << std::endl;
        return;
    }
    
    std::string name = args[0];
    std::string modulesDir = getModulesDir();
    std::string dllPath = modulesDir + "\\" + name + ".dll";
    
    if (!fs::exists(dllPath)) {
        std::cout << "Module '" << name << "' is not installed." << std::endl;
        std::cout << "Run 'flux-pkg install " << name << "' to install it." << std::endl;
        return;
    }
    
#ifdef _WIN32
    HMODULE hMod = LoadLibraryExA(dllPath.c_str(), NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hMod) {
        auto apiVer = (int(*)())GetProcAddress(hMod, "flux_module_api_version");
        auto modName = (const char*(*)())GetProcAddress(hMod, "flux_module_name");
        auto funcCnt = (int(*)())GetProcAddress(hMod, "flux_module_function_count");
        auto getFunc = (void(*)(int, const char**, int*, void**))GetProcAddress(hMod, "flux_module_get_function");
        
        if (apiVer && modName && funcCnt && getFunc) {
            std::cout << "Module:     " << modName() << std::endl;
            std::cout << "API Ver:    " << apiVer() << std::endl;
            std::cout << "Functions:  " << funcCnt() << std::endl;
            std::cout << std::endl;
            std::cout << "Exported functions:" << std::endl;
            
            int count = funcCnt();
            for (int i = 0; i < count; i++) {
                const char* fnName = nullptr;
                int fnArity = 0;
                void* fnPtr = nullptr;
                getFunc(i, &fnName, &fnArity, &fnPtr);
                if (fnName) {
                    std::cout << "  [" << i << "] " << fnName;
                    if (fnArity >= 0) std::cout << " (" << fnArity << " args)";
                    else std::cout << " (variadic)";
                    std::cout << std::endl;
                }
            }
        }
        FreeLibrary(hMod);
    }
#endif
}

void printUsage() {
    std::cout << "Flux Package Manager v1.0" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  flux-pkg install <name>    Download and install a module" << std::endl;
    std::cout << "  flux-pkg uninstall <name>  Remove an installed module" << std::endl;
    std::cout << "  flux-pkg list              List installed modules" << std::endl;
    std::cout << "  flux-pkg info <name>       Show module details" << std::endl;
    std::cout << "  flux-pkg --help, -h        Show this help" << std::endl;
    std::cout << std::endl;
    std::cout << "Shortcut:" << std::endl;
    std::cout << "  flux-pkg <name>            Same as 'flux-pkg install <name>'" << std::endl;
    std::cout << std::endl;
    std::cout << "Environment:" << std::endl;
    std::cout << "  FLUX_REGISTRY   Package registry URL (default: " << DEFAULT_REGISTRY << ")" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 0;
    }
    
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    
    std::string command = argv[1];
    std::vector<std::string> args;
    for (int i = 2; i < argc; i++) args.push_back(argv[i]);
    
    if (command == "--help" || command == "-h") {
        printUsage();
        return 0;
    }
    
    try {
        if (command == "install") cmdInstall(args);
        else if (command == "uninstall") cmdUninstall(args);
        else if (command == "list") cmdList();
        else if (command == "info") cmdInfo(args);
        else {
            // Shortcut: flux-pkg <name> = flux-pkg install <name>
            args.insert(args.begin(), command);
            cmdInstall(args);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}




