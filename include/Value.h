#ifndef FLUX_VALUE_H
#define FLUX_VALUE_H

#include <string>
#include <variant>
#include <vector>
#include <memory>
#include <map>

namespace Flux {
namespace Runtime {

struct Array;
struct Object;
struct Map;
struct ObjFunction;
class Chunk;

using Value = std::variant<int, float, std::string, bool, std::shared_ptr<Array>, std::shared_ptr<Object>, std::shared_ptr<Map>, std::shared_ptr<ObjFunction>, void*>;

struct ObjFunction {
    std::string name;
    int arity;
    std::shared_ptr<Chunk> chunk;
    ObjFunction() : arity(0), chunk(std::make_shared<Chunk>()) {}
};

struct Array {
    std::string elementType;
    std::vector<Value> elements;
};

struct Object {
    std::string typeName;
    std::map<std::string, Value> members;
};

struct Map {
    std::map<std::string, Value> elements;
};

inline std::string valueToString(const Value& val) {
    if (std::holds_alternative<int>(val)) return std::to_string(std::get<int>(val));
    if (std::holds_alternative<float>(val)) {
        std::string s = std::to_string(std::get<float>(val));
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (s.back() == '.') s.pop_back();
        return s;
    }
    if (std::holds_alternative<std::string>(val)) return std::get<std::string>(val);
    if (std::holds_alternative<bool>(val)) return std::get<bool>(val) ? "true" : "false";
    if (std::holds_alternative<std::shared_ptr<Array>>(val)) {
        auto arr = std::get<std::shared_ptr<Array>>(val);
        std::string res = "[";
        for (size_t i = 0; i < arr->elements.size(); ++i) {
            res += valueToString(arr->elements[i]);
            if (i + 1 < arr->elements.size()) res += ", ";
        }
        res += "]";
        return res;
    }
    if (std::holds_alternative<std::shared_ptr<Object>>(val)) {
        return "[object " + std::get<std::shared_ptr<Object>>(val)->typeName + "]";
    }
    if (std::holds_alternative<std::shared_ptr<Map>>(val)) {
        return "[map]";
    }
    return "";
}

} // namespace Runtime
} // namespace Flux

#endif
