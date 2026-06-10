#ifndef FLUX_COMPILER_H
#define FLUX_COMPILER_H

#include "AST.h"
#include "Chunk.h"
#include <set>
#include <string>

namespace Flux {

struct Local {
    std::string name;
    int depth;
};

class Compiler {
public:
    Compiler();
    void compile(AST::Program& program, Runtime::Chunk* chunk);

private:
    Runtime::Chunk* currentChunk;
    std::vector<Local> locals;
    int scopeDepth;
    std::shared_ptr<Runtime::ObjFunction> currentFunction;
    std::set<std::string> loadedFiles;

    void emitByte(uint8_t byte);
    void emitConstant(Runtime::Value value);
    int emitJump(uint8_t instruction);
    void patchJump(int offset);
    void emitLoop(int loopStart);
    
    void beginScope() { scopeDepth++; }
    void endScope();
    int resolveLocal(const std::string& name);

    void compileStatement(AST::Statement& stmt);
    void compileExpression(AST::Expression& expr);
};

} // namespace Flux

#endif
