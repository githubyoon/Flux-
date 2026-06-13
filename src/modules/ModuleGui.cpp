#ifdef _WIN32
#include "ModuleGui.h"
#include "VM.h"
#include "ModuleUtil.h"
#include <string>
#include <map>
#pragma comment(lib, "user32.lib")

namespace Flux::Modules {
    namespace {
        VM* g_activeVM = nullptr;
        std::map<int, std::string> g_buttonCallbacks;
        int g_nextControlId = 1000;
        HWND g_hWnd = NULL;

        LRESULT CALLBACK FluxWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
            switch (msg) {
                case WM_COMMAND: {
                    int id = LOWORD(wp);
                    if (HIWORD(wp) == BN_CLICKED && g_buttonCallbacks.count(id) && g_activeVM) {
                        g_activeVM->callFluxFunction(g_buttonCallbacks[id]);
                    }
                    break;
                }
                case WM_DESTROY: PostQuitMessage(0); return 0;
            }
            return DefWindowProcW(hwnd, msg, wp, lp);
        }
    }

    void guiSetActiveVM(VM* vm) { g_activeVM = vm; }

    void handleGui(const std::string& subName, VM& vm) {
        if (subName == "msgbox") {
            auto m = Runtime::valueToString(vm.pop()); auto t = Runtime::valueToString(vm.pop());
            MessageBoxW(g_hWnd, utf8ToWide(m).c_str(), utf8ToWide(t).c_str(), MB_OK);
        } else if (subName == "window") {
            int h = std::get<int>(vm.pop()); int w = std::get<int>(vm.pop()); std::string t = Runtime::valueToString(vm.pop());
            WNDCLASSW wc = {0}; wc.lpfnWndProc = FluxWndProc; wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = L"FluxWindowClass"; wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); RegisterClassW(&wc);
            g_hWnd = CreateWindowExW(0, wc.lpszClassName, utf8ToWide(t).c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, w, h, NULL, NULL, wc.hInstance, NULL);
        } else if (subName == "button") {
            std::string callback = Runtime::valueToString(vm.pop());
            int h = std::get<int>(vm.pop()); int w = std::get<int>(vm.pop());
            int y = std::get<int>(vm.pop()); int x = std::get<int>(vm.pop());
            std::string t = Runtime::valueToString(vm.pop());
            int id = g_nextControlId++;
            g_buttonCallbacks[id] = callback;
            CreateWindowExW(0, L"BUTTON", utf8ToWide(t).c_str(), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, x, y, w, h, g_hWnd, (HMENU)(intptr_t)id, GetModuleHandle(NULL), NULL);
        } else if (subName == "loop") { MSG msg; while (GetMessageW(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); } }
        else if (subName == "label") {
            int h = std::get<int>(vm.pop()); int w = std::get<int>(vm.pop());
            int y = std::get<int>(vm.pop()); int x = std::get<int>(vm.pop());
            std::string text = Runtime::valueToString(vm.pop());
            CreateWindowExW(0, L"STATIC", utf8ToWide(text).c_str(), WS_VISIBLE | WS_CHILD, x, y, w, h, g_hWnd, NULL, GetModuleHandle(NULL), NULL);
        } else if (subName == "close") {
            DestroyWindow(g_hWnd); g_hWnd = NULL;
        } else if (subName == "setTitle") {
            std::string t = Runtime::valueToString(vm.pop());
            SetWindowTextW(g_hWnd, utf8ToWide(t).c_str());
        } else if (subName == "getX") {
            RECT rect; GetWindowRect(g_hWnd, &rect);
            vm.pop(); vm.push(rect.left); return;
        } else if (subName == "getY") {
            RECT rect; GetWindowRect(g_hWnd, &rect);
            vm.pop(); vm.push(rect.top); return;
        } else if (subName == "show") {
            ShowWindow(g_hWnd, SW_SHOW);
        }
        vm.pop(); vm.push(0);
    }
}
#endif
