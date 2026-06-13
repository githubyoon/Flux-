#ifndef FLUX_MODULE_MATH_H
#define FLUX_MODULE_MATH_H
#include <string>
namespace Flux { class VM; }
namespace Flux::Modules {
    void handleMath(const std::string& subName, VM& vm);
}
#endif
