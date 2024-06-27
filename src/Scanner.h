//
// Created by Liam Eckert on 6/27/24.
//

#ifndef KALEIDO_SCANNER_H
#define KALEIDO_SCANNER_H

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

// each token is returned as an enum or as an ascii char
static int gettok();

#endif //KALEIDO_SCANNER_H
