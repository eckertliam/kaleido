//
// Created by Liam Eckert on 6/27/24.
//

#include "Scanner.h"

// returns the next token from std input
int gettok() {
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
};
