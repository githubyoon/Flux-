#ifndef FLUX_MODULE_TIME_H
#define FLUX_MODULE_TIME_H
#include <string>
namespace Flux { class VM; }
namespace Flux::Modules {
    void handleTime(const std::string& subName, VM& vm);
}
#endif
