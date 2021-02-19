#ifndef PARSER_H
#define PARSER_H

#include <string>
#include "tokentypes.h"
#include "token.h"
#include "scopeInfo.h"
#include "scanner.h"
#include "scopeMap.h"
#include <queue>

using namespace std;

class Parser
{
private:
	/* Methods, flags, and queue used for warning / error reporting in the Parser
	 * All report functions will enqueue a message along with line number and found tokens for that line.
	 */
	queue<std::string> error_queue;
	void DisplayErrorQueue();
	void ReportFatalError(string message);
	void ReportLineError(string message, bool skipSemicolon);
	void ReportError(string message);
	void ReportWarning(string message);

	// Error status flags and text and line number to report errors with
	bool hasError, hasWarning, hasLineError;
	string textLine;
	int currentLine;

	/* Pointers and method to handle the token stream passed from the Scanner.
	 * CheckToken() determines if the current token type is the input type. The stream moves forward one token.
	 * The two pointers point to the current token in the stream, and the previous token in the stream (useful for getting information)
	 */
	bool CheckToken(int type);
	void DeclareRunTime();
	void Program();
	bool ProgramHeader();
	bool ProgramBody();

	// Declarations
	bool Declaration(bool& procDec);
	bool TypeDeclaration(string& id, scopeInfo& typeEntry);

	// Variables
	bool VariableDeclaration(string& id, scopeInfo& varEntry);
	bool TypeMark(int& type);

	// Procedures
	bool ProcedureDeclaration(string& id, scopeInfo& procDeclaration, bool global);
	bool ProcedureHeader(string& id, scopeInfo& procDeclaration, bool global);
	bool ProcedureBody();
	bool ProcedureCall(string id);

	// Parameters / Arguments for procedure declarations / calls
	bool ParameterList(scopeInfo& procEntry);
	bool Parameter(scopeInfo& procEntry);
	bool ArgumentList(vector<scopeInfo>& list, scopeInfo procValue, int& offset);

	// Statements
	bool Statement();
	bool Assignment(string& id);
	bool Destination(string& id, int& dType, int& dSize, scopeInfo& destinationValue, bool& found, bool& isGlobal, bool& indirect, int& indirect_type);
	bool IfStatement();
	bool LoopStatement();
	bool ReturnStatement();

	// Recursive functions to evaluate statements 
	bool Expression(int& type, int& size);
	bool ExpressionPrime(int& inputType, int& inputSize, bool catchTypeError, bool catchSizeError);

	bool ArithOp(int& type, int& size);
	bool ArithOpPrime(int& inputType, int& inputSize, bool catchTypeError, bool catchSizeError);

	bool Relation(int& type, int& size);
	bool RelationPrime(int& inputType, int& inputSize, bool catchTypeError, bool catchSizeError);

	bool Term(int& type, int& size);
	bool TermPrime(int& inputType, int& inputSize, bool catchTypeError, bool catchSizeError);

	bool Factor(int& type, int& size);
	bool FactorPrime(int& inputType, int& inputSize, bool catchTypeError, bool catchSizeError);

	bool Name(int& type, int& size);

	// Constant Values
	bool Number();
	bool Integer();
	bool Float();
	bool String();
	bool Char();
	bool Bool();
	bool Identifier(string& id);
	bool isNumber(int& type_value);

	bool outArg;
public:
	// Pointers to the other major components of the compiler which are called / checked by the parser
	Token* token;
	Scanner* scanner;
	scopeMap* scopes;
	// Constructor and destructor - begins parsing as soon as it is constructed
	Parser(Token* tokenPtr, Scanner* scannerPtr, scopeMap* scopes);
	~Parser();
};

#endif