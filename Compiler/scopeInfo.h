#ifndef SCOPEINFO_H
#define SCOPEINFO_H

#include <vector>
#include <string>

using namespace std;

/* Struct to hold information about a symbol in scope table.
 *    type - symbol type (procedure, string, integer, etc.)
 *    size - size of arrays (0 for non-arrays)
 *    arguments - vector of input arguments for functions.
 *    paramType - parameter type IN | OUT | INOUT | NULL
 *    FPoffset - number of bytes offset from Frame Pointer on the stack
 *    callLabel - string of a procedure call label
 */
struct scopeInfo {
	int type;
	int size;
	int returnType;

	// Used solely for function calls
	vector<scopeInfo> arguments;
	string callLabel;

	// Used solely for variables
	int parameterType; // TODO: Do we even need this?
	int FPoffset;

};

#endif