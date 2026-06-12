#include "VM.h"
#include "OpCode.h"
#include "AST.h"
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <thread>
#include <chrono>
#include <algorithm>
#include <ctime>
#include <filesystem>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "wininet.lib")

namespace Flux { class VM; }
static Flux::VM* g_activeVM = nullptr;
static std::map<int, std::string> g_buttonCallbacks;
static int g_nextControlId = 1000;
static HWND g_hWnd = NULL;

static std::wstring utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

static LRESULT CALLBACK FluxWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
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
#endif

namespace Flux {

VM::VM() : frameCount(0), stackTop(stack) {
#ifdef _WIN32
    g_activeVM = this;
#endif
}

InterpretResult VM::interpret(Runtime::Chunk* chunk) {
    frameCount = 0;
    stackTop = stack;
    
    CallFrame* frame = &frames[frameCount++];
    frame->chunk = chunk;
    frame->ip = chunk->code.data();
    frame->slots = stack;

    return run();
}

void VM::printStackTrace() {
    std::cerr << "Stack Trace:" << std::endl;
    for (int i = frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &frames[i];
        std::cerr << "  at [IP: " << (frame->ip - frame->chunk->code.data()) << "]" << std::endl;
    }
}

InterpretResult VM::run() {
    for (;;) {
        try {
            uint8_t instruction = readByte();
            switch (instruction) {
                case OP_CONSTANT: push(readConstant()); break;
                case OP_NULL:  push(std::shared_ptr<Runtime::Object>(nullptr)); break;
                case OP_TRUE:  push(true); break;
                case OP_FALSE: push(false); break;

                case OP_EQUAL: {
                    auto b = pop(); auto a = pop();
                    push(a == b); break;
                }
                case OP_GREATER: {
                    auto b = pop(); auto a = pop();
                    double v1 = std::holds_alternative<int>(a) ? (double)std::get<int>(a) : std::get<float>(a);
                    double v2 = std::holds_alternative<int>(b) ? (double)std::get<int>(b) : std::get<float>(b);
                    push(v1 > v2); break;
                }
                case OP_LESS: {
                    auto b = pop(); auto a = pop();
                    double v1 = std::holds_alternative<int>(a) ? (double)std::get<int>(a) : std::get<float>(a);
                    double v2 = std::holds_alternative<int>(b) ? (double)std::get<int>(b) : std::get<float>(b);
                    push(v1 < v2); break;
                }
                case OP_ADD: {
                    auto b = pop(); auto a = pop();
                    if (std::holds_alternative<int>(a) && std::holds_alternative<int>(b)) push(std::get<int>(a) + std::get<int>(b));
                    else if (std::holds_alternative<std::string>(a) || std::holds_alternative<std::string>(b)) push(Runtime::valueToString(a) + Runtime::valueToString(b));
                    else push((float)((std::holds_alternative<int>(a) ? (double)std::get<int>(a) : (double)std::get<float>(a)) + (std::holds_alternative<int>(b) ? (double)std::get<int>(b) : (double)std::get<float>(b))));
                    break;
                }
                case OP_SUBTRACT: {
                    auto b = pop(); auto a = pop();
                    if (std::holds_alternative<int>(a) && std::holds_alternative<int>(b)) push(std::get<int>(a) - std::get<int>(b));
                    else push((float)((std::holds_alternative<int>(a) ? (double)std::get<int>(a) : (double)std::get<float>(a)) - (std::holds_alternative<int>(b) ? (double)std::get<int>(b) : (double)std::get<float>(b))));
                    break;
                }
                case OP_MULTIPLY: {
                    auto b = pop(); auto a = pop();
                    if (std::holds_alternative<int>(a) && std::holds_alternative<int>(b)) push(std::get<int>(a) * std::get<int>(b));
                    else push((float)((std::holds_alternative<int>(a) ? (double)std::get<int>(a) : (double)std::get<float>(a)) * (std::holds_alternative<int>(b) ? (double)std::get<int>(b) : (double)std::get<float>(b))));
                    break;
                }
                case OP_DIVIDE: {
                    auto b = pop(); auto a = pop();
                    push((float)((std::holds_alternative<int>(a) ? (double)std::get<int>(a) : (double)std::get<float>(a)) / (std::holds_alternative<int>(b) ? (double)std::get<int>(b) : (double)std::get<float>(b))));
                    break;
                }
                case OP_NOT: {
                    auto val = pop();
                    if (std::holds_alternative<bool>(val)) push(!std::get<bool>(val));
                    else if (std::holds_alternative<int>(val)) push(std::get<int>(val) == 0);
                    break;
                }
                case OP_PRINT: std::cout << Runtime::valueToString(pop()) << std::endl; break;
                case OP_POP: pop(); break;

                case OP_DEFINE_GLOBAL: {
                    std::string name = std::get<std::string>(readConstant());
                    globals[name] = pop(); break;
                }
                case OP_GET_GLOBAL: {
                    std::string name = std::get<std::string>(readConstant());
                    if (globals.count(name)) push(globals[name]);
                    else if (name.find('.') != std::string::npos) {
                        size_t dot = name.find('.');
                        std::string objName = name.substr(0, dot);
                        std::string memName = name.substr(dot+1);
                        if (globals.count(objName) && std::holds_alternative<std::shared_ptr<Runtime::Object>>(globals[objName])) {
                            push(std::get<std::shared_ptr<Runtime::Object>>(globals[objName])->members[memName]);
                        } else push(name); 
                    } else throw std::runtime_error("Undefined variable '" + name + "'");
                    break;
                }
                case OP_SET_GLOBAL: {
                    std::string name = std::get<std::string>(readConstant());
                    globals[name] = peek(0); break;
                }
                case OP_GET_LOCAL: push(frames[frameCount - 1].slots[readByte()]); break;
                case OP_SET_LOCAL: frames[frameCount - 1].slots[readByte()] = peek(0); break;

                case OP_JUMP: {
                    uint16_t offset = (uint16_t)((readByte() << 8) | readByte());
                    frames[frameCount - 1].ip += offset; break;
                }
                case OP_JUMP_IF_FALSE: {
                    uint16_t offset = (uint16_t)((readByte() << 8) | readByte());
                    auto condition = peek(0);
                    bool isTrue = (std::holds_alternative<bool>(condition) && std::get<bool>(condition)) || (std::holds_alternative<int>(condition) && std::get<int>(condition) != 0);
                    if (!isTrue) frames[frameCount - 1].ip += offset;
                    break;
                }
                case OP_LOOP: {
                    uint16_t offset = (uint16_t)((readByte() << 8) | readByte());
                    frames[frameCount - 1].ip -= offset; break;
                }
                case OP_CALL: {
                    int argCount = readByte();
                    auto callee = peek(argCount);
                    if (std::holds_alternative<std::shared_ptr<Runtime::ObjFunction>>(callee)) {
                        auto func = std::get<std::shared_ptr<Runtime::ObjFunction>>(callee);
                        if (frameCount == FRAMES_MAX) throw std::runtime_error("Stack overflow.");
                        CallFrame* frame = &frames[frameCount++];
                        frame->chunk = func->chunk.get();
                        frame->ip = func->chunk->code.data();
                        frame->slots = stackTop - argCount - 1;
                    } else if (std::holds_alternative<std::string>(callee)) {
                        std::string fullName = std::get<std::string>(callee);
                        if (fullName == "print") {
                             std::cout << Runtime::valueToString(peek(0)) << std::endl;
                             pop(); pop(); push(0);
                        } else if (fullName.find('.') != std::string::npos) {
                            size_t dot = fullName.find('.');
                            std::string objName = fullName.substr(0, dot);
                            std::string subName = fullName.substr(dot+1);
                            
                            if (objName == "math") {
                                double v = std::holds_alternative<int>(peek(0)) ? (double)std::get<int>(peek(0)) : (double)std::get<float>(peek(0));
                                if (subName == "abs") { pop(); pop(); push((float)std::abs(v)); }
                                else if (subName == "round") { pop(); pop(); push((int)std::round(v)); }
                                else if (subName == "sin") { pop(); pop(); push((float)std::sin(v)); }
                                else if (subName == "cos") { pop(); pop(); push((float)std::cos(v)); }
                                else if (subName == "tan") { pop(); pop(); push((float)std::tan(v)); }
                                else if (subName == "sqrt") { pop(); pop(); push((float)std::sqrt(v)); }
                                else if (subName == "pow") { 
                                    double base = std::holds_alternative<int>(peek(1)) ? (double)std::get<int>(peek(1)) : (double)std::get<float>(peek(1));
                                    pop(); pop(); pop(); push((float)std::pow(base, v)); 
                                }
                                else if (subName == "log") { pop(); pop(); push((float)std::log(v)); }
                                else if (subName == "pi") { pop(); push(3.14159265f); }
                                else if (subName == "e") { pop(); push(2.71828182f); }
                                else { pop(); pop(); push(0); }
                                break;
                            }

                            if (objName == "console") {
                                if (subName == "clear") {
#ifdef _WIN32
                                    system("cls");
#else
                                    system("clear");
#endif
                                } else if (subName == "title") {
#ifdef _WIN32
                                    SetConsoleTitleW(utf8ToWide(Runtime::valueToString(peek(0))).c_str());
#endif
                                } else if (subName == "color") {
#ifdef _WIN32
                                    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)std::get<int>(peek(0)));
#endif
                                } else if (subName == "input") {
                                    std::string prompt = Runtime::valueToString(peek(0));
                                    std::cout << prompt;
                                    std::string input;
                                    std::getline(std::cin, input);
                                    pop(); pop(); push(input); break;
                                }
                                pop(); pop(); push(0); break;
                            }

                            if (objName == "file") {
                                std::string path = Runtime::valueToString(peek(0));
                                if (subName == "read") {
                                    std::ifstream f(path);
                                    if (f) {
                                        std::stringstream b; b << f.rdbuf();
                                        pop(); pop(); push(b.str());
                                    } else { pop(); pop(); push(std::string("")); }
                                } else if (subName == "write") {
                                    std::string content = Runtime::valueToString(peek(1));
                                    std::ofstream f(path);
                                    if (f) f << content;
                                    pop(); pop(); pop(); push(0);
                                } else if (subName == "append") {
                                    std::string content = Runtime::valueToString(peek(1));
                                    std::ofstream f(path, std::ios::app);
                                    if (f) f << content;
                                    pop(); pop(); pop(); push(0);
                                } else if (subName == "exists") {
                                    pop(); pop(); push(std::filesystem::exists(path));
                                } else if (subName == "remove") {
                                    std::filesystem::remove(path);
                                    pop(); pop(); push(0);
                                } else if (subName == "size") {
                                    pop(); pop(); push((int)std::filesystem::file_size(path));
                                } else if (subName == "is_dir") {
                                    pop(); pop(); push(std::filesystem::is_directory(path));
                                } else if (subName == "mkdir") {
                                    std::filesystem::create_directories(path);
                                    pop(); pop(); push(0);
                                } else { pop(); pop(); push(0); }
                                break;
                            }

                            if (objName == "time") {
                                if (subName == "sleep") {
                                    int ms = std::get<int>(peek(0));
                                    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
                                } else if (subName == "now") {
                                    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                                    char buf[26]; ctime_s(buf, sizeof(buf), &now);
                                    std::string s(buf); if (!s.empty()) s.pop_back();
                                    pop(); push(s); break;
                                } else if (subName == "ticks") {
                                    auto now = std::chrono::steady_clock::now();
                                    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                                    pop(); push((int)ms); break;
                                }
                                pop(); pop(); push(0); break;
                            }

                            if (objName == "system") {
                                if (subName == "exit") std::exit(std::get<int>(pop()));
                                else if (subName == "run") {
                                    std::string cmd = Runtime::valueToString(pop());
                                    pop(); push(system(cmd.c_str()));
                                } else if (subName == "os") {
#ifdef _WIN32
                                    pop(); push(std::string("windows"));
#else
                                    pop(); push(std::string("linux"));
#endif
                                } else if (subName == "env") {
                                    std::string key = Runtime::valueToString(pop());
                                    char* val = nullptr;
                                    size_t len = 0;
                                    _dupenv_s(&val, &len, key.c_str());
                                    std::string res = val ? val : "";
                                    free(val);
                                    pop(); push(res);
                                } else { pop(); pop(); push(0); }
                                break;
                            }

    #ifdef _WIN32
                            else if (objName == "gui") {
                                if (subName == "msgbox") {
                                    auto m = Runtime::valueToString(pop()); auto t = Runtime::valueToString(pop());
                                    MessageBoxW(g_hWnd, utf8ToWide(m).c_str(), utf8ToWide(t).c_str(), MB_OK);
                                } else if (subName == "window") {
                                    int h = std::get<int>(pop()); int w = std::get<int>(pop()); std::string t = Runtime::valueToString(pop());
                                    WNDCLASSW wc = {0}; wc.lpfnWndProc = FluxWndProc; wc.hInstance = GetModuleHandle(NULL);
                                    wc.lpszClassName = L"FluxWindowClass"; wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); RegisterClassW(&wc);
                                    g_hWnd = CreateWindowExW(0, wc.lpszClassName, utf8ToWide(t).c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, w, h, NULL, NULL, wc.hInstance, NULL);
                                } else if (subName == "button") {
                                    std::string callback = Runtime::valueToString(pop());
                                    int h = std::get<int>(pop()); int w = std::get<int>(pop());
                                    int y = std::get<int>(pop()); int x = std::get<int>(pop());
                                    std::string t = Runtime::valueToString(pop());
                                    int id = g_nextControlId++;
                                    g_buttonCallbacks[id] = callback;
                                    CreateWindowExW(0, L"BUTTON", utf8ToWide(t).c_str(), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, x, y, w, h, g_hWnd, (HMENU)(intptr_t)id, GetModuleHandle(NULL), NULL);
                                } else if (subName == "loop") { MSG msg; while (GetMessageW(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); } }
                                pop(); push(0);
                            } 
    #endif
                            else if (objName == "net") {
                                std::string url = Runtime::valueToString(pop());
                                if (subName == "get" || subName == "post") {
                                    std::string data = (subName == "post") ? Runtime::valueToString(pop()) : "";
                                    pop(); // pop net object

                                    HINTERNET hSession = InternetOpenW(L"FluxAgent", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
                                    if (hSession) {
                                        HINTERNET hConnect = InternetOpenUrlW(hSession, utf8ToWide(url).c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
                                        if (hConnect) {
                                            std::string response;
                                            char buffer[4096];
                                            DWORD bytesRead;
                                            while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
                                                response.append(buffer, bytesRead);
                                            }
                                            InternetCloseHandle(hConnect);
                                            InternetCloseHandle(hSession);
                                            push(response);
                                            break;
                                        }
                                        InternetCloseHandle(hSession);
                                    }
                                    push(std::string(""));
                                } else { pop(); pop(); push(0); }
                            }
                        }
                    } else throw std::runtime_error("Can only call functions.");
                    break;
                }
                case OP_NEW: {
                    std::string name = std::get<std::string>(readConstant());
                    auto obj = std::make_shared<Runtime::Object>(); obj->typeName = name;
                    if (structs.count(name)) {
                        for (auto& m : structs[name]->members) {
                            if (m.type == "int") obj->members[m.name] = 0;
                            else if (m.type == "float") obj->members[m.name] = 0.0f;
                            else if (m.type == "bool") obj->members[m.name] = false;
                            else if (m.type == "string") obj->members[m.name] = std::string("");
                            else obj->members[m.name] = std::shared_ptr<Runtime::Object>(nullptr);
                        }
                    } else if (classes.count(name)) {
                        for (auto& p : classes[name]->properties) {
                            for (auto& decl : p->decls) {
                                 if (p->type == "int") obj->members[decl->name] = 0;
                                 else if (p->type == "float") obj->members[decl->name] = 0.0f;
                                 else if (p->type == "bool") obj->members[decl->name] = false;
                                 else if (p->type == "string") obj->members[decl->name] = std::string("");
                                 else obj->members[decl->name] = std::shared_ptr<Runtime::Object>(nullptr);
                            }
                        }
                    }


                    push(obj); break;
                }
                case OP_GET_PROPERTY: {
                    auto objVal = pop(); std::string name = std::get<std::string>(readConstant());
                    if (std::holds_alternative<std::shared_ptr<Runtime::Object>>(objVal)) push(std::get<std::shared_ptr<Runtime::Object>>(objVal)->members[name]);
                    else throw std::runtime_error("Property access on non-object.");
                    break;
                }
                case OP_SET_PROPERTY: {
                    auto val = pop(); 
                    auto objVal = pop(); 
                    std::string name = std::get<std::string>(readConstant());
                    
                    if (std::holds_alternative<std::shared_ptr<Runtime::Object>>(objVal)) { 
                        auto obj = std::get<std::shared_ptr<Runtime::Object>>(objVal);
                        if (!obj) throw std::runtime_error("Property assignment on null object '" + name + "'");
                        obj->members[name] = val; push(val); 
                    }
                    else throw std::runtime_error("Property assignment on non-object member: " + name + ". Index: " + std::to_string(objVal.index()));
                    break;
                }
                case OP_NEW_MAP: {
                    push(std::make_shared<Runtime::Map>()); break;
                }
                case OP_GET_MAP: {
                    auto mapVal = pop(); auto keyVal = pop();
                    std::string key = Runtime::valueToString(keyVal);
                    if (std::holds_alternative<std::shared_ptr<Runtime::Map>>(mapVal)) {
                        push(std::get<std::shared_ptr<Runtime::Map>>(mapVal)->elements[key]);
                    } else throw std::runtime_error("Map access on non-map.");
                    break;
                }
                case OP_SET_MAP: {
                    auto val = pop(); auto mapVal = pop(); auto keyVal = pop();
                    std::string key = Runtime::valueToString(keyVal);
                    if (std::holds_alternative<std::shared_ptr<Runtime::Map>>(mapVal)) {
                        std::get<std::shared_ptr<Runtime::Map>>(mapVal)->elements[key] = val; push(val);
                    } else throw std::runtime_error("Map assignment on non-map.");
                    break;
                }
                case OP_DEFINE_STRUCT: {
                    std::string name = std::get<std::string>(readConstant()); structs[name] = (AST::StructDef*)std::get<void*>(readConstant()); break;
                }
                case OP_DEFINE_CLASS: {
                    std::string name = std::get<std::string>(readConstant()); classes[name] = (AST::ClassDef*)std::get<void*>(readConstant()); break;
                }
                case OP_RETURN: {
                    auto result = pop(); frameCount--;
                    if (frameCount == 0) return InterpretResult::INTERPRET_OK;
                    stackTop = frames[frameCount].slots; push(result); break;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Runtime Error: " << e.what() << std::endl;
            printStackTrace();
            return InterpretResult::INTERPRET_RUNTIME_ERROR;
        }
    }
}

void VM::callFluxFunction(const std::string& name) {
    if (globals.count(name)) {
        auto val = globals[name];
        if (std::holds_alternative<std::shared_ptr<Runtime::ObjFunction>>(val)) {
             auto func = std::get<std::shared_ptr<Runtime::ObjFunction>>(val);
             if (frameCount == FRAMES_MAX) return;
             CallFrame* frame = &frames[frameCount++];
             frame->chunk = func->chunk.get();
             frame->ip = func->chunk->code.data();
             frame->slots = stackTop;
             run();
        }
    }
}

} // namespace Flux
