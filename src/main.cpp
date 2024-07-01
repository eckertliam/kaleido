//
// Created by Liam Eckert on 6/27/24.
//

#include <cstdio>
#include <map>
#include <utility>
#include <string>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Value.h"

using namespace llvm;

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

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

//===----------------------------------------------------------------------===//
// AST
//===----------------------------------------------------------------------===//

namespace {

    class ExprAst {
    public:
        virtual ~ExprAst() = default;

        virtual llvm::Value *codegen() = 0;
    };

/// NumberExprAst - Expression class for numeric literals
    class NumExprAst : public ExprAst {
        double Val;
    public:
        NumExprAst(double Val) : Val(Val) {};

        Value* codegen() override;
    };

///  VarExprAst - class for refing a var
    class VarExprAst : public ExprAst {
        std::string Name;
    public:
        VarExprAst(std::string Name) : Name(std::move(Name)) {};
        Value* codegen() override;
    };

/// BinExprAst - Expression for binary ops
    class BinExprAst : public ExprAst {
        char Op;
        std::unique_ptr<ExprAst> Lhs, Rhs;
    public:
        BinExprAst(char Op, std::unique_ptr<ExprAst> Lhs, std::unique_ptr<ExprAst> Rhs)
                : Op(Op), Lhs(std::move(Lhs)), Rhs(std::move(Rhs)) {};
        Value* codegen() override;
    };

/// CallExprAst - Expression class for function calls
    class CallExprAst : public ExprAst {
        std::string Callee;
        std::vector<std::unique_ptr<ExprAst>> Args;
    public:
        CallExprAst(std::string Callee, std::vector<std::unique_ptr<ExprAst>> Args)
                : Callee(std::move(Callee)), Args(std::move(Args)) {};
        Value* codegen() override;
    };

/// PrototypeAst - this class represents the prototype for a function,
/// which captures its name, and its argument names
    class PrototypeAst {
        std::string Name;
        std::vector<std::string> Args;
    public:
        PrototypeAst(std::string Name, std::vector<std::string> Args)
                : Name(std::move(Name)), Args(std::move(Args)) {};

        [[nodiscard]] const std::string &getName() const { return Name; };

        Function *codegen();
    };

/// FunctionAst - This class represents a function definition itself
    class FunctionAst {
        std::unique_ptr<PrototypeAst> Proto;
        std::unique_ptr<ExprAst> Body;
    public:
        FunctionAst(std::unique_ptr<PrototypeAst> Proto, std::unique_ptr<ExprAst> Body)
                : Proto(std::move(Proto)), Body(std::move(Body)) {};
        Function *codegen();
    };

}
//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

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

//===----------------------------------------------------------------------===//
// Compiler
//===----------------------------------------------------------------------===//

static std::unique_ptr<LLVMContext> TheContext;
static std::unique_ptr<IRBuilder<>> Builder;
static std::unique_ptr<Module> TheModule;
static std::map<std::string, Value*> NamedValues;

Value* LogErrorV(const char* Str) {
    LogError(Str);
    return nullptr;
}

Value* NumExprAst::codegen() {
    return ConstantFP::get(*TheContext, APFloat(Val));
}

Value* VarExprAst::codegen() {
    Value* V = NamedValues[Name];
    if (!V)
        LogErrorV("Unknown variable name");
    return V;
}

Value* BinExprAst::codegen() {
    Value *L = Lhs->codegen();
    Value *R = Rhs->codegen();
    if (!L || !R)
        return nullptr;

    switch (Op) {
        case '+':
            return Builder->CreateFAdd(L, R, "addtmp");
        case '-':
            return Builder->CreateFSub(L, R, "subtmp");
        case '*':
            return Builder->CreateFMul(L, R, "multmp");
        case '<':
            L = Builder->CreateFCmpULT(L, R, "cmptmp");
            // Convert bool 0/1 to double 0.0 or 1.0
            return Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*TheContext), "booltmp");
        default:
            return LogErrorV("invalid binary operator");
    }
}

Value* CallExprAst::codegen() {
    // look up the name in the global module table
    Function *CalleeF = TheModule->getFunction(Callee);
    if (!CalleeF)
        return LogErrorV("Unknown function referenced");

    // if arg mismatch err
    if (CalleeF->arg_size() != Args.size())
        return LogErrorV("Incorrect # args passed");

    std::vector<Value *> ArgsV;
    for (const auto & Arg : Args) {
        ArgsV.push_back(Arg->codegen());
        if (!ArgsV.back())
            return nullptr;
    }
    return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

Function* PrototypeAst::codegen() {
    // Make the function type: double(double, double) etc
    std::vector<Type*> Doubles(Args.size(), Type::getDoubleTy(*TheContext));
    FunctionType *Ft = FunctionType::get(Type::getDoubleTy(*TheContext), Doubles, false);
    Function *F = Function::Create(Ft, Function::ExternalLinkage, Name, TheModule.get());
    unsigned Idx = 0;
    for (auto &Arg : F->args())
        Arg.setName(Args[Idx++]);
    return F;
}

Function *FunctionAst::codegen() {
    // first check for an existing from a previous extern decl
    Function *TheFunction = TheModule->getFunction(Proto->getName());

    if (!TheFunction)
        TheFunction = Proto->codegen();

    if (!TheFunction)
        return nullptr;

    if (!TheFunction->empty())
        return (Function*)LogErrorV("Function cannot be redefined");
    // create a new basic block to start insertion into
    BasicBlock *Bb = BasicBlock::Create(*TheContext, "entry", TheFunction);
    Builder->SetInsertPoint(Bb);

    // record the function args in the NamedValues map
    NamedValues.clear();
    for (auto &Arg : TheFunction->args())
        NamedValues[std::string(Arg.getName())] = &Arg;

    if (Value *RetVal = Body->codegen()) {
        // finish off the function
        Builder->CreateRet(RetVal);

        // validate the generated code, checking for consistency
        verifyFunction(*TheFunction);

        return TheFunction;
    }
    // error reading body, remove func
    TheFunction->eraseFromParent();
    return nullptr;
}

//===----------------------------------------------------------------------===//
// Top-Level parsing and JIT driver
//===----------------------------------------------------------------------===//

static void InitializeModule() {
    // Open a new context and module.
    TheContext = std::make_unique<LLVMContext>();
    TheModule = std::make_unique<Module>("my cool jit", *TheContext);

    // Create a new builder for the module.
    Builder = std::make_unique<IRBuilder<>>(*TheContext);
}

static void HandleDefinition() {
    if (auto FnAST = ParseDefinition()) {
        if (auto *FnIR = FnAST->codegen()) {
            fprintf(stderr, "Read function definition:");
            FnIR->print(errs());
            fprintf(stderr, "\n");
        }
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

static void HandleExtern() {
    if (auto ProtoAST = ParseExtern()) {
        if (auto *FnIR = ProtoAST->codegen()) {
            fprintf(stderr, "Read extern: ");
            FnIR->print(errs());
            fprintf(stderr, "\n");
        }
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

static void HandleTopLevelExpression() {
    // Evaluate a top-level expression into an anonymous function.
    if (auto FnAST = ParseTopLevelExpr()) {
        if (auto *FnIR = FnAST->codegen()) {
            fprintf(stderr, "Read top-level expression:");
            FnIR->print(errs());
            fprintf(stderr, "\n");

            // Remove the anonymous expression.
            FnIR->eraseFromParent();
        }
    } else {
        // Skip token for error recovery.
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
            case ';': // ignore top-level semicolons.
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