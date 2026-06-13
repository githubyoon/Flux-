#include "ModuleMath.h"
#include "VM.h"
#include <cmath>

namespace Flux::Modules {

void handleMath(const std::string& subName, VM& vm) {
    double v = std::holds_alternative<int>(vm.peek(0)) ? (double)std::get<int>(vm.peek(0)) : (double)std::get<float>(vm.peek(0));
    if (subName == "abs") { vm.pop(); vm.pop(); vm.push((float)std::abs(v)); }
    else if (subName == "round") { vm.pop(); vm.pop(); vm.push((int)std::round(v)); }
    else if (subName == "sin") { vm.pop(); vm.pop(); vm.push((float)std::sin(v)); }
    else if (subName == "cos") { vm.pop(); vm.pop(); vm.push((float)std::cos(v)); }
    else if (subName == "tan") { vm.pop(); vm.pop(); vm.push((float)std::tan(v)); }
    else if (subName == "sqrt") { vm.pop(); vm.pop(); vm.push((float)std::sqrt(v)); }
    else if (subName == "pow") { 
        double base = std::holds_alternative<int>(vm.peek(1)) ? (double)std::get<int>(vm.peek(1)) : (double)std::get<float>(vm.peek(1));
        vm.pop(); vm.pop(); vm.pop(); vm.push((float)std::pow(base, v)); 
    }
    else if (subName == "log") { vm.pop(); vm.pop(); vm.push((float)std::log(v)); }
    else if (subName == "pi") { vm.pop(); vm.push(3.14159265f); }
    else if (subName == "e") { vm.pop(); vm.push(2.71828182f); }
    else { vm.pop(); vm.pop(); vm.push(0); }
    // No break — caller handles it
}

} // namespace Flux::Modules
