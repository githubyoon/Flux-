#ifndef FLUX_MODULE_FILE_H
#define FLUX_MODULE_FILE_H
#include <string>
namespace Flux { class VM; }
namespace Flux::Modules {
    void handleFile(const std::string& subName, VM& vm);
}
#endif
