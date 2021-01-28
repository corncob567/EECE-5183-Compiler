#ifndef SCANNER_H
#define SCANNER_H

#include <string>
#include <map>
#include <cstdio>
#include <stdlib.h>
#include "tokentypes.h"
#include "token.h"

using namespace std;
class Scanner
{
private:
	int line_number;
	Token return_token;
	FILE* fPtr = nullptr;
	bool debug = false;
	map<string, int> reserved_table;
	int scanToken(FILE* fPtr, Token* token);
	bool isNum(char character);
	bool isLetter(char character);
	bool isString(char character);
	bool isChar(char character);
	bool isSingleToken(char character);
	bool isSpace(char character);
public:
	Scanner();
	~Scanner();
	bool startScanner(string filename, bool debug_input);
	Token getToken();
	void printToken();
	Token* token;
};

#endif