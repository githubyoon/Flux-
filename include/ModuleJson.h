#ifndef FLUX_MODULE_JSON_H
#define FLUX_MODULE_JSON_H
#include <string>
namespace Flux { class VM; }
namespace Flux::Modules {
    void handleJson(const std::string& subName, VM& vm);
}
#endif
