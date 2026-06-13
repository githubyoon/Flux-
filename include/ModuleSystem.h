#ifndef FLUX_MODULE_SYSTEM_H
#define FLUX_MODULE_SYSTEM_H
#include <string>
namespace Flux { class VM; }
namespace Flux::Modules {
    void handleSystem(const std::string& subName, VM& vm);
}
#endif
