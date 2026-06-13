#include "ModuleSystem.h"
#include "VM.h"
#include <cstdlib>
#include <filesystem>
#include <thread>
#include <string>
#ifdef _WIN32
#include <process.h>
#endif

namespace Flux::Modules {

void handleSystem(const std::string& subName, VM& vm) {
    if (subName == "exit") std::exit(std::get<int>(vm.pop()));
    else if (subName == "run") {
        std::string cmd = Runtime::valueToString(vm.pop());
        vm.pop(); vm.push(system(cmd.c_str()));
    } else if (subName == "os") {
#ifdef _WIN32
        vm.pop(); vm.push(std::string("windows"));
#else
        vm.pop(); vm.push(std::string("linux"));
#endif
    } else if (subName == "env") {
        std::string key = Runtime::valueToString(vm.pop());
        char* val = nullptr;
        size_t len = 0;
        _dupenv_s(&val, &len, key.c_str());
        std::string res = val ? val : "";
        free(val);
        vm.pop(); vm.push(res);
    } else if (subName == "cpu") {
        vm.pop(); vm.push((int)std::thread::hardware_concurrency()); return;
    } else if (subName == "pid") {
#ifdef _WIN32
        vm.pop(); vm.push(_getpid());
#else
        vm.pop(); vm.push(static_cast<int>(getpid()));
#endif
        return;
    } else if (subName == "user") {
#ifdef _WIN32
        char* val = nullptr; size_t len = 0;
        _dupenv_s(&val, &len, "USERNAME");
        std::string res = val ? val : ""; free(val);
#else
        char* val = getenv("USER"); std::string res = val ? val : "";
#endif
        vm.pop(); vm.push(res); return;
    } else if (subName == "cwd") {
        vm.pop(); vm.push(std::filesystem::current_path().string()); return;
    } else if (subName == "temp") {
        vm.pop(); vm.push(std::filesystem::temp_directory_path().string()); return;
    } else if (subName == "host") {
#ifdef _WIN32
        char* val = nullptr; size_t len = 0;
        _dupenv_s(&val, &len, "COMPUTERNAME");
        std::string res = val ? val : ""; free(val);
#else
        char* val = getenv("HOSTNAME"); std::string res = val ? val : "";
#endif
        vm.pop(); vm.push(res); return;
    } else { vm.pop(); vm.pop(); vm.push(0); }
    // No break — caller handles it
}

} // namespace Flux::Modules
