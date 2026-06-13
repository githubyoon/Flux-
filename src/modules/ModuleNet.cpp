#include "ModuleNet.h"
#include "VM.h"
#include "ModuleUtil.h"
#include <string>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#include <wininet.h>
#include <shlwapi.h>
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "shlwapi.lib")
#endif

namespace Flux::Modules {
    namespace {
        int g_netLastStatus = 0;
        std::string g_netLastHeaders;
    }

    void handleNet(const std::string& subName, VM& vm) {
        if (subName == "status") {
            vm.pop(); vm.push(g_netLastStatus);
        } else if (subName == "headers") {
            vm.pop(); vm.push(g_netLastHeaders);
        } else if (subName == "ip") {
            std::string ipResult;
#ifdef _WIN32
            HINTERNET hSession = InternetOpenW(L"FluxAgent", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
            if (hSession) {
                HINTERNET hConnect = InternetOpenUrlW(hSession, L"http://api.ipify.org", NULL, 0, INTERNET_FLAG_RELOAD, 0);
                if (hConnect) {
                    char buffer[256]; DWORD bytesRead;
                    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
                        ipResult.append(buffer, bytesRead);
                    InternetCloseHandle(hConnect);
                }
                InternetCloseHandle(hSession);
            }
#endif
            vm.pop(); vm.push(ipResult);
        } else if (subName == "encode") {
            std::string str = Runtime::valueToString(vm.pop());
#ifdef _WIN32
            std::wstring wstr = utf8ToWide(str);
            wchar_t buf[4096]; DWORD buflen = 4096;
            if (UrlEscapeW(wstr.c_str(), buf, &buflen, URL_ESCAPE_AS_UTF8) == S_OK) {
                int size = WideCharToMultiByte(CP_UTF8, 0, buf, -1, NULL, 0, NULL, NULL);
                std::string result(size - 1, 0);
                WideCharToMultiByte(CP_UTF8, 0, buf, -1, &result[0], size, NULL, NULL);
                vm.pop(); vm.push(result);
            } else {
                vm.pop(); vm.push(str);
            }
#else
            vm.pop(); vm.push(str); return;
#endif
        } else if (subName == "decode") {
            std::string str = Runtime::valueToString(vm.pop());
#ifdef _WIN32
            std::wstring wstr = utf8ToWide(str);
            wchar_t buf[4096];
            wcscpy_s(buf, wstr.c_str());
            DWORD buflen = 4096;
            if (UrlUnescapeW(buf, NULL, &buflen, URL_UNESCAPE_INPLACE) == S_OK) {
                int size = WideCharToMultiByte(CP_UTF8, 0, buf, -1, NULL, 0, NULL, NULL);
                std::string result(size - 1, 0);
                WideCharToMultiByte(CP_UTF8, 0, buf, -1, &result[0], size, NULL, NULL);
                vm.pop(); vm.push(result);
            } else {
                vm.pop(); vm.push(str);
            }
#else
            vm.pop(); vm.push(str); return;
#endif
        } else {
            std::string url = Runtime::valueToString(vm.pop());
            if (subName == "get" || subName == "post") {
                std::string data = (subName == "post") ? Runtime::valueToString(vm.pop()) : "";
                vm.pop(); // pop net object

#ifdef _WIN32
                HINTERNET hSession = InternetOpenW(L"FluxAgent", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
                if (hSession) {
                    HINTERNET hRequest = InternetOpenUrlW(hSession, utf8ToWide(url).c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
                    if (hRequest) {
                        DWORD statusCode = 0; DWORD statusSize = sizeof(statusCode);
                        HttpQueryInfoW(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusSize, NULL);
                        g_netLastStatus = (int)statusCode;
                        char headerBuf[4096]; DWORD headerSize = sizeof(headerBuf);
                        if (HttpQueryInfoW(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, headerBuf, &headerSize, NULL))
                            g_netLastHeaders = std::string(headerBuf, headerSize);
                        std::string response;
                        char buffer[4096];
                        DWORD bytesRead;
                        while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
                            response.append(buffer, bytesRead);
                        InternetCloseHandle(hRequest);
                        InternetCloseHandle(hSession);
                        vm.push(response);
                        return;
                    }
                    InternetCloseHandle(hSession);
                }
#endif
                vm.push(std::string(""));
                return;
            } else if (subName == "put") {
                std::string data = Runtime::valueToString(vm.pop());
                vm.pop(); // pop net object

#ifdef _WIN32
                HINTERNET hSession = InternetOpenW(L"FluxAgent", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
                if (hSession) {
                    HINTERNET hConnect = InternetConnectW(hSession, NULL, INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
                    if (hConnect) {
                        std::wstring wurl = utf8ToWide(url);
                        std::wstring wHost; std::wstring wPath;
                        size_t sl = wurl.find(L"://");
                        if (sl != std::wstring::npos) sl = wurl.find(L'/', sl + 3);
                        else sl = wurl.find(L'/');
                        if (sl != std::wstring::npos) { wHost = wurl.substr(0, sl); wPath = wurl.substr(sl); }
                        else { wHost = wurl; wPath = L"/"; }
                        HINTERNET hRequest = HttpOpenRequestW(hConnect, L"PUT", wPath.c_str(), NULL, NULL, NULL, 0, 0);
                        if (hRequest) {
                            std::wstring wdata = utf8ToWide(data);
                            if (HttpSendRequestW(hRequest, NULL, 0, (LPVOID)wdata.c_str(), (DWORD)wdata.size() * 2)) {
                                DWORD statusCode = 0; DWORD statusSize = sizeof(statusCode);
                                HttpQueryInfoW(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusSize, NULL);
                                g_netLastStatus = (int)statusCode;
                                char headerBuf[4096]; DWORD headerSize = sizeof(headerBuf);
                                if (HttpQueryInfoW(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, headerBuf, &headerSize, NULL))
                                    g_netLastHeaders = std::string(headerBuf, headerSize);
                                std::string response;
                                char buffer[4096]; DWORD bytesRead;
                                while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
                                    response.append(buffer, bytesRead);
                                InternetCloseHandle(hRequest); InternetCloseHandle(hConnect); InternetCloseHandle(hSession);
                                vm.push(response); return;
                            }
                            InternetCloseHandle(hRequest);
                        }
                        InternetCloseHandle(hConnect);
                    }
                    InternetCloseHandle(hSession);
                }
#endif
                vm.push(std::string(""));
            } else if (subName == "del") {
                vm.pop(); // pop net object

#ifdef _WIN32
                HINTERNET hSession = InternetOpenW(L"FluxAgent", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
                if (hSession) {
                    HINTERNET hConnect = InternetConnectW(hSession, NULL, INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
                    if (hConnect) {
                        std::wstring wurl = utf8ToWide(url);
                        std::wstring wHost; std::wstring wPath;
                        size_t sl = wurl.find(L"://");
                        if (sl != std::wstring::npos) sl = wurl.find(L'/', sl + 3);
                        else sl = wurl.find(L'/');
                        if (sl != std::wstring::npos) { wHost = wurl.substr(0, sl); wPath = wurl.substr(sl); }
                        else { wHost = wurl; wPath = L"/"; }
                        HINTERNET hRequest = HttpOpenRequestW(hConnect, L"DELETE", wPath.c_str(), NULL, NULL, NULL, 0, 0);
                        if (hRequest) {
                            if (HttpSendRequestW(hRequest, NULL, 0, NULL, 0)) {
                                DWORD statusCode = 0; DWORD statusSize = sizeof(statusCode);
                                HttpQueryInfoW(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusSize, NULL);
                                g_netLastStatus = (int)statusCode;
                                char headerBuf[4096]; DWORD headerSize = sizeof(headerBuf);
                                if (HttpQueryInfoW(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, headerBuf, &headerSize, NULL))
                                    g_netLastHeaders = std::string(headerBuf, headerSize);
                                std::string response;
                                char buffer[4096]; DWORD bytesRead;
                                while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
                                    response.append(buffer, bytesRead);
                                InternetCloseHandle(hRequest); InternetCloseHandle(hConnect); InternetCloseHandle(hSession);
                                vm.push(response); return;
                            }
                            InternetCloseHandle(hRequest);
                        }
                        InternetCloseHandle(hConnect);
                    }
                    InternetCloseHandle(hSession);
                }
#endif
                vm.push(std::string(""));
            } else if (subName == "download") {
                std::string filePath = Runtime::valueToString(vm.pop());
                vm.pop(); // pop net object

#ifdef _WIN32
                HINTERNET hSession = InternetOpenW(L"FluxAgent", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
                if (hSession) {
                    HINTERNET hRequest = InternetOpenUrlW(hSession, utf8ToWide(url).c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
                    if (hRequest) {
                        DWORD statusCode = 0; DWORD statusSize = sizeof(statusCode);
                        HttpQueryInfoW(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusSize, NULL);
                        g_netLastStatus = (int)statusCode;
                        std::ofstream outFile(filePath, std::ios::binary);
                        char buffer[4096]; DWORD bytesRead;
                        while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
                            if (outFile) outFile.write(buffer, bytesRead);
                        InternetCloseHandle(hRequest);
                        InternetCloseHandle(hSession);
                        vm.push(0); return;
                    }
                    InternetCloseHandle(hSession);
                }
#endif
                vm.push(0);
            } else { vm.pop(); vm.pop(); vm.push(0); }
        }
    }
}
