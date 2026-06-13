#ifndef FLUX_MODULE_UTIL_H
#define FLUX_MODULE_UTIL_H

#include <string>
#ifdef _WIN32
#include <windows.h>
namespace Flux::Modules {
    inline std::wstring utf8ToWide(const std::string& str) {
        if (str.empty()) return L"";
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        std::wstring wstr(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
        return wstr;
    }
}
#endif
#endif
