#pragma once
#include <string>
#include <utility>
#include <vector>

// base class for expr nodes
class ExprAST {
public:
    virtual ~ExprAST() = default;
};

// expr class for numeric literals
class NumberExprAst : public ExprAST {
    double Val;

public:
    explicit NumberExprAst(const double Val) : Val(Val) {}
};

// expr class for referencing a var
class VariableExprAst : public ExprAST {
    std::string Name;

public:
    explicit VariableExprAst(std::string  Name) : Name(std::move(Name)) {}
};


// expr class for a binary operation
class BinaryExprAst : public ExprAST {
    char Op;
    std::unique_ptr<ExprAST> Lhs, Rhs;

public:
    BinaryExprAst(const char Op, std::unique_ptr<ExprAST> Lhs, std::unique_ptr<ExprAST> Rhs)
        : Op(Op), Lhs(std::move(Lhs)), Rhs(std::move(Rhs)) {}
};

class CallExprAst : public ExprAST {
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;

public:
    CallExprAst(std::string  Callee, std::vector<std::unique_ptr<ExprAST>> Args)
        : Callee(std::move(Callee)), Args(std::move(Args)) {}
};

// represents the prototype for a function
// captures its name, its argument names
class PrototypeAST {
    std::string Name;
    std::vector<std::string> Args;

public:
    PrototypeAST(std::string Name, std::vector<std::string> Args)
        : Name(std::move(Name)), Args(std::move(Args)) {}

    [[nodiscard]] const std::string& getName() const { return Name; }
};

// represents a function definition itself
class FunctionAST {
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto, std::unique_ptr<ExprAST> Body)
        : Proto(std::move(Proto)), Body(std::move(Body)) {}
};