#ifndef SCOPE_H
#define SCOPE_H

#include <string>
#include <map>
#include "tokentypes.h"
#include "scopeInfo.h"

using namespace std;

// The 'scope' class implements symbol tables to add and check variable/function declarations and calls
class scope
{
private:
	/* Maps of the key value pairs <identifier, identifier variables>
	 * localTable is checks symbols in the current scope, globalTable checks scopes further down
	 * globalTable contains all of the global variable/function declarations
	 * localTable contains all declarations in the current scope (including duplicates of everything in globalTable)
	 */
	map<string, scopeInfo > globalTable;
	map<string, scopeInfo > localTable;
	string name;

public:
	scope(bool programScope = false);
	~scope();
	int totalBytes;

	//pointer to parent scope one level up
	scope* prevScope;

	//print, purely for debugging purposes
	void printScope();

	//used as a label for the scope table. Will be useful for code generation.
	void setName(string id);

	//symbol table management
	bool addSymbol(string identifier, bool global, scopeInfo value);
	bool checkSymbol(string identifier, bool global);
	scopeInfo getSymbol(string identifier);
};

#endif