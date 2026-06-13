#include "ModuleFile.h"
#include "VM.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>

namespace Flux::Modules {

void handleFile(const std::string& subName, VM& vm) {
    std::string path = Runtime::valueToString(vm.peek(0));
    if (subName == "read") {
        std::ifstream f(path);
        if (f) {
            std::stringstream b; b << f.rdbuf();
            vm.pop(); vm.pop(); vm.push(b.str());
        } else { vm.pop(); vm.pop(); vm.push(std::string("")); }
    } else if (subName == "write") {
        std::string content = Runtime::valueToString(vm.peek(1));
        std::ofstream f(path);
        if (f) f << content;
        vm.pop(); vm.pop(); vm.pop(); vm.push(0);
    } else if (subName == "append") {
        std::string content = Runtime::valueToString(vm.peek(1));
        std::ofstream f(path, std::ios::app);
        if (f) f << content;
        vm.pop(); vm.pop(); vm.pop(); vm.push(0);
    } else if (subName == "exists") {
        vm.pop(); vm.pop(); vm.push(std::filesystem::exists(path));
    } else if (subName == "remove") {
        std::filesystem::remove(path);
        vm.pop(); vm.pop(); vm.push(0);
    } else if (subName == "size") {
        vm.pop(); vm.pop(); vm.push((int)std::filesystem::file_size(path));
    } else if (subName == "is_dir") {
        vm.pop(); vm.pop(); vm.push(std::filesystem::is_directory(path));
    } else if (subName == "mkdir") {
        std::filesystem::create_directories(path);
        vm.pop(); vm.pop(); vm.push(0);
    } else if (subName == "copy") {
        std::string dst = Runtime::valueToString(vm.peek(0));
        std::string src = Runtime::valueToString(vm.peek(1));
        try { std::filesystem::copy(src, dst, std::filesystem::copy_options::overwrite_existing); } catch (...) {}
        vm.pop(); vm.pop(); vm.pop(); vm.push(0);
    } else if (subName == "rename") {
        std::string newName = Runtime::valueToString(vm.peek(0));
        std::string oldName = Runtime::valueToString(vm.peek(1));
        try { std::filesystem::rename(oldName, newName); } catch (...) {}
        vm.pop(); vm.pop(); vm.pop(); vm.push(0);
    } else { vm.pop(); vm.pop(); vm.push(0); }
    // No break — caller handles it
}

} // namespace Flux::Modules
