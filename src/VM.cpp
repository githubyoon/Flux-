#include "VM.h"
#include "OpCode.h"
#include "AST.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>

namespace Flux {

VM::VM() : frameCount(0), stackTop(stack), globals(std::make_shared<std::map<std::string, Runtime::Value>>()),
            globalsMutex(std::make_shared<std::mutex>()) {
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
    int initialFrameCount = frameCount;
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
                    Runtime::Value val = pop();
                    {
                        std::lock_guard<std::mutex> lock(*globalsMutex);
                        (*globals)[name] = val;
                    }
                    break;
                }
                case OP_GET_GLOBAL: {
                    std::string name = std::get<std::string>(readConstant());
                    Runtime::Value result;
                    bool found = false;
                    {
                        std::lock_guard<std::mutex> lock(*globalsMutex);
                        if (globals->count(name)) {
                            result = (*globals)[name];
                            found = true;
                        } else if (name.find('.') != std::string::npos) {
                            size_t dot = name.find('.');
                            std::string objName = name.substr(0, dot);
                            std::string memName = name.substr(dot+1);
                            if (globals->count(objName) && std::holds_alternative<std::shared_ptr<Runtime::Object>>((*globals)[objName])) {
                                result = std::get<std::shared_ptr<Runtime::Object>>((*globals)[objName])->members[memName];
                                found = true;
                            }
                        }
                    }
                    if (found) push(result);
                    else if (name.find('.') != std::string::npos) push(name);
                    else throw std::runtime_error("Undefined variable '" + name + "'");
                    break;
                }
                case OP_SET_GLOBAL: {
                    std::string name = std::get<std::string>(readConstant());
                    Runtime::Value val = peek(0);
                    {
                        std::lock_guard<std::mutex> lock(*globalsMutex);
                        (*globals)[name] = val;
                    }
                    break;
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
                    } else if (std::holds_alternative<std::shared_ptr<Runtime::Method>>(callee)) {
                        auto method = std::get<std::shared_ptr<Runtime::Method>>(callee);
                        switch (method->type) {
                            case Runtime::Method::ARRAY: {
                                auto targetArr = std::get<std::shared_ptr<Runtime::Array>>(method->target);
                                std::string& methodName = method->name;
                                if (methodName == "map") {
                                    auto callback = pop();
                                    auto resultArr = std::make_shared<Runtime::Array>();
                                    for (auto const& el : targetArr->elements) resultArr->elements.push_back(callValue(callback, {el}));
                                    pop(); // pop the method object
                                    push(resultArr); break;
                                } else if (methodName == "filter") {
                                    auto callback = pop();
                                    auto resultArr = std::make_shared<Runtime::Array>();
                                    for (auto const& el : targetArr->elements) {
                                        auto res = callValue(callback, {el});
                                        bool isTrue = (std::holds_alternative<bool>(res) && std::get<bool>(res)) || (std::holds_alternative<int>(res) && std::get<int>(res) != 0);
                                        if (isTrue) resultArr->elements.push_back(el);
                                    }
                                    pop(); push(resultArr); break;
                                } else if (methodName == "reduce") {
                                    auto callback = pop(); auto initial = pop();
                                    Runtime::Value acc = initial;
                                    for (auto const& el : targetArr->elements) acc = callValue(callback, {acc, el});
                                    pop(); push(acc); break;
                                } else if (methodName == "sort") {
                                    std::sort(targetArr->elements.begin(), targetArr->elements.end(), [](const Runtime::Value& a, const Runtime::Value& b) {
                                        if (a.index() != b.index()) return a.index() < b.index();
                                        if (std::holds_alternative<int>(a)) return std::get<int>(a) < std::get<int>(b);
                                        if (std::holds_alternative<float>(a)) return std::get<float>(a) < std::get<float>(b);
                                        if (std::holds_alternative<std::string>(a)) return std::get<std::string>(a) < std::get<std::string>(b);
                                        return false;
                                    });
                                    pop(); push(targetArr); break;
                                } else if (methodName == "len") {
                                    pop(); push((int)targetArr->elements.size()); break;
                                } else if (methodName == "append") {
                                    targetArr->elements.push_back(pop());
                                    pop(); push(0); break;
                                }
                                break;
                            }
                            case Runtime::Method::MAP: {
                                throw std::runtime_error("Map methods not yet implemented");
                            }
                            case Runtime::Method::STRING: {
                                throw std::runtime_error("String methods not yet implemented");
                            }
                        }
                        break;
                    } else if (std::holds_alternative<std::shared_ptr<Runtime::Object>>(callee)) {
                        // Object calls handled elsewhere
                    } else if (std::holds_alternative<std::string>(callee)) {
                        std::string fullName = std::get<std::string>(callee);
                        if (fullName == "print") {
                             std::cout << Runtime::valueToString(peek(0)) << std::endl;
                             pop(); pop(); push(0);
                        }
                    } else throw std::runtime_error("Can only call functions.");
                    break;
                }
                case OP_SPAWN: {
                    int argCount = readByte();
                    // Collect args (last pushed is top)
                    std::vector<Runtime::Value> args;
                    for (int i = argCount - 1; i >= 0; --i)
                        args.push_back(peek(i));
                    Runtime::Value func = peek(argCount);

                    // Pop function + args from stack
                    for (int i = 0; i <= argCount; ++i) pop();

                    // Spawn thread
                    std::thread t([this, func, args]() {
                        VM spawnedVM;
                        {
                            std::lock_guard<std::mutex> lock(*globalsMutex);
                            spawnedVM.globals = globals; // share the same map
                            spawnedVM.globalsMutex = globalsMutex;
                        }
                        spawnedVM.callValue(func, args);
                    });
                    t.detach();

                    push((int)0);
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
                    if (std::holds_alternative<std::shared_ptr<Runtime::Object>>(objVal)) {
                        auto obj = std::get<std::shared_ptr<Runtime::Object>>(objVal);
                        if (!obj) throw std::runtime_error("Property access on null object.");
                        push(obj->members[name]);
                    } else if (std::holds_alternative<std::shared_ptr<Runtime::Array>>(objVal)) {
                        auto m = std::make_shared<Runtime::Method>();
                        m->type = Runtime::Method::ARRAY;
                        m->name = name;
                        m->target = objVal;
                        push(m);
                    } else if (std::holds_alternative<std::shared_ptr<Runtime::Map>>(objVal)) {
                        auto m = std::make_shared<Runtime::Method>();
                        m->type = Runtime::Method::MAP;
                        m->name = name;
                        m->target = objVal;
                        push(m);
                    } else if (std::holds_alternative<std::string>(objVal)) {
                        auto m = std::make_shared<Runtime::Method>();
                        m->type = Runtime::Method::STRING;
                        m->name = name;
                        m->target = objVal;
                        push(m);
                    }
                    else throw std::runtime_error("Property access on non-object type.");
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
                case OP_TRY: {
                    uint16_t offset = (uint16_t)((readByte() << 8) | readByte());
                    handlerStack.push_back({ frames[frameCount - 1].ip + offset, (int)(stackTop - stack), frameCount - 1 });
                    break;
                }
                case OP_THROW: {
                    Runtime::Value exception = pop();
                    if (handlerStack.empty()) {
                        throw std::runtime_error("Unhandled exception: " + Runtime::valueToString(exception));
                    }
                    ExceptionHandler handler = handlerStack.back();
                    handlerStack.pop_back();

                    // Unwind frames
                    frameCount = handler.frameIndex + 1;
                    // Restore stack
                    stackTop = stack + handler.stackDepth;
                    // Push exception value for catch block
                    push(exception);
                    // Jump to handler
                    frames[frameCount - 1].ip = handler.handlerIP;
                    break;
                }
                case OP_END_TRY: {
                    if (!handlerStack.empty()) handlerStack.pop_back();
                    break;
                }
                case OP_RETURN: {
                    auto result = pop(); 
                    while (!handlerStack.empty() && handlerStack.back().frameIndex == frameCount - 1) {
                        handlerStack.pop_back();
                    }
                    frameCount--;
                    if (frameCount < initialFrameCount) {
                         push(result); return InterpretResult::INTERPRET_OK;
                    }
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

Runtime::Value VM::callValue(Runtime::Value callee, const std::vector<Runtime::Value>& args) {
    if (std::holds_alternative<std::shared_ptr<Runtime::ObjFunction>>(callee)) {
        auto func = std::get<std::shared_ptr<Runtime::ObjFunction>>(callee);
        Runtime::Value* savedStackTop = stackTop;
        push(callee);
        for (auto const& arg : args) push(arg);
        CallFrame* frame = &frames[frameCount++];
        frame->chunk = func->chunk.get();
        frame->ip = func->chunk->code.data();
        frame->slots = stackTop - args.size() - 1;
        run();
        Runtime::Value result = pop();
        stackTop = savedStackTop;
        return result;
    }
    return std::shared_ptr<Runtime::Object>(nullptr);
}

void VM::callFluxFunction(const std::string& name) {
    Runtime::Value val;
    bool found = false;
    {
        std::lock_guard<std::mutex> lock(*globalsMutex);
        if (globals->count(name)) {
            val = (*globals)[name];
            found = true;
        }
    }
    if (found && std::holds_alternative<std::shared_ptr<Runtime::ObjFunction>>(val)) {
         auto func = std::get<std::shared_ptr<Runtime::ObjFunction>>(val);
         if (frameCount == FRAMES_MAX) return;
         CallFrame* frame = &frames[frameCount++];
         frame->chunk = func->chunk.get();
         frame->ip = func->chunk->code.data();
         frame->slots = stackTop;
         run();
    }
}

} // namespace Flux
