//
// Created by Liam Eckert on 6/27/24.
//

#include "Parser.h"

static inline int getNextToken() {
    return CurTok = gettok();
}

std::unique_ptr<ExprAst> LogError(const char *Str) {
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}

std::unique_ptr<PrototypeAst> LogErrorP(const char *Str) {
    LogError(Str);
    return nullptr;
}

std::unique_ptr<ExprAst> ParseNumExpr() {
    auto Result = std::make_unique<NumExprAst>(NumVal);
    getNextToken(); // consume the number
    return std::move(Result);
}

std::unique_ptr<ExprAst> ParseParenExpr() {
    getNextToken(); // eat (
    auto V = ParseExpression();
    if (!V)
        return nullptr;

    if (CurTok != ')')
        return LogError("expected ')'");
    getNextToken(); // eat )
    return V;
}

std::unique_ptr<ExprAst> ParseIdentExpr() {
    std::string IdName = IdentStr;

    getNextToken(); // eat identifier

    if (CurTok != '(') // simple var ref
        return std::make_unique<VarExprAst>(IdName);

    // call
    getNextToken(); // eat (
    std::vector<std::unique_ptr<ExprAst>> Args;
    if (CurTok != ')') {
        while (true) {
            if (auto Arg = ParseExpression())
                Args.push_back(std::move(Arg));
            else
                return nullptr;

            if (CurTok == ')')
                break;

            if (CurTok != ',')
                return LogError("Expected ')' or ',' in argument list");
            getNextToken();
        }
    }
    // eat the ')'
    getNextToken();

    return std::make_unique<CallExprAst>(IdName, std::move(Args));
}

std::unique_ptr<ExprAst> ParsePrimary() {
    switch (CurTok) {
        default:
            return LogError("unknown token when expecting an expression");
        case tok_ident:
            return ParseIdentExpr();
        case tok_num:
            return ParseNumExpr();
        case '(':
            return ParseParenExpr();
    }
}
