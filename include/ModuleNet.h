#ifndef FLUX_MODULE_NET_H
#define FLUX_MODULE_NET_H
#include <string>
namespace Flux { class VM; }
namespace Flux::Modules {
    void handleNet(const std::string& subName, VM& vm);
}
#endif
