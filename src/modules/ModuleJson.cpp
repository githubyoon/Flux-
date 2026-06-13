#include "ModuleJson.h"
#include "VM.h"
#include "AST.h"
#include <string>
#include <cctype>

namespace Flux::Modules {
    namespace {
        static Runtime::Value parseJsonValue(const std::string& source, size_t& pos) {
            while (pos < source.size() && isspace(source[pos])) pos++;
            if (pos >= source.size()) return std::shared_ptr<Runtime::Object>(nullptr);
            
            char c = source[pos];
            if (c == '{') {
                pos++;
                auto map = std::make_shared<Runtime::Map>();
                while (pos < source.size()) {
                    while (pos < source.size() && isspace(source[pos])) pos++;
                    if (source[pos] == '}') { pos++; break; }
                    Runtime::Value keyVal = parseJsonValue(source, pos);
                    std::string key = Runtime::valueToString(keyVal);
                    while (pos < source.size() && isspace(source[pos])) pos++;
                    if (source[pos] == ':') pos++;
                    map->elements[key] = parseJsonValue(source, pos);
                    while (pos < source.size() && isspace(source[pos])) pos++;
                    if (source[pos] == ',') pos++;
                    else if (source[pos] == '}') { pos++; break; }
                }
                return map;
            }
            if (c == '[') {
                pos++;
                auto arr = std::make_shared<Runtime::Array>();
                while (pos < source.size()) {
                    while (pos < source.size() && isspace(source[pos])) pos++;
                    if (source[pos] == ']') { pos++; break; }
                    arr->elements.push_back(parseJsonValue(source, pos));
                    while (pos < source.size() && isspace(source[pos])) pos++;
                    if (source[pos] == ',') pos++;
                    else if (source[pos] == ']') { pos++; break; }
                }
                return arr;
            }
            if (c == '"') {
                pos++; std::string s;
                while (pos < source.size() && source[pos] != '"') {
                    if (source[pos] == '\\') {
                        pos++;
                        if (pos >= source.size()) break;
                        switch (source[pos]) {
                            case '"':  s += '"'; break;
                            case '\\': s += '\\'; break;
                            case '/':  s += '/'; break;
                            case 'n':  s += '\n'; break;
                            case 't':  s += '\t'; break;
                            case 'r':  s += '\r'; break;
                            case 'b':  s += '\b'; break;
                            case 'f':  s += '\f'; break;
                            default:   s += source[pos]; break;
                        }
                    } else {
                        s += source[pos];
                    }
                    pos++;
                }
                if (pos < source.size()) pos++;
                return s;
            }
            size_t start = pos;
            while (pos < source.size() && !isspace(source[pos]) && source[pos] != ',' && source[pos] != '}' && source[pos] != ']') pos++;
            std::string token = source.substr(start, pos - start);
            if (token == "true") return true;
            if (token == "false") return false;
            if (token == "null") return std::shared_ptr<Runtime::Object>(nullptr);
            try { if (token.find('.') != std::string::npos) return std::stof(token); return std::stoi(token); }
            catch (...) { return token; }
        }

        static std::string stringifyJson(const Runtime::Value& val) {
            if (std::holds_alternative<int>(val)) return std::to_string(std::get<int>(val));
            if (std::holds_alternative<float>(val)) return std::to_string(std::get<float>(val));
            if (std::holds_alternative<std::string>(val)) {
                auto s = std::get<std::string>(val);
                std::string escaped = "\"";
                for (char ch : s) {
                    switch (ch) {
                        case '"':  escaped += "\\\""; break;
                        case '\\': escaped += "\\\\"; break;
                        case '\n': escaped += "\\n";  break;
                        case '\t': escaped += "\\t";  break;
                        case '\r': escaped += "\\r";  break;
                        case '\b': escaped += "\\b";  break;
                        case '\f': escaped += "\\f";  break;
                        default:   escaped += ch;      break;
                    }
                }
                escaped += "\"";
                return escaped;
            }
            if (std::holds_alternative<bool>(val)) return std::get<bool>(val) ? "true" : "false";
            if (std::holds_alternative<std::shared_ptr<Runtime::Array>>(val)) {
                auto arr = std::get<std::shared_ptr<Runtime::Array>>(val);
                std::string res = "[";
                for (size_t i = 0; i < arr->elements.size(); ++i) {
                    res += stringifyJson(arr->elements[i]);
                    if (i + 1 < arr->elements.size()) res += ",";
                }
                return res + "]";
            }
            if (std::holds_alternative<std::shared_ptr<Runtime::Map>>(val)) {
                auto map = std::get<std::shared_ptr<Runtime::Map>>(val);
                std::string res = "{"; bool first = true;
                for (auto const& [k, v] : map->elements) {
                    if (!first) res += ",";
                    res += "\"" + k + "\":" + stringifyJson(v);
                    first = false;
                }
                return res + "}";
            }
            return "null";
        }

        static std::string stringifyJsonPretty(const Runtime::Value& v, int indent = 0) {
            std::string ind(indent, ' ');
            if (std::holds_alternative<int>(v)) return std::to_string(std::get<int>(v));
            if (std::holds_alternative<float>(v)) return std::to_string(std::get<float>(v));
            if (std::holds_alternative<std::string>(v)) {
                auto s = std::get<std::string>(v);
                std::string escaped = "\"";
                for (char ch : s) {
                    switch (ch) {
                        case '"':  escaped += "\\\""; break;
                        case '\\': escaped += "\\\\"; break;
                        case '\n': escaped += "\\n";  break;
                        case '\t': escaped += "\\t";  break;
                        case '\r': escaped += "\\r";  break;
                        case '\b': escaped += "\\b";  break;
                        case '\f': escaped += "\\f";  break;
                        default:   escaped += ch;      break;
                    }
                }
                escaped += "\"";
                return escaped;
            }
            if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? "true" : "false";
            if (std::holds_alternative<std::shared_ptr<Runtime::Array>>(v)) {
                auto arr = std::get<std::shared_ptr<Runtime::Array>>(v);
                if (arr->elements.empty()) return "[]";
                std::string r = "[\n";
                bool first = true;
                for (auto& el : arr->elements) {
                    if (!first) r += ",\n";
                    first = false;
                    r += ind + "  " + stringifyJsonPretty(el, indent + 2);
                }
                return r + "\n" + ind + "]";
            }
            if (std::holds_alternative<std::shared_ptr<Runtime::Map>>(v)) {
                auto map = std::get<std::shared_ptr<Runtime::Map>>(v);
                if (map->elements.empty()) return "{}";
                std::string r = "{\n";
                bool first = true;
                for (auto& [k, v] : map->elements) {
                    if (!first) r += ",\n";
                    first = false;
                    r += ind + "  \"" + k + "\": " + stringifyJsonPretty(v, indent + 2);
                }
                return r + "\n" + ind + "}";
            }
            return "null";
        }
    }

    void handleJson(const std::string& subName, VM& vm) {
        if (subName == "parse") {
            std::string s = Runtime::valueToString(vm.pop());
            size_t p = 0;
            vm.pop(); // pop json
            vm.push(parseJsonValue(s, p));
        } else if (subName == "stringify") {
            auto v = vm.pop();
            vm.pop(); // pop json
            vm.push(stringifyJson(v));
        } else if (subName == "isValid") {
            std::string s = Runtime::valueToString(vm.pop());
            size_t p = 0;
            parseJsonValue(s, p);
            while (p < s.size() && isspace(s[p])) p++;
            vm.pop(); // pop json
            vm.push(p >= s.size());
        } else if (subName == "format") {
            std::string s = Runtime::valueToString(vm.pop());
            size_t p = 0;
            auto parsed = parseJsonValue(s, p);
            vm.pop(); // pop json
            vm.push(stringifyJsonPretty(parsed));
        } else if (subName == "minify") {
            std::string s = Runtime::valueToString(vm.pop());
            size_t p = 0;
            auto parsed = parseJsonValue(s, p);
            vm.pop(); // pop json
            vm.push(stringifyJson(parsed));
        } else if (subName == "keys") {
            auto v = vm.pop();
            vm.pop(); // pop json
            auto arr = std::make_shared<Runtime::Array>();
            if (std::holds_alternative<std::shared_ptr<Runtime::Map>>(v)) {
                auto map = std::get<std::shared_ptr<Runtime::Map>>(v);
                for (auto& [k, _] : map->elements)
                    arr->elements.push_back(k);
            }
            vm.push(arr);
        } else if (subName == "values") {
            auto v = vm.pop();
            vm.pop(); // pop json
            auto arr = std::make_shared<Runtime::Array>();
            if (std::holds_alternative<std::shared_ptr<Runtime::Map>>(v)) {
                auto map = std::get<std::shared_ptr<Runtime::Map>>(v);
                for (auto& [_, val] : map->elements)
                    arr->elements.push_back(val);
            }
            vm.push(arr);
        } else if (subName == "has") {
            std::string key = Runtime::valueToString(vm.pop());
            auto v = vm.pop();
            vm.pop(); // pop json
            bool found = false;
            if (std::holds_alternative<std::shared_ptr<Runtime::Map>>(v)) {
                auto map = std::get<std::shared_ptr<Runtime::Map>>(v);
                found = map->elements.count(key) > 0;
            }
            vm.push(found);
        } else if (subName == "merge") {
            auto b = vm.pop();
            auto a = vm.pop();
            vm.pop(); // pop json
            if (std::holds_alternative<std::shared_ptr<Runtime::Map>>(a) &&
                std::holds_alternative<std::shared_ptr<Runtime::Map>>(b)) {
                auto mapA = std::get<std::shared_ptr<Runtime::Map>>(a);
                auto mapB = std::get<std::shared_ptr<Runtime::Map>>(b);
                auto result = std::make_shared<Runtime::Map>();
                // deep copy from a
                for (auto& [k, v] : mapA->elements) {
                    if (std::holds_alternative<std::shared_ptr<Runtime::Map>>(v) &&
                        mapB->elements.count(k) &&
                        std::holds_alternative<std::shared_ptr<Runtime::Map>>(mapB->elements[k])) {
                        // recursive merge
                        auto merged = std::make_shared<Runtime::Map>();
                        auto innerA = std::get<std::shared_ptr<Runtime::Map>>(v);
                        auto innerB = std::get<std::shared_ptr<Runtime::Map>>(mapB->elements[k]);
                        for (auto& [ik, iv] : innerA->elements) merged->elements[ik] = iv;
                        for (auto& [ik, iv] : innerB->elements) merged->elements[ik] = iv;
                        result->elements[k] = merged;
                    } else {
                        result->elements[k] = v;
                    }
                }
                // copy from b (overwrites)
                for (auto& [k, v] : mapB->elements) {
                    if (!mapA->elements.count(k) ||
                        !std::holds_alternative<std::shared_ptr<Runtime::Map>>(mapA->elements[k]) ||
                        !std::holds_alternative<std::shared_ptr<Runtime::Map>>(v)) {
                        result->elements[k] = v;
                    }
                }
                vm.push(result);
            } else if (std::holds_alternative<std::shared_ptr<Runtime::Map>>(a)) {
                vm.push(a);
            } else if (std::holds_alternative<std::shared_ptr<Runtime::Map>>(b)) {
                vm.push(b);
            } else {
                vm.push(std::make_shared<Runtime::Map>());
            }
        } else if (subName == "type") {
            auto v = vm.pop();
            vm.pop(); // pop json
            std::string typeStr;
            if (std::holds_alternative<std::shared_ptr<Runtime::Map>>(v)) typeStr = "object";
            else if (std::holds_alternative<std::shared_ptr<Runtime::Array>>(v)) typeStr = "array";
            else if (std::holds_alternative<std::string>(v)) typeStr = "string";
            else if (std::holds_alternative<int>(v) || std::holds_alternative<float>(v)) typeStr = "number";
            else if (std::holds_alternative<bool>(v)) typeStr = "boolean";
            else typeStr = "null";
            vm.push(typeStr);
        }
        // Note: original final break; removed — function returns naturally
    }
}
