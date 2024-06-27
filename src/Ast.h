//
// Created by Liam Eckert on 6/27/24.
//

#ifndef KALEIDO_AST_H
#define KALEIDO_AST_H

#include <string>
#include <utility>
#include <vector>

class ExprAst {
public:
    virtual ~ExprAst() = default;
};

class NumExprAst : public ExprAst {
    double Val;
public:
    NumExprAst(double Val) : Val(Val) {};
};

///  VarExprAst - class for refing a var
class VarExprAst : public ExprAst {
    std::string Name;
public:
    VarExprAst(std::string Name) : Name(std::move(Name)) {};
};

/// BinExprAst - Expression for binary ops
class BinExprAst : public ExprAst {
    char Op;
    std::unique_ptr<ExprAst> Lhs, Rhs;
public:
    BinExprAst(char Op, std::unique_ptr<ExprAst> Lhs, std::unique_ptr<ExprAst> Rhs)
            : Op(Op), Lhs(std::move(Lhs)), Rhs(std::move(Rhs)) {};
};

/// CallExprAst - Expression class for function calls
class CallExprAst : public ExprAst {
    std::string Callee;
    std::vector<std::unique_ptr<ExprAst>> Args;
public:
    CallExprAst(std::string Callee, std::vector<std::unique_ptr<ExprAst>> Args)
            : Callee(std::move(Callee)), Args(std::move(Args)) {};
};

/// PrototypeAst - this class represents the prototype for a function,
/// which captures its name, and its argument names
class PrototypeAst {
    std::string Name;
    std::vector<std::string> Args;
public:
    PrototypeAst(std::string Name, std::vector<std::string> Args)
            : Name(std::move(Name)), Args(std::move(Args)) {};

    [[nodiscard]] const std::string& getName() const { return Name; };
};

/// FunctionAst - This class represents a function definition itself
class FunctionAst {
    std::unique_ptr<PrototypeAst> Proto;
    std::unique_ptr<ExprAst> Body;
public:
    FunctionAst(std::unique_ptr<PrototypeAst> Proto, std::unique_ptr<ExprAst> Body)
            : Proto(std::move(Proto)), Body(std::move(Body)) {};
};

#endif //KALEIDO_AST_H
