#include "parser.hpp"

int main () {
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40; // highest

    // prime the first token
    fprintf(stderr, "ready> ");
    getNextToken();

    // run the main loop
    MainLoop();

    return 0;
}