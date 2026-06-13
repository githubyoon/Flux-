#include "ModuleTime.h"
#include "VM.h"
#include <thread>
#include <chrono>
#include <ctime>
#include <string>

namespace Flux::Modules {

void handleTime(const std::string& subName, VM& vm) {
    if (subName == "sleep") {
        int ms = std::get<int>(vm.peek(0));
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    } else if (subName == "now") {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        char buf[26]; ctime_s(buf, sizeof(buf), &now);
        std::string s(buf); if (!s.empty()) s.pop_back();
        vm.pop(); vm.push(s); return;
    } else if (subName == "ticks") {
        auto now = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        vm.pop(); vm.push((int)ms); return;
    } else if (subName == "year") {
        time_t now = time(0); tm ltm; localtime_s(&ltm, &now);
        vm.pop(); vm.push(1900 + ltm.tm_year); return;
    } else if (subName == "month") {
        time_t now = time(0); tm ltm; localtime_s(&ltm, &now);
        vm.pop(); vm.push(1 + ltm.tm_mon); return;
    } else if (subName == "date") {
        time_t now = time(0); tm ltm; localtime_s(&ltm, &now);
        char buf[11]; std::strftime(buf, sizeof(buf), "%Y-%m-%d", &ltm);
        vm.pop(); vm.push(std::string(buf)); return;
    } else if (subName == "hour") {
        time_t now = time(0); tm ltm; localtime_s(&ltm, &now);
        vm.pop(); vm.push(ltm.tm_hour); return;
    } else if (subName == "minute") {
        time_t now = time(0); tm ltm; localtime_s(&ltm, &now);
        vm.pop(); vm.push(ltm.tm_min); return;
    } else if (subName == "second") {
        time_t now = time(0); tm ltm; localtime_s(&ltm, &now);
        vm.pop(); vm.push(ltm.tm_sec); return;
    } else if (subName == "format") {
        std::string fmt = Runtime::valueToString(vm.peek(0));
        time_t now = time(0); tm ltm; localtime_s(&ltm, &now);
        char buf[256]; std::strftime(buf, sizeof(buf), fmt.c_str(), &ltm);
        vm.pop(); vm.pop(); vm.push(std::string(buf)); return;
    }
    vm.pop(); vm.pop(); vm.push(0);
    // No break — caller handles it
}

} // namespace Flux::Modules
