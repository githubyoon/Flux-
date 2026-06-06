#include "Interpreter.h"
#include <iostream>
#include <cmath>
#include <thread>
#include <chrono>
#include <random>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <ctime>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#pragma comment(lib, "user32.lib")

namespace Flux { class Interpreter; }
static Flux::Interpreter* g_activeInterpreter = nullptr;
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
            if (HIWORD(wp) == BN_CLICKED && g_buttonCallbacks.count(id) && g_activeInterpreter) {
                g_activeInterpreter->callFluxFunction(g_buttonCallbacks[id]);
            }
            break;
        }
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
#endif

#include "Lexer.h"
#include "Parser.h"

namespace fs = std::filesystem;

namespace Flux {

class ReturnException : public std::exception {
public:
    Runtime::Value value;
    ReturnException(Runtime::Value v) : value(v) {}
};

class BreakException : public std::exception {};

class FluxException : public std::exception {
public:
    Runtime::Value value;
    FluxException(Runtime::Value v) : value(v) {}
};

inline bool valuesEqual(const Runtime::Value& l, const Runtime::Value& r) {
    if (l.index() != r.index()) {
        if ((std::holds_alternative<int>(l) || std::holds_alternative<float>(l)) &&
            (std::holds_alternative<int>(r) || std::holds_alternative<float>(r))) {
            double v1 = std::holds_alternative<int>(l) ? (double)std::get<int>(l) : (double)std::get<float>(l);
            double v2 = std::holds_alternative<int>(r) ? (double)std::get<int>(r) : (double)std::get<float>(r);
            return v1 == v2;
        }
        return false;
    }
    return l == r;
}

Interpreter::Interpreter() {
    globals = std::make_shared<Environment>();
    environment = globals;
    g_activeInterpreter = this;
}

void Interpreter::interpret(AST::Program& program) {
    try {
        for (auto& stmt : program.statements) {
            if (auto* func = dynamic_cast<AST::FunctionDef*>(stmt.get())) functions[func->name] = func;
            else if (auto* sd = dynamic_cast<AST::StructDef*>(stmt.get())) structs[sd->name] = sd;
            else if (auto* cd = dynamic_cast<AST::ClassDef*>(stmt.get())) classes[cd->name] = cd;
        }
        for (auto& stmt : program.statements) {
            if (!dynamic_cast<AST::FunctionDef*>(stmt.get()) && 
                !dynamic_cast<AST::StructDef*>(stmt.get()) && 
                !dynamic_cast<AST::ClassDef*>(stmt.get())) {
                execute(*stmt);
            }
        }
        if (functions.count("main")) {
            auto* mainFunc = functions["main"];
            auto previousEnv = environment;
            environment = std::make_shared<Environment>(globals);
            try {
                for (auto& stmt : mainFunc->body) execute(*stmt);
            } catch (const ReturnException&) {}
            environment = previousEnv;
        }
    } catch (const FluxException& e) {
        std::cerr << "Uncaught Exception: " << Runtime::valueToString(e.value) << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
    }
}

void Interpreter::execute(AST::Statement& stmt) {
    if (auto* varDecl = dynamic_cast<AST::VarDeclaration*>(&stmt)) {
        for (const auto& name : varDecl->names) {
            if (varDecl->isArray) {
                auto arr = std::make_shared<Runtime::Array>();
                arr->elementType = varDecl->type; environment->define(name, arr);
            } else {
                if (varDecl->type == "int") environment->define(name, 0);
                else if (varDecl->type == "float") environment->define(name, 0.0f);
                else if (varDecl->type == "string") environment->define(name, std::string(""));
                else if (varDecl->type == "bool") environment->define(name, false);
                else environment->define(name, std::shared_ptr<Runtime::Object>(nullptr));
            }
        }
    } else if (auto* exprStmt = dynamic_cast<AST::ExpressionStmt*>(&stmt)) {
        evaluate(*exprStmt->expr);
    } else if (auto* block = dynamic_cast<AST::BlockStmt*>(&stmt)) {
        auto previous = environment; environment = std::make_shared<Environment>(previous);
        for (auto& s : block->statements) execute(*s);
        environment = previous;
    } else if (auto* ifStmt = dynamic_cast<AST::IfStmt*>(&stmt)) {
        auto condition = evaluate(*ifStmt->condition);
        bool isTrue = (std::holds_alternative<bool>(condition) && std::get<bool>(condition)) || (std::holds_alternative<int>(condition) && std::get<int>(condition) != 0);
        if (isTrue) execute(*ifStmt->thenBranch);
        else if (ifStmt->elseBranch) execute(*ifStmt->elseBranch);
    } else if (auto* whileStmt = dynamic_cast<AST::WhileStmt*>(&stmt)) {
        try {
            while (true) {
                auto condition = evaluate(*whileStmt->condition);
                bool isTrue = (std::holds_alternative<bool>(condition) && std::get<bool>(condition)) || (std::holds_alternative<int>(condition) && std::get<int>(condition) != 0);
                if (!isTrue) break;
                execute(*whileStmt->body);
            }
        } catch (const BreakException&) {}
    } else if (auto* forStmt = dynamic_cast<AST::ForStmt*>(&stmt)) {
        auto previous = environment; environment = std::make_shared<Environment>(previous);
        if (forStmt->init) execute(*forStmt->init);
        try {
            while (true) {
                if (forStmt->condition) {
                    auto condition = evaluate(*forStmt->condition);
                    bool isTrue = (std::holds_alternative<bool>(condition) && std::get<bool>(condition)) || (std::holds_alternative<int>(condition) && std::get<int>(condition) != 0);
                    if (!isTrue) break;
                }
                execute(*forStmt->body);
                if (forStmt->update) evaluate(*forStmt->update);
            }
        } catch (const BreakException&) {}
        environment = previous;
    } else if (auto* tryCatch = dynamic_cast<AST::TryCatchStmt*>(&stmt)) {
        try { execute(*tryCatch->tryBlock); }
        catch (const FluxException& e) {
            auto previous = environment; environment = std::make_shared<Environment>(previous);
            environment->define(tryCatch->exceptionVarName, e.value);
            execute(*tryCatch->catchBlock); environment = previous;
        }
    } else if (auto* throwStmt = dynamic_cast<AST::ThrowStmt*>(&stmt)) {
        throw FluxException(evaluate(*throwStmt->value));
    } else if (auto* returnStmt = dynamic_cast<AST::ReturnStmt*>(&stmt)) {
        throw ReturnException(returnStmt->value ? evaluate(*returnStmt->value) : 0);
    } else if (dynamic_cast<AST::BreakStmt*>(&stmt)) {
        throw BreakException();
    } else if (auto* switchStmt = dynamic_cast<AST::SwitchStmt*>(&stmt)) {
        auto discVal = evaluate(*switchStmt->discriminant);
        bool matched = false;
        for (auto& c : switchStmt->cases) {
            if (!matched && (c.value == nullptr || valuesEqual(discVal, evaluate(*c.value)))) matched = true;
            if (matched) { try { for (auto& s : c.body) execute(*s); } catch (const BreakException&) { break; } }
        }
    } else if (auto* importStmt = dynamic_cast<AST::ImportStmt*>(&stmt)) {
        importedModules.insert(importStmt->moduleName);
    }
}

Runtime::Value Interpreter::evaluate(AST::Expression& expr) {
    if (auto* intLit = dynamic_cast<AST::IntLiteral*>(&expr)) return intLit->value;
    if (auto* floatLit = dynamic_cast<AST::FloatLiteral*>(&expr)) return floatLit->value;
    if (auto* strLit = dynamic_cast<AST::StringLiteral*>(&expr)) return strLit->value;
    if (auto* boolLit = dynamic_cast<AST::BooleanLiteral*>(&expr)) return boolLit->value;
    if (auto* varExpr = dynamic_cast<AST::VariableExpr*>(&expr)) return environment->get(varExpr->name);
    if (dynamic_cast<AST::ThisExpr*>(&expr)) return environment->get("this");

    if (auto* assign = dynamic_cast<AST::Assignment*>(&expr)) {
        auto val = evaluate(*assign->value); environment->assign(assign->name, val); return val;
    }
    if (auto* memAcc = dynamic_cast<AST::MemberAccessExpr*>(&expr)) {
        auto objVal = evaluate(*memAcc->object);
        if (std::holds_alternative<std::shared_ptr<Runtime::Object>>(objVal)) {
            auto obj = std::get<std::shared_ptr<Runtime::Object>>(objVal);
            if (!obj) throw std::runtime_error("Null pointer access to '" + memAcc->memberName + "'");
            return obj->members[memAcc->memberName];
        }
        throw std::runtime_error("Member access on non-object type.");
    }
    if (auto* memAssign = dynamic_cast<AST::MemberAssignment*>(&expr)) {
        auto objVal = evaluate(*memAssign->memberAccess->object);
        auto obj = std::get<std::shared_ptr<Runtime::Object>>(objVal);
        if (!obj) throw std::runtime_error("Null pointer assignment to '" + memAssign->memberAccess->memberName + "'");
        auto newVal = evaluate(*memAssign->value);
        obj->members[memAssign->memberAccess->memberName] = newVal; return newVal;
    }
    if (auto* newExpr = dynamic_cast<AST::NewExpr*>(&expr)) {
        auto obj = std::make_shared<Runtime::Object>(); obj->typeName = newExpr->className;
        if (structs.count(newExpr->className)) {
            for (auto& m : structs[newExpr->className]->members) {
                if (m.type == "int") obj->members[m.name] = 0; else if (m.type == "float") obj->members[m.name] = 0.0f;
                else if (m.type == "bool") obj->members[m.name] = false; else obj->members[m.name] = std::string("");
            }
        } else if (classes.count(newExpr->className)) {
            for (auto& p : classes[newExpr->className]->properties) {
                for (const auto& n : p->names) {
                    if (p->type == "int") obj->members[n] = 0; else if (p->type == "float") obj->members[n] = 0.0f;
                    else if (p->type == "bool") obj->members[n] = false; else obj->members[n] = std::string("");
                }
            }
        }
        return obj;
    }
    if (auto* binary = dynamic_cast<AST::BinaryExpr*>(&expr)) {
        if (binary->op.type == TokenType::T_AND_AND || binary->op.type == TokenType::T_OR_OR) {
            auto l = evaluate(*binary->left);
            bool lb = (std::holds_alternative<bool>(l) && std::get<bool>(l)) || (std::holds_alternative<int>(l) && std::get<int>(l) != 0);
            if (binary->op.type == TokenType::T_AND_AND) { if (!lb) return false; } else { if (lb) return true; }
            auto r = evaluate(*binary->right);
            return (std::holds_alternative<bool>(r) && std::get<bool>(r)) || (std::holds_alternative<int>(r) && std::get<int>(r) != 0);
        }
        auto left = evaluate(*binary->left); auto right = evaluate(*binary->right);
        auto getD = [](const Runtime::Value& v) { return std::holds_alternative<int>(v) ? (double)std::get<int>(v) : (double)std::get<float>(v); };
        switch (binary->op.type) {
            case TokenType::T_EQUAL_EQUAL: return valuesEqual(left, right);
            case TokenType::T_BANG_EQUAL: return !valuesEqual(left, right);
            case TokenType::T_GREATER: return getD(left) > getD(right);
            case TokenType::T_GREATER_EQUAL: return getD(left) >= getD(right);
            case TokenType::T_LESS: return getD(left) < getD(right);
            case TokenType::T_LESS_EQUAL: return getD(left) <= getD(right);
            case TokenType::T_PLUS:
                if (std::holds_alternative<std::string>(left) || std::holds_alternative<std::string>(right)) return Runtime::valueToString(left) + Runtime::valueToString(right);
                if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) return std::get<int>(left) + std::get<int>(right);
                return (float)(getD(left) + getD(right));
            case TokenType::T_MINUS: if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) return std::get<int>(left) - std::get<int>(right); return (float)(getD(left) - getD(right));
            case TokenType::T_STAR: if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) return std::get<int>(left) * std::get<int>(right); return (float)(getD(left) * getD(right));
            case TokenType::T_SLASH: return (float)(getD(left) / getD(right));
            default: break;
        }
    }
    if (auto* printfExpr = dynamic_cast<AST::PrintfExpr*>(&expr)) {
        std::string res; for (const auto& p : printfExpr->parts) res += p.isExpression ? Runtime::valueToString(evaluate(*p.expr)) : p.text;
        std::cout << res << std::endl; return 0;
    }
    if (auto* call = dynamic_cast<AST::CallExpr*>(&expr)) {
        if (call->callee.find('.') != std::string::npos) {
            size_t dotPos = call->callee.find('.');
            std::string objName = call->callee.substr(0, dotPos);
            std::string subName = call->callee.substr(dotPos + 1);
            Runtime::Value objVal; bool isObj = false;
            try { objVal = environment->get(objName); isObj = std::holds_alternative<std::shared_ptr<Runtime::Object>>(objVal); } catch(...) {}
            if (isObj) {
                auto obj = std::get<std::shared_ptr<Runtime::Object>>(objVal);
                if (obj && classes.count(obj->typeName)) {
                    for (auto& m : classes[obj->typeName]->methods) {
                        if (m->name == subName) {
                            auto env = std::make_shared<Environment>(globals); env->define("this", obj);
                            for (size_t i=0; i<call->args.size() && i<m->params.size(); ++i) env->define(m->params[i].name, evaluate(*call->args[i]));
                            auto prev = environment; environment = env; Runtime::Value res = 0;
                            try { for (auto& s : m->body) execute(*s); } catch (const ReturnException& e) { res = e.value; }
                            environment = prev; return res;
                        }
                    }
                }
            } else if (importedModules.count(objName)) {
                if (objName == "gui") {
#ifdef _WIN32
                    if (subName == "msgbox") {
                        MessageBoxW(g_hWnd, utf8ToWide(Runtime::valueToString(evaluate(*call->args[1]))).c_str(), utf8ToWide(Runtime::valueToString(evaluate(*call->args[0]))).c_str(), MB_OK | MB_ICONINFORMATION); return 0;
                    }
                    if (subName == "window") {
                        WNDCLASSW wc = {0}; wc.lpfnWndProc = FluxWndProc; wc.hInstance = GetModuleHandle(NULL); wc.lpszClassName = L"FluxWindowClass";
                        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); RegisterClassW(&wc);
                        g_hWnd = CreateWindowExW(0, wc.lpszClassName, utf8ToWide(Runtime::valueToString(evaluate(*call->args[0]))).c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, std::get<int>(evaluate(*call->args[1])), std::get<int>(evaluate(*call->args[2])), NULL, NULL, wc.hInstance, NULL); return 0;
                    }
                    if (subName == "button") {
                        std::wstring text = utf8ToWide(Runtime::valueToString(evaluate(*call->args[0])));
                        int x = std::get<int>(evaluate(*call->args[1])), y = std::get<int>(evaluate(*call->args[2]));
                        int w = std::get<int>(evaluate(*call->args[3])), h = std::get<int>(evaluate(*call->args[4]));
                        std::string callback = Runtime::valueToString(evaluate(*call->args[5]));
                        int id = g_nextControlId++; g_buttonCallbacks[id] = callback;
                        CreateWindowExW(0, L"BUTTON", text.c_str(), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, x, y, w, h, g_hWnd, (HMENU)id, GetModuleHandle(NULL), NULL); return 0;
                    }
                    if (subName == "label") {
                        std::wstring text = utf8ToWide(Runtime::valueToString(evaluate(*call->args[0])));
                        int x = std::get<int>(evaluate(*call->args[1])), y = std::get<int>(evaluate(*call->args[2]));
                        int w = std::get<int>(evaluate(*call->args[3])), h = std::get<int>(evaluate(*call->args[4]));
                        CreateWindowExW(0, L"STATIC", text.c_str(), WS_VISIBLE | WS_CHILD | SS_LEFT, x, y, w, h, g_hWnd, NULL, GetModuleHandle(NULL), NULL); return 0;
                    }
                    if (subName == "loop") { MSG msg; while (GetMessageW(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); } return 0; }
#endif
                } else if (objName == "system") {
                    if (subName == "shell") return std::system(Runtime::valueToString(evaluate(*call->args[0])).c_str());
                    if (subName == "exit") std::exit(call->args.empty() ? 0 : std::get<int>(evaluate(*call->args[0])));
                } else if (objName == "console" && subName == "color") {
                    std::string c = Runtime::valueToString(evaluate(*call->args[0]));
                    if (c == "red") std::cout << "\033[31m"; else if (c == "reset") std::cout << "\033[0m"; return 0;
                }
            }
        }
        if (call->callee == "print") { std::cout << Runtime::valueToString(evaluate(*call->args[0])) << std::endl; return 0; }
        if (functions.count(call->callee)) {
            auto* f = functions[call->callee]; auto sub = std::make_shared<Environment>(globals);
            for (size_t i=0; i<call->args.size() && i<f->params.size(); ++i) sub->define(f->params[i].name, evaluate(*call->args[i]));
            auto prev = environment; environment = sub; Runtime::Value r = 0;
            try { for (auto& s : f->body) execute(*s); } catch (const ReturnException& e) { r = e.value; }
            environment = prev; return r;
        }
    }
    return 0;
}

void Interpreter::print(const std::vector<Runtime::Value>& args) {
    for (const auto& a : args) std::cout << Runtime::valueToString(a);
    std::cout << std::endl;
}

void Interpreter::callFluxFunction(const std::string& name) {
    if (functions.count(name)) {
        auto* f = functions[name];
        auto sub = std::make_shared<Environment>(globals);
        auto prev = environment;
        environment = sub;
        try { for (auto& s : f->body) execute(*s); } 
        catch (const std::exception& e) { std::cerr << "Callback Error: " << e.what() << std::endl; }
        catch (...) { std::cerr << "Unknown Callback Error" << std::endl; }
        environment = prev;
    }
}

} // namespace Flux
