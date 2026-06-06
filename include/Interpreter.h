#ifndef FLUX_INTERPRETER_H
#define FLUX_INTERPRETER_H

#include "AST.h"
#include "Environment.h"
#include <map>

#include <set>

namespace Flux {

class Interpreter {
public:
    Interpreter();
    void interpret(AST::Program& program);
    void callFluxFunction(const std::string& name);

private:
    std::shared_ptr<Environment> globals;
    std::shared_ptr<Environment> environment;
    std::map<std::string, AST::FunctionDef*> functions;
    std::map<std::string, AST::StructDef*> structs;
    std::map<std::string, AST::ClassDef*> classes;
    std::set<std::string> importedModules;
    std::vector<std::shared_ptr<AST::Program>> loadedPrograms;

    Runtime::Value evaluate(AST::Expression& expr);
    void execute(AST::Statement& stmt);

    void print(const std::vector<Runtime::Value>& args);
};

} // namespace Flux

#endif
