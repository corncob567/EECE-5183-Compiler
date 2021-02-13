#ifndef SCOPEMAP_H
#define SCOPEMAP_H

#include "scope.h"
#include "scopeInfo.h"
#include "tokentypes.h"

/*
 * Interface for managing nested scope tables. Uses the 'scope' class to implement all functionality.
 * Will add and check symbols to the nested scope tables.
 */
class scopeMap
{
private:
	scope* curPtr;
	scope* tmpPtr;
	scope* outermost;
	bool debug;
public:
	scopeMap(bool debug_input);
	~scopeMap();
	void newScope();
	void exitScope();
	bool addSymbol(string identifier, scopeInfo value, bool global);
	//identical to addSymbol, but for one scope level up. Used to add procedure declaration to its parent scope and own scope
	bool prevAddSymbol(string identifier, scopeInfo value, bool global);
	bool checkSymbol(string identifier, scopeInfo& value, bool& global);
	void ChangeScopeName(string name);
	int getFrameSize();
};

#endif