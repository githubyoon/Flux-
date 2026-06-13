#ifndef FLUX_MODULE_CONSOLE_H
#define FLUX_MODULE_CONSOLE_H
#include <string>
namespace Flux { class VM; }
namespace Flux::Modules {
    void handleConsole(const std::string& subName, VM& vm);
}
#endif
