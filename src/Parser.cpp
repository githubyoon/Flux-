#include "Parser.h"
#include "Lexer.h"
#include <stdexcept>

namespace Flux {

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0) {}

const Token& Parser::peek() const { return tokens[pos]; }
const Token& Parser::previous() const { return tokens[pos - 1]; }
const Token& Parser::advance() { if (!isAtEnd()) pos++; return previous(); }

bool Parser::isAtEnd() const { return peek().type == TokenType::T_EOF; }
bool Parser::check(TokenType type) const { return !isAtEnd() && peek().type == type; }
bool Parser::match(TokenType type) { if (check(type)) { advance(); return true; } return false; }

const Token& Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    throw std::runtime_error("Error at [" + std::to_string(peek().line) + ":" + std::to_string(peek().column) + "]: " + message + " (Got: " + peek().value + ")");
}

std::unique_ptr<AST::Program> Parser::parse() {
    auto program = std::make_unique<AST::Program>();
    while (!isAtEnd()) program->statements.push_back(parseStatement());
    return program;
}

std::unique_ptr<AST::Statement> Parser::parseStatement() {
    if (match(TokenType::T_FUNCTION)) return parseFunctionDef();
    if (match(TokenType::T_IMPORT)) return parseImportStmt();
    if (match(TokenType::T_IF)) return parseIfStmt();
    if (match(TokenType::T_WHILE)) return parseWhileStmt();
    if (match(TokenType::T_FOR)) return parseForStmt();
    if (match(TokenType::T_LBRACE)) return std::make_unique<AST::BlockStmt>(parseBlock());
    
    // New Statements
    if (match(TokenType::T_STRUCT)) return parseStructDef();
    if (match(TokenType::T_CLASS)) return parseClassDef();
    if (match(TokenType::T_TRY)) return parseTryCatchStmt();
    if (match(TokenType::T_THROW)) return parseThrowStmt();

    // 변수 선언 체크 (IDENTIFIER IDENTIFIER 패턴만 선언으로 간주)
    if (check(TokenType::T_INT) || check(TokenType::T_FLOAT) || check(TokenType::T_STRING) || check(TokenType::T_BOOL) ||
        (check(TokenType::T_IDENTIFIER) && pos + 1 < tokens.size() && tokens[pos+1].type == TokenType::T_IDENTIFIER)) {
        
        advance();
        return parseVarDeclaration();
    }
    if (match(TokenType::T_RETURN)) return parseReturnStmt();
    if (match(TokenType::T_SWITCH)) return parseSwitchStmt();
    if (match(TokenType::T_BREAK)) return parseBreakStmt();

    auto expr = parseExpression();
    consume(TokenType::T_SEMICOLON, "Expect ';' after expression.");
    return std::make_unique<AST::ExpressionStmt>(std::move(expr));
}

std::vector<std::unique_ptr<AST::Statement>> Parser::parseBlock() {
    std::vector<std::unique_ptr<AST::Statement>> statements;
    while (!check(TokenType::T_RBRACE) && !isAtEnd()) {
        statements.push_back(parseStatement());
    }
    consume(TokenType::T_RBRACE, "Expect '}' after block.");
    return statements;
}

std::unique_ptr<AST::Statement> Parser::parseStructDef() {
    std::string name = consume(TokenType::T_IDENTIFIER, "Expect struct name.").value;
    consume(TokenType::T_LBRACE, "Expect '{' before struct body.");
    std::vector<AST::StructDef::Member> members;
    while (!check(TokenType::T_RBRACE) && !isAtEnd()) {
        std::string type;
        if (match(TokenType::T_INT)) type = "int";
        else if (match(TokenType::T_FLOAT)) type = "float";
        else if (match(TokenType::T_STRING)) type = "string";
        else if (match(TokenType::T_BOOL)) type = "bool";
        else throw std::runtime_error("Expect member type.");

        std::string mName = consume(TokenType::T_IDENTIFIER, "Expect member name.").value;
        consume(TokenType::T_SEMICOLON, "Expect ';' after member.");
        members.push_back({type, mName});
    }
    consume(TokenType::T_RBRACE, "Expect '}' after struct body.");
    return std::make_unique<AST::StructDef>(name, std::move(members));
}

std::unique_ptr<AST::Statement> Parser::parseClassDef() {
    std::string name = consume(TokenType::T_IDENTIFIER, "Expect class name.").value;
    consume(TokenType::T_LBRACE, "Expect '{' before class body.");
    std::vector<std::unique_ptr<AST::VarDeclaration>> props;
    std::vector<std::unique_ptr<AST::FunctionDef>> methods;
    while (!check(TokenType::T_RBRACE) && !isAtEnd()) {
        if (match(TokenType::T_FUNCTION)) {
            auto stmt = parseFunctionDef();
            methods.push_back(std::unique_ptr<AST::FunctionDef>(static_cast<AST::FunctionDef*>(stmt.release())));
        } else {
            if (match(TokenType::T_INT) || match(TokenType::T_FLOAT) || match(TokenType::T_STRING) || match(TokenType::T_BOOL)) {
                auto stmt = parseVarDeclaration();
                props.push_back(std::unique_ptr<AST::VarDeclaration>(static_cast<AST::VarDeclaration*>(stmt.release())));
            } else {
                throw std::runtime_error("Expect property or method in class.");
            }
        }
    }
    consume(TokenType::T_RBRACE, "Expect '}' after class body.");
    return std::make_unique<AST::ClassDef>(name, std::move(props), std::move(methods));
}

std::unique_ptr<AST::Statement> Parser::parseTryCatchStmt() {
    consume(TokenType::T_LBRACE, "Expect '{' for try block.");
    auto tryBlock = std::make_unique<AST::BlockStmt>(parseBlock());
    consume(TokenType::T_CATCH, "Expect 'catch' after try block.");
    consume(TokenType::T_LPAREN, "Expect '(' after catch.");
    std::string varName = consume(TokenType::T_IDENTIFIER, "Expect exception variable name.").value;
    consume(TokenType::T_RPAREN, "Expect ')' after exception variable.");
    consume(TokenType::T_LBRACE, "Expect '{' for catch block.");
    auto catchBlock = std::make_unique<AST::BlockStmt>(parseBlock());
    return std::make_unique<AST::TryCatchStmt>(std::move(tryBlock), varName, std::move(catchBlock));
}

std::unique_ptr<AST::Statement> Parser::parseThrowStmt() {
    auto value = parseExpression();
    consume(TokenType::T_SEMICOLON, "Expect ';' after throw.");
    return std::make_unique<AST::ThrowStmt>(std::move(value));
}

std::unique_ptr<AST::Statement> Parser::parseIfStmt() {
    consume(TokenType::T_LPAREN, "Expect '(' after 'if'.");
    auto condition = parseExpression();
    consume(TokenType::T_RPAREN, "Expect ')' after condition.");
    auto thenBranch = parseStatement();
    std::unique_ptr<AST::Statement> elseBranch = nullptr;
    if (match(TokenType::T_ELSE)) {
        elseBranch = parseStatement();
    }
    return std::make_unique<AST::IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

std::unique_ptr<AST::Statement> Parser::parseWhileStmt() {
    consume(TokenType::T_LPAREN, "Expect '(' after 'while'.");
    auto condition = parseExpression();
    consume(TokenType::T_RPAREN, "Expect ')' after condition.");
    auto body = parseStatement();
    return std::make_unique<AST::WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<AST::Statement> Parser::parseForStmt() {
    consume(TokenType::T_LPAREN, "Expect '(' after 'for'.");
    std::unique_ptr<AST::Statement> init = nullptr;
    if (match(TokenType::T_SEMICOLON)) { }
    else if (match(TokenType::T_INT) || match(TokenType::T_FLOAT) || match(TokenType::T_STRING) || match(TokenType::T_BOOL)) { init = parseVarDeclaration(); }
    else {
        auto expr = parseExpression();
        consume(TokenType::T_SEMICOLON, "Expect ';' after init expression.");
        init = std::make_unique<AST::ExpressionStmt>(std::move(expr));
    }

    std::unique_ptr<AST::Expression> condition = nullptr;
    if (!check(TokenType::T_SEMICOLON)) condition = parseExpression();
    consume(TokenType::T_SEMICOLON, "Expect ';' after loop condition.");

    std::unique_ptr<AST::Expression> update = nullptr;
    if (!check(TokenType::T_RPAREN)) {
        update = parseExpression();
    }
    consume(TokenType::T_RPAREN, "Expect ')' after for clauses.");

    auto body = parseStatement();
    return std::make_unique<AST::ForStmt>(std::move(init), std::move(condition), std::move(update), std::move(body));
}

std::unique_ptr<AST::Statement> Parser::parseReturnStmt() {
    consume(TokenType::T_LPAREN, "Expect '(' after 'return'.");
    std::unique_ptr<AST::Expression> value = nullptr;
    if (!check(TokenType::T_RPAREN)) value = parseExpression();
    consume(TokenType::T_RPAREN, "Expect ')' after return value.");
    consume(TokenType::T_SEMICOLON, "Expect ';' after return call.");
    return std::make_unique<AST::ReturnStmt>(std::move(value));
}

std::unique_ptr<AST::Statement> Parser::parseFunctionDef() {
    std::string name = consume(TokenType::T_IDENTIFIER, "Expect function name.").value;
    consume(TokenType::T_LPAREN, "Expect '(' after function name.");
    std::vector<AST::FunctionDef::Param> params;
    if (!check(TokenType::T_RPAREN)) {
        do {
            std::string type;
            if (match(TokenType::T_INT)) type = "int";
            else if (match(TokenType::T_FLOAT)) type = "float";
            else if (match(TokenType::T_STRING)) type = "string";
            else if (match(TokenType::T_BOOL)) type = "bool";
            else throw std::runtime_error("Expect parameter type.");

            std::string pName = consume(TokenType::T_IDENTIFIER, "Expect parameter name.").value;
            params.push_back({type, pName});
        } while (match(TokenType::T_COMMA));
    }
    consume(TokenType::T_RPAREN, "Expect ')' after parameters.");
    consume(TokenType::T_LBRACE, "Expect '{' before function body.");
    return std::make_unique<AST::FunctionDef>(name, params, parseBlock());
}

std::unique_ptr<AST::Statement> Parser::parseVarDeclaration() {
    std::string type = previous().value;
    std::vector<std::unique_ptr<AST::VariableDeclaration>> decls;
    bool isArray = false;
    
    do {
        std::string name = consume(TokenType::T_IDENTIFIER, "Expect variable name.").value;
        std::unique_ptr<AST::Expression> initializer = nullptr;
        
        if (match(TokenType::T_LBRACKET)) {
            consume(TokenType::T_RBRACKET, "Expect ']' after '['.");
            isArray = true;
        } else if (match(TokenType::T_EQUALS)) {
            initializer = parseExpression();
        }
        
        auto decl = std::make_unique<AST::VariableDeclaration>();
        decl->name = name;
        decl->initializer = std::move(initializer);
        decls.push_back(std::move(decl));
    } while (match(TokenType::T_COMMA));

    consume(TokenType::T_SEMICOLON, "Expect ';' after variable declaration.");
    
    return std::make_unique<AST::VarDeclaration>(type, std::move(decls), isArray);
}

std::unique_ptr<AST::Statement> Parser::parseImportStmt() {
    std::string first = consume(TokenType::T_IDENTIFIER, "Expect module or function name.").value;
    if (check(TokenType::T_IDENTIFIER) && peek().value == "from") {
        advance(); // consume "from"
        std::string fileName;
        if (match(TokenType::T_STRING_LITERAL)) {
            fileName = previous().value;
        } else {
            fileName = consume(TokenType::T_IDENTIFIER, "Expect filename.").value;
            if (match(TokenType::T_DOT)) fileName += "." + consume(TokenType::T_IDENTIFIER, "extension").value;
        }
        consume(TokenType::T_SEMICOLON, ";");
        return std::make_unique<AST::ImportFromStmt>(first, fileName);
    }
    consume(TokenType::T_SEMICOLON, ";");
    return std::make_unique<AST::ImportStmt>(first);
}

std::unique_ptr<AST::Expression> Parser::parseExpression() { return parseAssignment(); }

std::unique_ptr<AST::Expression> Parser::parseAssignment() {
    auto expr = parseLogicalOr();
    if (match(TokenType::T_EQUALS)) {
        auto value = parseAssignment();
        if (AST::VariableExpr* var = dynamic_cast<AST::VariableExpr*>(expr.get())) {
            return std::make_unique<AST::Assignment>(var->name, std::move(value));
        } else if (AST::ArrayIndexExpr* arrIdx = dynamic_cast<AST::ArrayIndexExpr*>(expr.get())) {
            auto released = std::unique_ptr<AST::ArrayIndexExpr>(static_cast<AST::ArrayIndexExpr*>(expr.release()));
            return std::make_unique<AST::ArrayIndexAssignment>(std::move(released), std::move(value));
        } else if (AST::MemberAccessExpr* memAcc = dynamic_cast<AST::MemberAccessExpr*>(expr.get())) {
            auto released = std::unique_ptr<AST::MemberAccessExpr>(static_cast<AST::MemberAccessExpr*>(expr.release()));
            return std::make_unique<AST::MemberAssignment>(std::move(released), std::move(value));
        }
        throw std::runtime_error("Invalid assignment target.");
    }
    return expr;
}

std::unique_ptr<AST::Expression> Parser::parseLogicalOr() {
    auto expr = parseLogicalAnd();
    while (match(TokenType::T_OR_OR)) {
        Token op = previous();
        auto right = parseLogicalAnd();
        expr = std::make_unique<AST::BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<AST::Expression> Parser::parseLogicalAnd() {
    auto expr = parseComparison();
    while (match(TokenType::T_AND_AND)) {
        Token op = previous();
        auto right = parseComparison();
        expr = std::make_unique<AST::BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<AST::Expression> Parser::parseComparison() {
    auto expr = parseAdditive();
    while (match(TokenType::T_EQUAL_EQUAL) || match(TokenType::T_BANG_EQUAL) ||
           match(TokenType::T_GREATER) || match(TokenType::T_GREATER_EQUAL) ||
           match(TokenType::T_LESS) || match(TokenType::T_LESS_EQUAL)) {
        Token op = previous();
        auto right = parseAdditive();
        expr = std::make_unique<AST::BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<AST::Expression> Parser::parseAdditive() {
    auto expr = parseMultiplicative();
    while (match(TokenType::T_PLUS) || match(TokenType::T_MINUS)) {
        Token op = previous();
        auto right = parseMultiplicative();
        expr = std::make_unique<AST::BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<AST::Expression> Parser::parseMultiplicative() {
    auto expr = parseUnary();
    while (match(TokenType::T_STAR) || match(TokenType::T_SLASH)) {
        Token op = previous();
        auto right = parseUnary();
        expr = std::make_unique<AST::BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<AST::Expression> Parser::parseUnary() {
    if (match(TokenType::T_MINUS) || match(TokenType::T_BANG)) {
        Token op = previous();
        auto operand = parseUnary();
        return std::make_unique<AST::UnaryExpr>(op, std::move(operand));
    }
    return parsePostfix();
}

std::unique_ptr<AST::Expression> Parser::parsePostfix() {
    auto expr = parsePrimary();
    while (true) {
        if (match(TokenType::T_LBRACKET)) {
            auto index = parseExpression();
            consume(TokenType::T_RBRACKET, "Expect ']' after array index.");
            expr = std::make_unique<AST::ArrayIndexExpr>(std::move(expr), std::move(index));
        } else if (match(TokenType::T_DOT)) {
            std::string name = consume(TokenType::T_IDENTIFIER, "Expect member name after '.'.").value;
            expr = std::make_unique<AST::MemberAccessExpr>(std::move(expr), name);
        } else if (match(TokenType::T_LPAREN)) {
            // 함수 또는 메서드 호출 처리
            std::vector<std::unique_ptr<AST::Expression>> args;
            if (!check(TokenType::T_RPAREN)) {
                do { args.push_back(parseExpression()); } while (match(TokenType::T_COMMA));
            }
            consume(TokenType::T_RPAREN, "Expect ')' after arguments.");

            // printf인 경우 특수 처리
            if (AST::VariableExpr* v = dynamic_cast<AST::VariableExpr*>(expr.get())) {
                if (v->name == "printf") {
                    if (args.empty()) throw std::runtime_error("printf expects a format string.");
                    // printf는 별도의 AST 노드를 사용하므로 여기서는 간단히 string literal만 지원하도록 제약하거나
                    // 기존 parsePrintfInterpolation 로직을 타게 해야 함.
                    // MVP에서는 간단히 처리하기 위해 CallExpr로 유지하고 Interpreter에서 처리.
                }
                expr = std::make_unique<AST::CallExpr>(v->name, std::move(args));
            } else if (AST::MemberAccessExpr* m = dynamic_cast<AST::MemberAccessExpr*>(expr.get())) {
                if (AST::VariableExpr* obj = dynamic_cast<AST::VariableExpr*>(m->object.get())) {
                    expr = std::make_unique<AST::CallExpr>(obj->name + "." + m->memberName, std::move(args));
                } else {
                    throw std::runtime_error("Complex method calls not supported yet.");
                }
            } else {
                throw std::runtime_error("Invalid call target.");
            }
        } else {
            break;
        }
    }
    if (match(TokenType::T_PLUS_PLUS) || match(TokenType::T_MINUS_MINUS)) {
        Token op = previous();
        if (auto* var = dynamic_cast<AST::VariableExpr*>(expr.get())) return std::make_unique<AST::PostfixExpr>(var->name, op);
        throw std::runtime_error("Invalid postfix operator target.");
    }
    return expr;
}

std::unique_ptr<AST::Expression> Parser::parsePrimary() {
    if (match(TokenType::T_INT_LITERAL)) return std::make_unique<AST::IntLiteral>(std::stoi(previous().value));
    if (match(TokenType::T_FLOAT_LITERAL)) return std::make_unique<AST::FloatLiteral>(std::stof(previous().value));
    if (match(TokenType::T_STRING_LITERAL)) return std::make_unique<AST::StringLiteral>(previous().value);
    if (match(TokenType::T_TRUE)) return std::make_unique<AST::BooleanLiteral>(true);
    if (match(TokenType::T_FALSE)) return std::make_unique<AST::BooleanLiteral>(false);
    if (match(TokenType::T_THIS)) return std::make_unique<AST::ThisExpr>();
    if (match(TokenType::T_NEW)) {
        std::string name = consume(TokenType::T_IDENTIFIER, "Expect name.").value;
        consume(TokenType::T_LPAREN, "(");
        std::vector<std::unique_ptr<AST::Expression>> args;
        if (!check(TokenType::T_RPAREN)) {
            do { args.push_back(parseExpression()); } while (match(TokenType::T_COMMA));
        }
        consume(TokenType::T_RPAREN, ")");
        return std::make_unique<AST::NewExpr>(name, std::move(args));
    }
    if (match(TokenType::T_LBRACKET)) {
        std::vector<std::unique_ptr<AST::Expression>> elements;
        if (!check(TokenType::T_RBRACKET)) {
            do { elements.push_back(parseExpression()); } while (match(TokenType::T_COMMA));
        }
        consume(TokenType::T_RBRACKET, "]");
        return std::make_unique<AST::ArrayLiteralExpr>(std::move(elements));
    }
    if (match(TokenType::T_IDENTIFIER)) {
        std::string name = previous().value;
        // printf 특수 처리는 parsePostfix에서 수행하거나 여기서 미리 체크 가능
        if (name == "printf" && check(TokenType::T_LPAREN)) {
             advance(); // consume (
             std::string fmt = consume(TokenType::T_STRING_LITERAL, "printf fmt").value;
             consume(TokenType::T_RPAREN, ")");
             return parsePrintfInterpolation(fmt);
        }
        return std::make_unique<AST::VariableExpr>(name);
    }
    if (match(TokenType::T_LPAREN)) {
        auto expr = parseExpression();
        consume(TokenType::T_RPAREN, ")"); return expr;
    }
    throw std::runtime_error("Expect expression.");
}

std::unique_ptr<AST::Expression> Parser::parsePrintfInterpolation(const std::string& str) {
    std::vector<AST::PrintfExpr::Part> parts;
    size_t start = 0;
    while (true) {
        size_t braceStart = str.find('{', start);
        if (braceStart == std::string::npos) { parts.push_back({false, str.substr(start), nullptr}); break; }
        if (braceStart > start) parts.push_back({false, str.substr(start, braceStart - start), nullptr});
        size_t braceEnd = str.find('}', braceStart);
        if (braceEnd == std::string::npos) throw std::runtime_error("Unclosed {");
        Lexer l(str.substr(braceStart+1, braceEnd-braceStart-1) + ";");
        auto ts = l.tokenize(); Parser p(ts);
        auto pr = p.parse();
        if (auto* es = dynamic_cast<AST::ExpressionStmt*>(pr->statements[0].get()))
            parts.push_back({true, "", std::move(es->expr)});
        start = braceEnd + 1;
    }
    return std::make_unique<AST::PrintfExpr>(std::move(parts));
}

std::unique_ptr<AST::Statement> Parser::parseBreakStmt() {
    consume(TokenType::T_LPAREN, "("); consume(TokenType::T_RPAREN, ")"); consume(TokenType::T_SEMICOLON, ";");
    return std::make_unique<AST::BreakStmt>();
}

std::unique_ptr<AST::Statement> Parser::parseSwitchStmt() {
    consume(TokenType::T_LPAREN, "("); auto disc = parseExpression(); consume(TokenType::T_RPAREN, ")");
    consume(TokenType::T_LBRACE, "{"); std::vector<AST::SwitchStmt::Case> cases;
    while (!check(TokenType::T_RBRACE) && !isAtEnd()) {
        if (match(TokenType::T_CASE)) {
            consume(TokenType::T_LPAREN, "("); auto val = parseExpression(); consume(TokenType::T_RPAREN, ")");
            consume(TokenType::T_LBRACE, "{"); auto body = parseBlock(); cases.push_back({std::move(val), std::move(body)});
        } else if (match(TokenType::T_DEFAULT)) {
            consume(TokenType::T_LBRACE, "{"); auto body = parseBlock(); cases.push_back({nullptr, std::move(body)});
        }
    }
    consume(TokenType::T_RBRACE, "}");
    return std::make_unique<AST::SwitchStmt>(std::move(disc), std::move(cases));
}

} // namespace Flux
