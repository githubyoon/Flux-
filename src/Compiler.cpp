#include "Compiler.h"
#include "OpCode.h"
#include "Lexer.h"
#include "Parser.h"
#include <stdexcept>
#include <fstream>
#include <sstream>

namespace Flux {

Compiler::Compiler() : currentChunk(nullptr), scopeDepth(0), currentFunction(nullptr) {}

void Compiler::compile(AST::Program& program, Runtime::Chunk* chunk) {
    this->currentChunk = chunk;
    locals.push_back({"", 0}); 

    for (auto& stmt : program.statements) {
        compileStatement(*stmt);
    }

    // Automatically call main() at the end of script
    int index = currentChunk->addConstant(std::string("main"));
    emitByte(OP_GET_GLOBAL);
    emitByte((uint8_t)index);
    emitByte(OP_CALL);
    emitByte(0); // 0 arguments
    emitByte(OP_POP);

    emitByte(OP_RETURN);
}


void Compiler::emitByte(uint8_t byte) {
    currentChunk->write(byte, 0); 
}

void Compiler::emitConstant(Runtime::Value value) {
    int index = currentChunk->addConstant(value);
    emitByte(OP_CONSTANT);
    emitByte((uint8_t)index);
}

int Compiler::emitJump(uint8_t instruction) {
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return (int)currentChunk->code.size() - 2;
}

void Compiler::patchJump(int offset) {
    int jump = (int)currentChunk->code.size() - offset - 2;
    currentChunk->code[offset] = (jump >> 8) & 0xff;
    currentChunk->code[offset + 1] = jump & 0xff;
}

void Compiler::emitLoop(int loopStart) {
    emitByte(OP_LOOP);
    int offset = (int)currentChunk->code.size() - loopStart + 2;
    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

void Compiler::endScope() {
    scopeDepth--;
    while (!locals.empty() && locals.back().depth > scopeDepth) {
        emitByte(OP_POP);
        locals.pop_back();
    }
}

int Compiler::resolveLocal(const std::string& name) {
    for (int i = (int)locals.size() - 1; i >= 0; i--) {
        if (locals[i].name == name) return i;
    }
    return -1;
}

void Compiler::compileStatement(AST::Statement& stmt) {
    if (auto* es = dynamic_cast<AST::ExpressionStmt*>(&stmt)) {
        compileExpression(*es->expr);
        emitByte(OP_POP);
    } else if (auto* varDecl = dynamic_cast<AST::VarDeclaration*>(&stmt)) {
        for (auto& decl : varDecl->decls) {
            if (decl->initializer) {
                compileExpression(*decl->initializer);
            } else if (varDecl->type == "int") emitConstant(0);
            else if (varDecl->type == "float") emitConstant(0.0f);
            else if (varDecl->type == "bool") emitByte(OP_FALSE);
            else if (varDecl->type == "string") emitConstant(std::string(""));
            else {
                int nameIdx = currentChunk->addConstant(varDecl->type);
                emitByte(OP_NEW);
                emitByte((uint8_t)nameIdx);
            }

            if (scopeDepth > 0) {
                // Local variable
                locals.push_back({decl->name, scopeDepth});
            } else {
                // Global variable
                int index = currentChunk->addConstant(decl->name);
                emitByte(OP_DEFINE_GLOBAL);
                emitByte((uint8_t)index);
            }
        }
    } else if (auto* func = dynamic_cast<AST::FunctionDef*>(&stmt)) {
        auto fnObj = std::make_shared<Runtime::ObjFunction>();
        fnObj->name = func->name;
        fnObj->arity = (int)func->params.size();

        Compiler subCompiler;
        subCompiler.currentFunction = fnObj;
        subCompiler.currentChunk = fnObj->chunk.get();
        subCompiler.scopeDepth = 1;

        // Slot 0 is reserved for function itself or 'this'
        subCompiler.locals.push_back({func->name, 0}); 
        for (auto& p : func->params) {
            subCompiler.locals.push_back({p.name, 1});
        }

        for (auto& s : func->body) subCompiler.compileStatement(*s);
        subCompiler.emitByte(OP_NULL);
        subCompiler.emitByte(OP_RETURN);

        // Define function in global scope
        int index = currentChunk->addConstant(fnObj);
        emitByte(OP_CONSTANT);
        emitByte((uint8_t)index);
        
        int nameIdx = currentChunk->addConstant(func->name);
        emitByte(OP_DEFINE_GLOBAL);
        emitByte((uint8_t)nameIdx);

    } else if (auto* ifStmt = dynamic_cast<AST::IfStmt*>(&stmt)) {
        compileExpression(*ifStmt->condition);
        int thenJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
        compileStatement(*ifStmt->thenBranch);
        int elseJump = emitJump(OP_JUMP);
        patchJump(thenJump);
        emitByte(OP_POP);
        if (ifStmt->elseBranch) compileStatement(*ifStmt->elseBranch);
        patchJump(elseJump);
    } else if (auto* whileStmt = dynamic_cast<AST::WhileStmt*>(&stmt)) {
        int loopStart = (int)currentChunk->code.size();
        compileExpression(*whileStmt->condition);
        int exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
        compileStatement(*whileStmt->body);
        emitLoop(loopStart);
        patchJump(exitJump);
        emitByte(OP_POP);
    } else if (auto* returnStmt = dynamic_cast<AST::ReturnStmt*>(&stmt)) {
        if (returnStmt->value) compileExpression(*returnStmt->value);
        else emitByte(OP_NULL);
        emitByte(OP_RETURN);
    } else if (auto* block = dynamic_cast<AST::BlockStmt*>(&stmt)) {
        beginScope();
        for (auto& s : block->statements) compileStatement(*s);
        endScope();
    } else if (auto* importStmt = dynamic_cast<AST::ImportStmt*>(&stmt)) {
        std::string moduleName = importStmt->moduleName;
        if (moduleName == "math" || moduleName == "gui" || moduleName == "system") return;
        
        std::string fileName = moduleName + ".fx";
        if (loadedFiles.count(fileName)) return;
        loadedFiles.insert(fileName);

        std::ifstream file(fileName);
        if (!file.is_open()) throw std::runtime_error("Could not open module file: " + fileName);
        std::stringstream buffer; buffer << file.rdbuf();
        std::string code = buffer.str();

        Lexer lexer(code);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto program = parser.parse();

        for (auto& s : program->statements) compileStatement(*s);

    } else if (auto* importFrom = dynamic_cast<AST::ImportFromStmt*>(&stmt)) {
        std::string fileName = importFrom->fileName;
        if (loadedFiles.count(fileName)) return;
        loadedFiles.insert(fileName);

        std::ifstream file(fileName);
        if (!file.is_open()) throw std::runtime_error("Could not open module file: " + fileName);
        std::stringstream buffer; buffer << file.rdbuf();
        std::string code = buffer.str();

        Lexer lexer(code);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto program = parser.parse();

        for (auto& s : program->statements) compileStatement(*s);

    } else if (auto* sd = dynamic_cast<AST::StructDef*>(&stmt)) {
        int nameIdx = currentChunk->addConstant(sd->name);
        int ptrIdx = currentChunk->addConstant((void*)sd);
        emitByte(OP_DEFINE_STRUCT);
        emitByte((uint8_t)nameIdx);
        emitByte((uint8_t)ptrIdx);
    } else if (auto* cd = dynamic_cast<AST::ClassDef*>(&stmt)) {
        int nameIdx = currentChunk->addConstant(cd->name);
        int ptrIdx = currentChunk->addConstant((void*)cd);
        emitByte(OP_DEFINE_CLASS);
        emitByte((uint8_t)nameIdx);
        emitByte((uint8_t)ptrIdx);

        // Compile methods
        for (auto& m : cd->methods) {
            auto fnObj = std::make_shared<Runtime::ObjFunction>();
            fnObj->name = cd->name + "." + m->name;
            fnObj->arity = (int)m->params.size();

            Compiler sub;
            sub.currentChunk = fnObj->chunk.get();
            sub.scopeDepth = 1;
            sub.locals.push_back({"this", 0}); // Slot 0 is this
            for (auto& p : m->params) sub.locals.push_back({p.name, 1});
            for (auto& s : m->body) sub.compileStatement(*s);
            sub.emitByte(OP_NULL);
            sub.emitByte(OP_RETURN);
            
            // Register method in global (simplified for now)
            int fnIdx = currentChunk->addConstant(fnObj);
            emitByte(OP_CONSTANT); emitByte((uint8_t)fnIdx);
            int mNameIdx = currentChunk->addConstant(cd->name + "." + m->name);
            emitByte(OP_DEFINE_GLOBAL); emitByte((uint8_t)mNameIdx);
        }
    }
}

void Compiler::compileExpression(AST::Expression& expr) {
    if (auto* il = dynamic_cast<AST::IntLiteral*>(&expr)) {
        emitConstant(il->value);
    } else if (auto* fl = dynamic_cast<AST::FloatLiteral*>(&expr)) {
        emitConstant(fl->value);
    } else if (auto* sl = dynamic_cast<AST::StringLiteral*>(&expr)) {
        emitConstant(sl->value);
    } else if (auto* bl = dynamic_cast<AST::BooleanLiteral*>(&expr)) {
        emitByte(bl->value ? OP_TRUE : OP_FALSE);
    } else if (dynamic_cast<AST::ThisExpr*>(&expr)) {
        emitByte(OP_GET_LOCAL);
        emitByte(0);
    } else if (auto* newExpr = dynamic_cast<AST::NewExpr*>(&expr)) {
        int index = currentChunk->addConstant(newExpr->className);
        emitByte(OP_NEW);
        emitByte((uint8_t)index);
    } else if (auto* memAcc = dynamic_cast<AST::MemberAccessExpr*>(&expr)) {
        compileExpression(*memAcc->object);
        int index = currentChunk->addConstant(memAcc->memberName);
        emitByte(OP_GET_PROPERTY);
        emitByte((uint8_t)index);
    } else if (auto* memAss = dynamic_cast<AST::MemberAssignment*>(&expr)) {
        compileExpression(*memAss->memberAccess->object);
        compileExpression(*memAss->value);
        int index = currentChunk->addConstant(memAss->memberAccess->memberName);
        emitByte(OP_SET_PROPERTY);
        emitByte((uint8_t)index);
    } else if (auto* unary = dynamic_cast<AST::UnaryExpr*>(&expr)) {
        if (unary->op.type == TokenType::T_MINUS) {
            emitConstant(0);
            compileExpression(*unary->operand);
            emitByte(OP_SUBTRACT);
        } else if (unary->op.type == TokenType::T_BANG) {
            compileExpression(*unary->operand);
            emitByte(OP_NOT);
        }
    } else if (auto* var = dynamic_cast<AST::VariableExpr*>(&expr)) {
        int local = resolveLocal(var->name);
        if (local != -1) {
            emitByte(OP_GET_LOCAL);
            emitByte((uint8_t)local);
        } else {
            int index = currentChunk->addConstant(var->name);
            emitByte(OP_GET_GLOBAL);
            emitByte((uint8_t)index);
        }
    } else if (auto* assign = dynamic_cast<AST::Assignment*>(&expr)) {
        compileExpression(*assign->value);
        int local = resolveLocal(assign->name);
        if (local != -1) {
            emitByte(OP_SET_LOCAL);
            emitByte((uint8_t)local);
        } else {
            int index = currentChunk->addConstant(assign->name);
            emitByte(OP_SET_GLOBAL);
            emitByte((uint8_t)index);
        }
    } else if (auto* bin = dynamic_cast<AST::BinaryExpr*>(&expr)) {
        compileExpression(*bin->left);
        compileExpression(*bin->right);
        switch (bin->op.type) {
            case TokenType::T_PLUS: emitByte(OP_ADD); break;
            case TokenType::T_MINUS: emitByte(OP_SUBTRACT); break;
            case TokenType::T_STAR: emitByte(OP_MULTIPLY); break;
            case TokenType::T_SLASH: emitByte(OP_DIVIDE); break;
            case TokenType::T_EQUAL_EQUAL: emitByte(OP_EQUAL); break;
            case TokenType::T_GREATER: emitByte(OP_GREATER); break;
            case TokenType::T_LESS: emitByte(OP_LESS); break;
            default: break;
        }
    } else if (auto* call = dynamic_cast<AST::CallExpr*>(&expr)) {
        if (call->callee == "print") {
            compileExpression(*call->args[0]);
            emitByte(OP_PRINT);
            emitByte(OP_NULL); 
        } else {
            // Function call
            int index = currentChunk->addConstant(call->callee);
            emitByte(OP_GET_GLOBAL);
            emitByte((uint8_t)index);
            for (auto& arg : call->args) compileExpression(*arg);
            emitByte(OP_CALL);
            emitByte((uint8_t)call->args.size());
        }
    } else if (auto* printfExpr = dynamic_cast<AST::PrintfExpr*>(&expr)) {
        // Handle printf interpolation: "text {expr} text"
        bool first = true;
        for (const auto& p : printfExpr->parts) {
            if (p.isExpression) {
                compileExpression(*p.expr);
            } else {
                emitConstant(p.text);
            }
            if (!first) {
                emitByte(OP_ADD);
            }
            first = false;
        }
        if (first) emitConstant(std::string("")); // Empty printf
        emitByte(OP_PRINT);
        emitByte(OP_NULL);
    }
}

} // namespace Flux
