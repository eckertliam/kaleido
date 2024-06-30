//
// Created by Liam Eckert on 6/27/24.
//


#include <cstdio>
#include "Ast.h"
#include <map>
#include <utility>
#include <string>

enum Token {
    tok_eof = -1,

    // commands
    tok_def = -2,
    tok_extern = -3,

    //primary
    tok_ident = -4,
    tok_num = -5,
};

// holds the name of an ident
static std::string IdentStr;
// holds numeric literals
static double NumVal;

// returns the next token from std input
static int gettok() {
    static int LastChar = ' ';

    // skip any ws
    while (isspace(LastChar))
        LastChar = getchar();

    // identifier pass
    if (isalpha(LastChar)) {
        IdentStr = std::to_string(LastChar);

        while (isalnum(LastChar = getchar()))
            IdentStr += std::to_string(LastChar);

        if (IdentStr == "def")
            return tok_def;
        if (IdentStr == "extern")
            return tok_extern;
        return tok_ident;
    }

    // number pass
    if (isdigit(LastChar)) {
        std::string NumStr;
        do {
            NumStr += std::to_string(LastChar);
            LastChar = getchar();
        } while (isdigit(LastChar));

        if (LastChar == '.') {
            do {
                NumStr += std::to_string(LastChar);
                LastChar = getchar();
            } while (isdigit(LastChar));
        }

        NumVal = strtod(NumStr.c_str(), 0);
        return tok_num;
    }

    // comment pass
    if (LastChar == '#') {
        do
            LastChar = getchar();
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF)
            return gettok();
    }

    // check for eof, dont eat eof
    if (LastChar == EOF)
        return tok_eof;

    // otherwise return char as its ascii value
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
static int CurTok;
static int getNextToken() {
    return CurTok = gettok();
};

/// BinopPrecedence - holds the precedence for each binary operator
static std::map<char, int> BinopPrecedence = {
        // 1 is lowest precedence
        {'<', 10},
        {'+', 20},
        {'-', 20},
        {'*', 40}, // highest
};

/// GetTokPrecedence - Get the precedence of the pending binop token
static int GetTokPrecedence() {
    if (!isascii(CurTok))
        return -1;

    // make sure it's a declared binop
    int TokPrec = BinopPrecedence[CurTok];
    if (TokPrec <= 0) return -1;
    return TokPrec;
}

/// LogError* - These are little helper functions for error handling.
static std::unique_ptr<ExprAst> LogError(const char *Str) {
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
};

static std::unique_ptr<PrototypeAst> LogErrorP(const char *Str) {
    LogError(Str);
    return nullptr;
};

static std::unique_ptr<ExprAst> ParseExpression();

/// numberexpr ::= number
static std::unique_ptr<ExprAst> ParseNumExpr() {
    auto Result = std::make_unique<NumExprAst>(NumVal);
    getNextToken(); // consume the number
    return std::move(Result);
};

/// parenexpr ::= '(' expression ')'
static std::unique_ptr<ExprAst> ParseParenExpr() {
    getNextToken(); // eat (
    auto V = ParseExpression();
    if (!V)
        return nullptr;

    if (CurTok != ')')
        return LogError("expected ')'");
    getNextToken(); // eat )
    return V;
};

/// identexpr
///    ::= ident
///    ::= ident '(' expression ')'
static std::unique_ptr<ExprAst> ParseIdentExpr() {
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

/// primary
///    ::= identexpr
///    ::= numberexpr
///    ::= parenexpr
static std::unique_ptr<ExprAst> ParsePrimary() {
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

/// binoprhs
///    ::= ('+' primary)*
static std::unique_ptr<ExprAst> ParseBinopRhs(int ExprPrec, std::unique_ptr<ExprAst> Lhs) {
    // if this is a binop find its prec
    while (true) {
        int TokPrec = GetTokPrecedence();
        // if this is a binop that binds at least as tightly as the current binop.
        // consume it, otherwise we are done
        if (TokPrec < ExprPrec)
            return Lhs;
        // we know it is a binop
        int Binop = CurTok;
        getNextToken(); // eat binop
        // parse the primary expression after the binary operator
        auto Rhs = ParsePrimary();
        if (!Rhs)
            return nullptr;
        // if Binop binds less tightly with Rhs than the operator after Rhs let
        // the pending op take Rhs as its Lhs
        int NextPrec = GetTokPrecedence();
        if (TokPrec < NextPrec) {
            Rhs = ParseBinopRhs(TokPrec + 1, std::move(Rhs));
            if (!Rhs)
                return nullptr;
        }
        Lhs = std::make_unique<BinExprAst>(Binop, std::move(Lhs), std::move(Rhs));
    }
}

/// expression
///    ::= primary binoprhs
///
static std::unique_ptr<ExprAst> ParseExpression() {
    auto Lhs = ParsePrimary();
    if (!Lhs)
        return nullptr;

    return ParseBinopRhs(0, std::move(Lhs));
}

/// prototype
///    ::= id '(' id* ')'
static std::unique_ptr<PrototypeAst> ParsePrototype() {
    if (CurTok != tok_ident)
        return LogErrorP("Expected function name in prototype");

    std::string FnName = IdentStr;
    getNextToken();

    if (CurTok != '(')
        return LogErrorP("Expected '(' in prototype");

    // read the list of arg names
    std::vector<std::string> ArgNames;
    while (getNextToken() == tok_ident)
        ArgNames.push_back(IdentStr);
    if (CurTok != ')')
        return LogErrorP("Expected ')' in prototype");

    // success
    getNextToken(); // eat ')'

    return std::make_unique<PrototypeAst>(FnName, std::move(ArgNames));
}

/// definition ::= 'def' prototype expression
static std::unique_ptr<FunctionAst> ParseDefinition() {
    getNextToken(); // eat def
    auto Proto = ParsePrototype();
    if (!Proto) return nullptr;

    if (auto E = ParseExpression())
        return std::make_unique<FunctionAst>(std::move(Proto), std::move(E));
    return nullptr;
}

/// external ::= 'extern' prototype
static std::unique_ptr<PrototypeAst> ParseExtern() {
    getNextToken(); // eat extern
    return ParsePrototype();
}

/// toplevelexpr ::= expression
static std::unique_ptr<FunctionAst> ParseTopLevelExpr() {
    if (auto E = ParseExpression()) {
        // make an anonymous proto
        auto Proto = std::make_unique<PrototypeAst>("", std::vector<std::string>());
        return std::make_unique<FunctionAst>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

static void HandleDefinition() {
    if (ParseDefinition()) {
        fprintf(stderr, "Parsed a function definition.\n");
    } else {
        getNextToken();
    }
}

static void HandleExtern() {
    if (ParseExtern()) {
        fprintf(stderr, "Parsed an extern\n");
    } else {
        getNextToken();
    }
}

static void HandleTopLevelExpression() {
    if (ParseTopLevelExpr()) {
        fprintf(stderr, "Parsed a top-level expr\n");
    } else {
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

int main() {
    fprintf(stderr, "ready> ");
    getNextToken();

    MainLoop();

    return 0;
}