//
// Created by Liam Eckert on 6/27/24.
//

#ifndef KALEIDO_PARSER_H
#define KALEIDO_PARSER_H

#include <utility>
#include "Scanner.h"
#include "Ast.h"

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
static int CurTok;
static inline int getNextToken();

/// LogError* - These are little helper functions for error handling.
std::unique_ptr<ExprAst> LogError(const char* Str);
std::unique_ptr<PrototypeAst> LogErrorP(const char* Str);

/// numberexpr ::= number
static std::unique_ptr<ExprAst> ParseNumExpr();

/// parenexpr ::= '(' expression ')'
static std::unique_ptr<ExprAst> ParseParenExpr();

/// identexpr
///    ::= ident
///    ::= ident '(' expression ')'
static std::unique_ptr<ExprAst> ParseIdentExpr();

/// primary
///    ::= identexpr
///    ::= numberexpr
///    ::= parenexpr
static std::unique_ptr<ExprAst> ParsePrimary();

#endif //KALEIDO_PARSER_H
