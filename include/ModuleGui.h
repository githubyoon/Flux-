#ifndef FLUX_MODULE_GUI_H
#define FLUX_MODULE_GUI_H
#include <string>
namespace Flux { class VM; }
#ifdef _WIN32
namespace Flux::Modules {
    void handleGui(const std::string& subName, VM& vm);
    void guiSetActiveVM(VM* vm);
}
#else
namespace Flux::Modules {
    inline void handleGui(const std::string&, VM&) {}
    inline void guiSetActiveVM(VM*) {}
}
#endif
#endif
