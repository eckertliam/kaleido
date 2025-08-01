#pragma once
#include <string>


enum Token {
    tok_eof = -1,

    // commands
    tok_def = -2,
    tok_extern = -3,

    // primary
    tok_identifier = -4,
    tok_number = -5,
};

static std::string IdentifierStr; // filled in if tok_identifier
static double NumVal; // filled in if tok_number

static int gettok() {
    static int LastChar = ' ';

    // skip any whitespace
    while (isspace(LastChar))
        LastChar = getchar();

    if (isalpha(LastChar)) { // identifier
        IdentifierStr = LastChar;

        while (isalnum(LastChar = getchar()))
            IdentifierStr += LastChar;

        if (IdentifierStr == "def")
            return tok_def;
        if (IdentifierStr == "extern")
            return tok_extern;
        return tok_identifier;
    }

    if (isdigit(LastChar) || LastChar == '.') { // number
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), nullptr);
        return tok_number;
    }

    if (LastChar == '#') {
        // comment until eol
        do
            LastChar = getchar();
        while (LastChar != '\n' && LastChar != EOF && LastChar != '\r');

        if (LastChar != EOF)
            return gettok();
    }

    // check for eof but dont consume
    if (LastChar == EOF)
        return tok_eof;

    // otherwise just return the char as its ascii value
    const int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}