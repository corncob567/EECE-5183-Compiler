#ifndef SCANNER_H
#define SCANNER_H

//#include "token.h"
//#include "tokenType.h"
#include <string>
#include <fstream>
#include <unordered_map>

enum CharacterClassMembers { NUM, LC_ALPHA, UC_ALPHA, SPECIAL, WHITESPACE};

class Scanner {
	public:
		Scanner(bool debug);
		~Scanner();
		bool debugMode;
		bool errorFlag;
		bool openFile(std::string fileName);
		Token scan();
	private:
		std::ifstream file;
		std::string fileName;
		int lineNumber;
		std::unordered_map<char, CharacterClassMembers> characterClass;
		int getCharacterClass(char c);
		Token scanToken();
		void error(std::string msg);
		void warning(std::string msg);
		void debug(std::string msg);
		void debug(Token t);
};
#endif