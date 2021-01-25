#include "scanner.h"
#include <iostream>

int main(int argc, char* argv[]) {
    bool debug = true; // TODO: This is always true for now, but make it an argument eventually.

    if (argc < 2) {
        std::cout << ("ERROR: Missing filename argument\n");
        return 1;
    }

    Scanner s(debug);

    if (!s.openFile(argv[1])) {
        std::cout << ("ERROR: Could not open file: %s\n", argv[1]);
        return 1;
    }

    Token token;
    do {
        token = s.scan();
    } while (token.type != T_EOF);

    return 0;
}