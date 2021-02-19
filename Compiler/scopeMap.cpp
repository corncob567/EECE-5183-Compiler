#include "scope.h"
#include "scopeInfo.h"
#include "scopeMap.h"
#include "tokentypes.h"
#include <iostream>

scopeMap::scopeMap(bool debug_input) {
	debug = debug_input;
	tmpPtr = nullptr;
	curPtr = nullptr;
	outermost = nullptr;
}

scopeMap::~scopeMap() {

}

void scopeMap::newScope() {
	if (curPtr != nullptr) {
		//Add procedure scope
		tmpPtr = curPtr;
		curPtr = new scope(false);
		curPtr->prevScope = tmpPtr;
	}
	else {
		//Add program scope
		curPtr = new scope(true);
		curPtr->prevScope = tmpPtr;
		outermost = curPtr;
	}
}

void scopeMap::exitScope() {
	if (curPtr != nullptr) {
		if (debug) curPtr->printScope();
		tmpPtr = curPtr;
		curPtr = curPtr->prevScope;
		delete tmpPtr;
	}
	else if (debug) cout << "not in a scope!" << endl;
	return;
}

bool scopeMap::addSymbol(string identifier, scopeInfo value, bool global) {
	if (curPtr != nullptr) {
		if (!curPtr->checkSymbol(identifier, false)) {
			curPtr->addSymbol(identifier, global, value);
			return true;
		}
		else return false;
	}
	else return false;
}

bool scopeMap::prevAddSymbol(string identifier, scopeInfo value, bool global) {
	scope* prevPtr = curPtr->prevScope;
	if (prevPtr != nullptr) {
		if (!prevPtr->checkSymbol(identifier, false)) {
			prevPtr->addSymbol(identifier, global, value);
			return true;
		}
		else return false;
	}
	else return false;
}

//returns true if symbol exists and puts its table entry into &value
bool scopeMap::checkSymbol(string identifier, scopeInfo& value, bool& global) {
	// Ensure there is actuall a scope to check
	if (curPtr == nullptr) return false;

	// Check local symbols of current scope
	bool found = curPtr->checkSymbol(identifier, false);
	if (found) {
		global = false;
		value = curPtr->getSymbol(identifier);
		return true;
	}
	else {
		found = outermost->checkSymbol(identifier, true);
		if (found) {
			global = true;
			value = outermost->getSymbol(identifier);
			return true;
		}
		else return false;
	}
	return false;
}

// Return the size in bytes of the current symbol table. This will give the call frame size needed to place the table's parent procedure with parameters and local variables.
int scopeMap::getFrameSize() {
	return curPtr->totalBytes;
}

// Set scope name - useful for debugging
void scopeMap::ChangeScopeName(string name) {
	curPtr->setName(name);
	return;
}