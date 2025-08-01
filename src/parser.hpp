#pragma once

#include <map>

#include "lexer.hpp"
#include "ast.hpp"

/// CurTok/getNextToken - provides a token buffer
/// CurTok is the current token the parser is looking at
static int CurTok;
/// getNextToken reads another token from the lexer and updates CurTok with its results
static int getNextToken() {
    return CurTok = gettok();
}

/// Helper for error handling with expressions
inline std::unique_ptr<ExprAST> LogError(const char *Str) {
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}

/// Helper for error handling with functions
inline std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
    LogError(Str);
    return nullptr;
}

/// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr() {
    auto Result = std::make_unique<NumberExprAst>(NumVal);
    getNextToken(); // consume num
    return std::move(Result);
}

// forward decl
static std::unique_ptr<ExprAST> ParseExpression();

/// parenexpr ::= '(' expression ')'
static std::unique_ptr<ExprAST> ParseParenExpr() {
    getNextToken(); // consume (
    auto V = ParseExpression();
    if (!V)
        return nullptr;

    if (CurTok != ')')
        return LogError("Unmatched ')'");
    getNextToken(); // consume )
    return V;
}

/// identifierexpr
///    ::= identifier
///    ::= identifier '(' expression* ')'
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
    std::string IdName = IdentifierStr;

    getNextToken(); // eat ident

    if (CurTok != '(') // simple var ref
        return std::make_unique<VariableExprAst>(IdName);

    // Call
    getNextToken(); // consume (
    std::vector<std::unique_ptr<ExprAST>> Args;
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

    // consume the )
    getNextToken();

    return std::make_unique<CallExprAst>(IdName, std::move(Args));
}

/// primary
///    ::= identifierexpr
///    ::= numberexpr
///    ::= parenexpr
static std::unique_ptr<ExprAST> ParsePrimary() {
    switch (CurTok) {
        default:
            return LogError("Unknown token when expecting an expression");
        case tok_identifier:
            return ParseIdentifierExpr();
        case tok_number:
            return ParseNumberExpr();
        case '(':
            return ParseParenExpr();
    }
}


/// holds the prec of each binop that is defined
static std::map<char, int> BinopPrecedence;

/// get the prec of the pending binop token
static int GetTokPrecedence() {
    if (!isascii(CurTok))
        return -1;

    // make sure it's a declared binop
    const int TokPrec = BinopPrecedence[CurTok];
    if (TokPrec <= 0) return -1;
    return TokPrec;
}


static std::unique_ptr<ExprAST> ParseBinOpRhs(const int ExprPrec, std::unique_ptr<ExprAST> Lhs);

/// expression
///   ::= primary binoprhs
static std::unique_ptr<ExprAST> ParseExpression() {
    auto Lhs = ParsePrimary();
    if (!Lhs)
        return nullptr;

    return ParseBinOpRhs(0, std::move(Lhs));
}

/// binoprhs
///    ::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRhs(const int ExprPrec, std::unique_ptr<ExprAST> Lhs) {
    // if this is a binop find the prec
    while (true) {
        int TokPrec = GetTokPrecedence();

        // if this is a binop that binds at least as tight as the current binop
        // consume it otherwise we're done
        if (TokPrec < ExprPrec)
            return Lhs;

        // we know we have a binop
        int BinOp = CurTok;
        getNextToken();

        // parse the primary expr after the operator
        auto Rhs = ParsePrimary();
        if (!Rhs)
            return nullptr;

        // if binop binds less tightly with rhs than the operator after rhs
        // let the pending operator takes rhs as its lhs
        int NextPrec = GetTokPrecedence();
        if (TokPrec < NextPrec) {
            Rhs = ParseBinOpRhs(TokPrec + 1, std::move(Rhs));
            if (!Rhs)
                return nullptr;
        }

        // merge lhs/rhs
        Lhs = std::make_unique<BinaryExprAst>(BinOp, std::move(Lhs), std::move(Rhs));
    }
}

/// prototype
///     ::= id '(' id* ')'
static std::unique_ptr<PrototypeAST> ParsePrototype() {
    if (CurTok != tok_identifier)
        return LogErrorP("Expected function nmae in prototype");

    std::string FnName = IdentifierStr;
    getNextToken();

    if (CurTok != '(')
        return LogErrorP("Expected '(' in prototype");

    // read the list of argument names
    std::vector<std::string> ArgNames;
    while (getNextToken() == tok_identifier)
        ArgNames.push_back(IdentifierStr);

    if (CurTok != ')')
        return LogErrorP("Expected ')' in prototype");

    // success
    getNextToken(); // consume )

    return std::make_unique<PrototypeAST>(std::move(FnName), std::move(ArgNames));
}

/// definition ::= 'def' prototype expression
static std::unique_ptr<FunctionAST> ParseDefinition() {
    getNextToken(); // consume def
    auto Proto = ParsePrototype();
    if (!Proto) return nullptr;

    if (auto E = ParseExpression())
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    return nullptr;
}


/// external ::= 'extern' prototype
static std::unique_ptr<PrototypeAST> ParseExtern() {
    getNextToken();
    return ParsePrototype();
}

/// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
    if (auto E = ParseExpression()) {
        // make an anon proto
        auto Proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

static void HandleDefinition() {
    if (ParseDefinition()) {
        fprintf(stderr, "Parsed a function definition.\n");
    } else {
        // skip token for err recovery
        getNextToken();
    }
}

static void HandleExtern() {
    if (ParseExtern()) {
        fprintf(stderr, "Parsed an extern\n");
    } else {
        // skip token for error recovery
        getNextToken();
    }
}

static void HandleTopLevelExpression() {
    if (ParseTopLevelExpr()) {
        fprintf(stderr, "Parsed a top-level expression.\n");
    } else {
        // skip token for error recovery
        getNextToken();
    }
}

/// top ::= definition | external | expression | ';'
static void MainLoop() {
    while (true) {
        fprintf(stderr, "ready> ");
        switch (CurTok) {
            case tok_eof:
                return;
            case ';':
                getNextToken();
                break;
            case tok_def:
                HandleDefinition();
                break;
            case tok_extern:
                HandleExtern();
                break;
            default:
                HandleTopLevelExpression();
                break;
        }
    }
}


