#include "scanner.h"
#include "tokentypes.h"
#include "token.h"
#include "parser.h"
#include "scopeMap.h"
#include <iostream>

void invalidCommand() {
	cout << "Invalid command line arguments. Example: [ --help | --h | --debug | --d ] filename" << endl;
	return;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << ("ERROR: No filename argument provided\n");
        return 0;
    }

	bool debug = false;
	string filename;
	if ((argc == 2) || (argc == 3)) {
		if ((string(argv[1]) == "--help") || (string(argv[1]) == "--h")) {
			std::cout << "\nThis is a compiler written for the University of Cincinnati class: EECE5183 Compiler Theory" << endl;
			std::cout << "\nThe compiler is an LL(1) recursive descent compiler that uses C++ to scan, parse, and type check the program and LLVM to generate the compiler backend." << endl;
			std::cout << "\nTo use this compiler, compile and then run from the command line using the arguments: [ --help | --h | --debug | --d ] filename." << endl;
			std::cout << "\nThe compiler will scan and parse your file and generate code if parsing is successful. Otherwise relevant errors and warnings will be shown." << endl;
			std::cout << "\n--debug or --d argument will print out each token as it is scanned and print out each scope's symbol table after the scope is exited." << endl;
			return 0;
		}
		else if ((string(argv[1]) == "--debug") || (string(argv[1]) == "--d")) {
			debug = true;
			filename = string(argv[2]);
		}
		else if (argc == 2) {
			debug = false;
			filename = string(argv[1]);
		}
	}
	else {
		invalidCommand();
		return 0;
	}

    // Initializing the scanner
    Scanner* scanner = new Scanner;

    // Contains token currently being scanned/parsed
    Token* curr_token = new Token;

    scanner->token = curr_token;

	// Initializing symbol table
	scopeMap* scopes = new scopeMap(debug);

    // Initialize scanner, then begin parsing if there are no errors
    if (scanner->startScanner(filename, debug)) {
		do {
			*curr_token = scanner->getToken();
			Parser parser(curr_token, scanner, scopes);
			//std::cout << "{" << curr_token->type << ", " << curr_token->ascii << "}" << endl;
		} while (curr_token->type != T_EOF);
    }
    delete scanner;
	delete scopes;

    return 0;
}