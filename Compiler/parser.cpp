#include "parser.h"
#include "scopeMap.h"
#include "scanner.h"
#include "tokentypes.h"
#include "scopeInfo.h"
#include "token.h"
#include <string>
#include <iostream>
#include <queue>

using namespace std;

/* Constructor for the parser
 * Grabs the linked list of tokens from the scanner
 * Starts parsing using the Program() grammar
 * Each function has a comment showing the BNF grammar
 * curly braces '{' and '}' denote optional grammar elements
 * 	these optional elements may be singular { }
 * 	zero or more { }*
 * 	or one or more { }+
 */
Parser::Parser(Token* tokenPtr, Scanner* scannerPtr, scopeMap* prgScopes) {
	// Set default values needed to begin parsing and attach other compiler classes to their pointers
	token = tokenPtr;
	scopes = prgScopes;
	scanner = scannerPtr;
	textLine = "";
	currentLine = 0;
	hasWarning = false;
	hasError = false;
	hasLineError = false;
	outArg = false;

	// Start program parsing
	Program();

	// Ensure the end of the file is reached
	if (token->type != T_EOF) {
		ReportWarning("Tokens remaining. Parsing reached 'end program' and won't process any tokens after it.");
	}

	// Display all errors / warnings
	DisplayErrorQueue();

	if (hasError) {
		cout << "\nParser completed with some errors.\n\tCode cannot be generated.\n" << endl;
	}
	else if (hasWarning) {
		cout << "\nParser completed with some warnings.\n\tCode can still be generated.\n" << endl;
	}
	else {
		cout << "\nParser completed with no errors or warnings.\n\tCode has been generated.\n" << endl;
	}

}

// Destructor
Parser::~Parser() {
	// Set all pointers to nullptr
	token = nullptr;
	scanner = nullptr;
	scopes = nullptr;
}

// Report fatal error and stop parsing.
void Parser::ReportFatalError(string message) {
	error_queue.push("Fatal Error: line - " + to_string(currentLine) + "\n\t" + message + "\n\tFound: " + textLine + " " + token->ascii);
	hasError = true;
	DisplayErrorQueue();
	exit(EXIT_FAILURE);
}

// Report error, line number, and descriptive message. Get tokens until the next line or a ';' is found.
void Parser::ReportLineError(string message, bool skipSemicolon = true) {
	hasError = true;
	hasLineError = true;

	// Use a '^' under the error text line to indicate where the error was encountered
	int error_location = textLine.size();
	string error_callout = "";
	while (error_location > 0) {
		error_location--;
		error_callout.append(" ");
	}
	error_callout.append("^");

	// Get the rest of the line of tokens ( looks for newline or a semicolon )
	bool getNext = true;
	while (getNext) {
		textLine.append(" " + token->ascii);
		// attempt to resync for structure keywords
		if (skipSemicolon) {
			if (token->type == T_SEMICOLON) {
				*token = scanner->getToken();
				getNext = false;
			}
		}
		switch (token->type) {
		case T_SEMICOLON: case T_BEGIN: case T_END: case T_PROCEDURE: case T_THEN: case T_ELSE: case T_FOR:
			getNext = false;
			break;
		default:
			break;
		}
		if (token->line != currentLine) getNext = false;
		if (getNext) *token = scanner->getToken();
	}
	error_queue.push("Line Error: line - " + to_string(currentLine) + "\n\t" + message + "\n\tFound: " + textLine + "\n\t       " + error_callout);
	textLine = "";
	currentLine = token->line;
	return;
}

// Report error line number and descriptive message
void Parser::ReportError(string message) {
	error_queue.push("Error: line - " + to_string(currentLine) + "\n\t" + message + "\n\tFound: " + textLine + " " + token->ascii);
	hasError = true;
	return;
}

// Report warning and descriptive message
void Parser::ReportWarning(string message) {
	error_queue.push("Warning: line - " + to_string(currentLine) + "\n\t" + message + "\n\tFound: " + textLine + " " + token->ascii);
	hasWarning = true;
	return;
}

// Display all of the stored warnings/errors after parsing is complete or a fatal error occurs
void Parser::DisplayErrorQueue() {
	if (!error_queue.empty())
		cout << "\nWarnings / Errors:\n" << endl;
	else return;

	while (!error_queue.empty()) {
		cout << error_queue.front() << "\n" << endl;
		error_queue.pop();
	}
	return;
}

// Check if current token is the correct type, if so get next
bool Parser::CheckToken(int type) {
	// Skip over any comment tokens
	while (token->type == T_COMMENT) {
		*token = scanner->getToken();
	}

	if (token->line != currentLine) {
		currentLine = token->line;
		textLine = "";
	}
	// Make sure current token matches input type, if so move to next token
	if (token->type == type) {
		textLine.append(" " + token->ascii);
		*token = scanner->getToken();
		return true;
	}
	else if (token->type == T_UNKNOWN) {
		ReportError("Found unknown token " + token->ascii);
		*token = scanner->getToken();
		return CheckToken(type);
	}
	else if (token->type == T_EOF) {
		return false;
	}
	else return false;
}

void Parser::DeclareRunTime() {
	// Procedure to be added to the symbol tables
	scopeInfo procVal;
	procVal.size = 0;
	procVal.type = TYPE_PROCEDURE;
	procVal.parameterType = TYPE_PARAM_NULL;

	// input / output parameter of the runtime procedure
	scopeInfo inputVal;
	inputVal.arguments.clear();

	string IDs[10] = { "GETBOOL", "GETINTEGER", "GETFLOAT", "GETSTRING", "GETCHAR", "PUTBOOL", "PUTINTEGER", "PUTFLOAT", "PUTSTRING", "PUTCHAR" };
	int parameterTypes[10] = { TYPE_PARAM_OUT, TYPE_PARAM_OUT, TYPE_PARAM_OUT, TYPE_PARAM_OUT, TYPE_PARAM_OUT, TYPE_PARAM_IN, TYPE_PARAM_IN, TYPE_PARAM_IN, TYPE_PARAM_IN, TYPE_PARAM_IN };
	int Types[10] = { TYPE_BOOL, TYPE_INTEGER, TYPE_FLOAT, TYPE_STRING, TYPE_CHAR, TYPE_BOOL, TYPE_INTEGER, TYPE_FLOAT, TYPE_STRING, TYPE_CHAR };

	for (int i = 0; i < 10; i++) {
		// Clear parameter list
		procVal.arguments.clear();

		inputVal.size = 0;

		// Get new parameter values
		inputVal.type = Types[i];
		inputVal.parameterType = parameterTypes[i];

		// Start new parameter list
		procVal.arguments.push_back(inputVal);

		// Add procedure as a global symbol to the outermost scope
		string symbolID = IDs[i];
		procVal.callLabel = IDs[i];
		scopes->addSymbol(symbolID, procVal, true);
	}
	return;
}

// <program> ::= <program_header> <program_body>
void Parser::Program() {
	scopes->newScope(); // Create a new scope for the program
	DeclareRunTime(); // Set up runtime functions as global in the outermost scope
	if (!ProgramHeader()) ReportError("Expected program header");
	if (!ProgramBody()) ReportError("Expected program body");
	if (!CheckToken(T_PERIOD)) ReportWarning("expected '.' at end of program");
	if (CheckToken(T_EOF)) scopes->exitScope(); // Exit program scope once program ends

	else ReportError("Found some tokens remaining in file when end of program was expected");
	return;
}

//<program_header> ::= program <identifier> is
bool Parser::ProgramHeader() {
	if (CheckToken(T_PROGRAM)) {
		string id;
		if (Identifier(id)) {
			scopes->ChangeScopeName("Program " + id);
			if (CheckToken(T_IS)) return true;
			else {
				ReportError("expected 'is' after program identifier");
				return false;
			}
		}
		else {
			ReportError("expected program identifier after 'program'");
			return false;
		}
	}
	else return false;
}

/* <program_body> ::=
 *	{ <declaration> ;}*
 *	begin
 *	{ <statement> ;}*
 *	end program
 */
bool Parser::ProgramBody() {
	bool resyncEnabled = true;
	bool procDec = false;
	// Get Procedure and Variable Declarations
	while (true) {
		while (Declaration(procDec)) {
			if (procDec) {
				if (!CheckToken(T_SEMICOLON)) ReportWarning("expected ';' after procedure declaration in procedure");
			}
			else if (!CheckToken(T_SEMICOLON)) ReportLineError("expected ';' after variable declaration in procedure", true);
		}
		if (CheckToken(T_BEGIN)) {
			// Reset resync for statements
			resyncEnabled = false;
			while (true) {
				// Get all valid statements
				while (Statement()) {
					if (!CheckToken(T_SEMICOLON)) ReportLineError("Expected ';' at end of statement in program body.", true);
				}
				// Get program body's end
				if (CheckToken(T_END)) {
					if (CheckToken(T_PROGRAM)) return true;
					else ReportError("Expected 'end program' to close program execution.");
				}
				// Use up resync attempt if parser can't find a statement or 'end'
				else if (resyncEnabled) {
					resyncEnabled = false;
					ReportLineError("Bad line. Expected Statement or 'END' reserved keyword in program body.");
				}
				// If resync failed, report a fatal error
				else ReportFatalError("Parser resync failed. Could not find another valid statement or end of program.");
			}
		}
		// Use up resync attempt if parser can't find a declaration or 'begin'
		else if (resyncEnabled) {
			resyncEnabled = false;
			ReportLineError("Bad line. Expected Declaration or 'BEGIN' reserved keyword in program body.");
		}
		// If resync failed, report a fatal error
		else {
			ReportFatalError("Parser resync failed. Could not find another valid declaration or start of program execution.");
		}
	}
}

/*	<declaration> ::=
 *		 {global} <procedure_declaration>
 *		|{global} <variable_declaration>
 */
bool Parser::Declaration(bool& procDec) {
	bool global;
	string id;
	scopeInfo newSymbol;

	/* Ensure that the arguments member and parameterType are both cleared before starting.
	 * None of these cases will set parameterType, that will be set later
	 * ProcedureDeclaration will set the arguments based on the procedure's parameters. */
	newSymbol.arguments.clear();
	newSymbol.parameterType = TYPE_PARAM_NULL;

	// Determine if symbol declaration is global in scope
	if (CheckToken(T_GLOBAL)) global = true;
	else global = false;

	// Determine if a procedure or variable declaration exists
	if (ProcedureDeclaration(id, newSymbol, global)) {
		scopes->exitScope();
		scopes->addSymbol(id, newSymbol, global);
		procDec = true;
		return true;
	}
	else if (VariableDeclaration(id, newSymbol)) {
		// Add symbol to current scope. VariableDeclaration will pass the symbol's type and size members.
		scopes->addSymbol(id, newSymbol, global);
		return true;
	}
	else if (global) {
		ReportLineError("Bad line. Expected either a valid procedure or variable declaration after 'global' keyword");
		return false;
	}
	else return false;
}

// <variable_declarartion> ::= <type_mark><identifier>{ [<array_size>] }
bool Parser::VariableDeclaration(string& id, scopeInfo& varEntry) {
	// Get variable type, otherwise no variable should be declared
	if (!TypeMark(varEntry.type)) return false;
	else {
		// Get variable identifier
		if (Identifier(id)) {
			// Get size for array variable declarations
			if (CheckToken(T_LBRACKET)) {
				if (CheckToken(TYPE_INTEGER)) {
					varEntry.size = token->val.intValue;
					if (!CheckToken(T_RBRACKET)) ReportError("expected ']' at end of array variable declaration.");
					return true;
				}
				else {
					ReportLineError("Expected integer for array size in variable declaration.");
					return true;
				}
			}
			else {
				varEntry.size = 0;
				return true;
			}
		}
		else {
			ReportLineError("Bad line. Expected variable identifier for declaration after type mark.");
			return true;
		}
	}
}

/*	<type_mark> ::=
 *		 integer
 *		|float
 *		|bool
 *		|string
 *		|char
 */
bool Parser::TypeMark(int& type) {
	if (CheckToken(T_INTEGER)) type = TYPE_INTEGER;
	else if (CheckToken(T_FLOAT)) type = TYPE_FLOAT;
	else if (CheckToken(T_BOOL)) type = TYPE_BOOL;
	else if (CheckToken(T_STRING)) type = TYPE_STRING;
	else if (CheckToken(T_CHAR)) type = TYPE_CHAR;
	else return false;
	return true;
}

//<procedure_declaration> ::= <procedure_header><procedure_body>
bool Parser::ProcedureDeclaration(string& id, scopeInfo& procDeclaration, bool global) {
	//Get Procedure Header
	if (ProcedureHeader(id, procDeclaration, global)) {
		if (ProcedureBody()) return true;
		else {
			ReportFatalError("Expected procedure body after procedure header.");
			return true;
		}
	}
	else return false;
}

// <procedure_header> ::= procedure <identifier> ( { <parameter_list> } )
bool Parser::ProcedureHeader(string& id, scopeInfo& procDeclaration, bool global) {
	if (CheckToken(T_PROCEDURE)) {
		//Create new scope in nested symbol tables for the procedure
		scopes->newScope();

		// Set the symbol table entry's type and size to the correct values for a procedure
		procDeclaration.type = TYPE_PROCEDURE;
		procDeclaration.size = 0;

		// Get procedure identifier and set value to be added to the symbol table
		if (Identifier(id)) {
			scopes->ChangeScopeName(id);

			// Get parameter list for the procedure, if it has parameters
			if (CheckToken(T_LPAREN)) {
				ParameterList(procDeclaration);
				if (!CheckToken(T_RPAREN)) ReportLineError("Bad Line. Expected ')' after parameter list in procedure header");
				scopes->addSymbol(id, procDeclaration, global);
				return true;

			}
			else {
				ReportFatalError("Expected '(' in procedure header before parameter list.");
				return true;
			}
		}
		else {
			ReportFatalError("Expected procedure identifier in procedure header.");
			return true;
		}
	}
	else return false;
}

/*	<procedure_body> ::=
 *		{ <declaration> ; }*
 *		begin
 *		{ <statement> ; }*
 *		end procedure
 */
bool Parser::ProcedureBody() {
	bool resyncEnabled = true;
	bool procDec = false;
	// Get symbol declarations for next procedure
	while (true) {
		while (Declaration(procDec)) {
			if (procDec) {
				if (!CheckToken(T_SEMICOLON)) ReportWarning("expected ';' after procedure declaration in procedure");
			}
			else if (!CheckToken(T_SEMICOLON)) ReportLineError("expected ';' after variable declaration in procedure", true);
		}

		// GEN: set the procedure's FP and SP in the program body after all declarations have been made
		// Each procedure declaration added its bytes to the frame size so this is the max size
		int frameSize = scopes->getFrameSize();

		// Get statements for procedure body
		if (CheckToken(T_BEGIN)) {
			resyncEnabled = true;
			while (true) {
				while (Statement()) {
					if (!CheckToken(T_SEMICOLON)) ReportLineError("expected ';' after statement in procedure", true);
				}
				if (CheckToken(T_END)) {
					if (CheckToken(T_PROCEDURE)) return true;
					else {
						ReportError("expected 'end procedure' at end of procedure declaration");
						return true;
					}
				}
				else if (resyncEnabled) {
					resyncEnabled = false;
					ReportLineError("Bad line. Expected Statement or 'end' reserved keyword in procedure body.");
				}
				else {
					ReportFatalError("expected 'end procedure' at end of procedure declaration");
					return false;
				}
			}
		}
		else if (resyncEnabled) {
			resyncEnabled = false;
			ReportLineError("Bad line. Expected Declaration or 'begin' reserved keyword in procedure body.");
		}
		else {
			ReportFatalError("Parser resync failed. Couldn't find a valid declaration or the 'begin reserved keyword in procedure body.");
			return false;
		}

	}

}

/* <procedure_call> ::= <identifier>( { <argument_list> } )
 * Note: the identifier is found in the previously called assignment statement and will have its value passed to the ProcedureCall. */
bool Parser::ProcedureCall(string id) {
	// Argument list whose type and size values will be compared against those declared in the procedure's parameter list
	vector<scopeInfo> argList;
	scopeInfo procedureCall;
	bool isGlobal;
	int offset = 2;
	bool found;

	// Ensure an id was found in the assignment statement check called right before ProcedureCall, otherwise return false
	if (id == "") return false;

	// Get procedure's declared information from scope table
	found = scopes->checkSymbol(id, procedureCall, isGlobal);

	// Get argument list used in the procedure call
	if (CheckToken(T_LPAREN)) {
		ArgumentList(argList, procedureCall, offset);
		if (!CheckToken(T_RPAREN)) ReportLineError("Expected ')' closing procedure call");
	}
	else ReportError("expected '(' in procedure call");

	// Compare called argument list against the declared parameter list
	if (found) {
		bool match = true;

		// Iterators to go through the parameter and argument lists
		vector<scopeInfo>::iterator it1 = argList.begin();
		vector<scopeInfo>::iterator it2 = procedureCall.arguments.begin();

		while ((it1 != argList.end()) && (it2 != procedureCall.arguments.end())) {
			if ((it1->type != it2->type) || (it1->size != it2->size)) match = false;
			++it1;
			++it2;
		}
		if ((it1 != argList.end()) || (it2 != procedureCall.arguments.end()) || (!match)) {
			ReportError("Procedure call argument list does not match declared parameter list.");
		}

		return true;
	}
	else {
		ReportError("Procedure: " + id + " was not declared in this scope.");
		return true;
	}
}

/*	<argument_list> ::=
 *	 <expression> , <argument_list>
 *	|<expression>
 */
bool Parser::ArgumentList(vector<scopeInfo>& list, scopeInfo procValue, int& offset) {
	// Create vector<scopeInfo> where all argument list information will be stored and later returned by the function
	list.clear();

	/* argEntry variable will contain argument information about expression type and size.
	 * All arguments will have an empty arguments vector and have their parameterType = TYPE_PARAM_NULL
	 * The procedure call, when checking these arguments, will not compare the parameterType value against the declared values */
	scopeInfo argEntry;
	argEntry.arguments.clear();
	argEntry.parameterType = TYPE_PARAM_NULL;

	// For each comma-separated expression, store the type and size values, in the order encountered, in the 'list' vector
	vector<scopeInfo>::iterator it = procValue.arguments.begin();
	if (it != procValue.arguments.end()) {
		if (it->parameterType == TYPE_PARAM_OUT) outArg = true;
	}

	if (Expression(argEntry.type, argEntry.size)) {
		// GEN: add arguments from register to correct frame

		offset = 2;
		++it;

		list.push_back(argEntry);
		while (CheckToken(T_COMMA)) {
			if (it != procValue.arguments.end()) {
				if (it->parameterType == TYPE_PARAM_OUT) {
					outArg = true;
				}
				else outArg = false;
			}
			if (Expression(argEntry.type, argEntry.size)) {
				list.push_back(argEntry);
				// Add arguments from register to correct frame
				++it;
			}
			else ReportError("expected another argument after ',' in argument list of procedure call");
		}
	}
	outArg = false;
	return true;
}


/*	<parameter_list> ::=
 *		 <parameter> , <parameter_list>
 *		|<parameter>
 */
bool Parser::ParameterList(scopeInfo& procEntry) {
	if (Parameter(procEntry)) {
		while (CheckToken(T_COMMA)) {
			if (!Parameter(procEntry)) ReportError("Expected parameter after ',' in procedure's parameter list");
		}
	}
	return true;
}

// <parameter> ::= <variable_declaration>
bool Parser::Parameter(scopeInfo& procEntry) {
	scopeInfo paramEntry;
	string id;
	// Get parameter declaration
	if (VariableDeclaration(id, paramEntry)) {
		// Add parameter to current scope
		scopes->addSymbol(id, paramEntry, false);

		// Add to current procedure declaration's parameter list
		procEntry.arguments.push_back(paramEntry);

		return true;
	}
	else return false;
}

/*	<statement> ::=
 *		 <assignment_statement>
 *		|<if_statement>
 *		|<loop_statement>
 *		|<return_statement>
 *		|<procedure_call>
 */
bool Parser::Statement() {
	string id = "";
	if (IfStatement()) return true;
	else if (LoopStatement()) return true;
	else if (ReturnStatement()) return true;
	else if (Assignment(id)) return true;
	else if (ProcedureCall(id)) return true;
	else return false;
}

// <assignment_statement> ::= <destination> := <expression>
bool Parser::Assignment(string& id) {
	int type, size, dType, dSize;
	bool found;
	scopeInfo destinationValue;
	bool isGlobal;
	bool indirect;
	int indirect_type;

	// Determine destination if this is a valid assignment statement
	if (!Destination(id, dType, dSize, destinationValue, found, isGlobal, indirect, indirect_type)) {
		return false;
	}

	// Get assignment expression
	if (CheckToken(T_ASSIGNMENT)) {
		Expression(type, size);

		if (found) {
			if (size != dSize && (size > 1) && (dSize <= 1)) {
				ReportError("Bad assignment, size of expression must match destination's size.");
			}
			if ((type != dType) && ((!isNumber(dType)) || (!isNumber(type)))) {
				ReportError("Bad assignment, type of expression must match destination.");
			}
		}
		return true;
	}
	else {
		ReportLineError("Bad line. Expected ':=' after destination in assignment statement.", false);
		return true;
	}
}

/* <destination> ::= <identifier> { [<expression] }
 * Returns the destination's identifier (will be used in procedure call if assignment fails).
 * Returns destination's type and size for comparison with what is being assigned to the destination.
 */
bool Parser::Destination(string& id, int& dType, int& dSize, scopeInfo& destinationValue, bool& found, bool& isGlobal, bool& indirect, int& indirect_type) {
	// Variable to hold destination symbol's information from the nested scope tables
	int type, size;

	if (Identifier(id)) {
		found = scopes->checkSymbol(id, destinationValue, isGlobal);

		/* If a procedure is found, return false.
		   This can't be a destination and the found id will be passed to a procedure call */
		if ((found) && (destinationValue.type == TYPE_PROCEDURE)) return false;
		else if (!found) {
			ReportError("Destination: " + id + " was not declared in this scope");
			dType = T_UNKNOWN;
			dSize = 0;
		}
		else {
			dType = destinationValue.type;
			dSize = destinationValue.size;
		}

		if (CheckToken(T_LBRACKET)) {
			indirect = true;
			if (Expression(type, size)) {
				// Ensure array index is a single numeric value
				indirect_type = type;
				if (size != 0 || ((type != TYPE_FLOAT) && (type != TYPE_INTEGER) && (type != TYPE_BOOL))) {
					ReportError("Destination array's index must be a scalar numeric value");
				}
				else dSize = 0;

				if (CheckToken(T_RBRACKET)) {
					return true;
				}
				else {
					ReportLineError("expected ']' after destination array's index");
					return true;
				}
			}
			else {
				ReportLineError("Bad Line. Expected scalar numeric expression in array index.");
				return true;
			}
		}
		else {
			indirect = false;
			return true;
		}
	}
	else return false;
}

/*	<if_statement> ::=
 *		if ( <expression> ) then { <statement> ; }+
 *		{ else { <statement> ; }+ }
 *		end if
 */
bool Parser::IfStatement() {
	/* Flag to determine if at least one statement occured after 'then' and 'else'
	 * Type and size variables are unused but needed to perfrom the call to Expression(type, size) */
	bool flag;
	int type, size;
	string labelTrue, labelFalse, labelEnd;
	bool resyncEnabled = true;

	// Determine if this is the start of an if statement
	if (!CheckToken(T_IF)) {
		return false;
	}

	// Get expression for conditional statement: '( <expression> )'
	if (!CheckToken(T_LPAREN)) {
		ReportLineError("Expected '(' before condition in if statement.");
	}
	else if (!Expression(type, size)) {
		ReportLineError("Expected condition for if statement.");
	}
	else if (type != TYPE_BOOL) {
		ReportLineError("Conditional expression in if statement must evaluate to type bool.");
	}
	else if (!CheckToken(T_RPAREN)) {
		ReportLineError("Expected ')' after condition in if statement.");
	}

	/* Get statements to be evaluated if the statement's expression evaluates to true.
	 * There must be at least one statement following 'then'. */
	if (CheckToken(T_THEN)) {
		flag = false;
		while (true) {
			while (Statement()) {
				flag = true;
				if (!CheckToken(T_SEMICOLON)) {
					ReportLineError("expected ';' after statement in conditional statement's 'if' condition", true);
				}
			}
			if (!flag) {
				ReportError("expected at least one statement after 'then' in conditional statement");
			}

			if (CheckToken(T_ELSE)) {
				flag = false;
				resyncEnabled = true;
				while (true) {
					while (Statement()) {
						flag = true;
						if (!CheckToken(T_SEMICOLON)) {
							ReportLineError("Expected ';' after statement in conditional statement's 'else' condition.", true);
						}
					}
					/* Check for correct closure of statement: 'end if' */

					if (CheckToken(T_END)) {
						if (!flag) {
							ReportError("expected at least one statement after 'else' in conditional statement.");
						}
						if (!CheckToken(T_IF)) {
							ReportFatalError("missing 'if' in the 'end if' closure of conditional statement");
						}
						return true;
					}
					else if (resyncEnabled) {
						resyncEnabled = false;
						ReportLineError("Bad Line. Unable to find valid statement or 'else' or 'end' reserved keywords.");
					}
					else {
						ReportFatalError("Parser resync failed. Unable to find valid statement, 'else' or 'end if' reserved keywords.");
						return true;
					}
				}
			}
			/* Check for correct closure of statement: 'end if' */
			else if (CheckToken(T_END)) {
				if (!CheckToken(T_IF)) {
					ReportFatalError("Missing 'if' in the 'end if' closure of the if statement.");
				}
				return true;
			}
			else if (resyncEnabled) {
				resyncEnabled = false;
				ReportLineError("Bad Line. Unable to find valid statement or 'else' or 'end' reserved keywords.");
			}
			else ReportFatalError("Parser resync failed. Unable to find valid statement, 'else' or 'end if' reserved keywords.");
		}
	}
	else ReportFatalError("Expected 'then' after condition in if statement.");
}

/*	<loop_statement> ::=
 *		for ( <assignment_statement> ; <expression> )
 *		{ <statement> ; }*
 *		end for
 */
bool Parser::LoopStatement() {
	int type, size;
	string id;
	bool resyncEnabled = true;

	// Determine if a loop statement is going to be declared
	if (!CheckToken(T_FOR)) return false;

	/* Get assignment statement and expression for loop: '( <assignment_statement> ; <expression> )'
	 * Throws errors if '(' or ')' is missing. Throws warnings for other missing components */
	if (!CheckToken(T_LPAREN)) ReportFatalError("Expected '(' before assignment and expression in for loop statement.");

	if (!Assignment(id)) ReportError("Expected an assignment at start of for loop statement.");

	if (!CheckToken(T_SEMICOLON)) ReportError("Expected ';' separating assignment statement and expression in for loop statement.");

	if (!Expression(type, size)) ReportError("Expected a valid expression following assignment in for loop statement.");

	if (!CheckToken(T_RPAREN)) ReportError("Expected ')' after assignment and expression in for loop statement.");

	while (true) {
		while (Statement()) {
			if (!CheckToken(T_SEMICOLON)) ReportLineError("Expected ';' after statement in for loop.", true);
		}

		if (CheckToken(T_END)) {
			if (!CheckToken(T_FOR)) ReportError("Missing 'for' in the 'end for' closure of the for loop statement.");
			return true;
		}
		else if (resyncEnabled) {
			resyncEnabled = false;
			ReportLineError("Bad line. Could not find a valid statement or 'end' reserved keyword in loop statement.");
		}
		else {
			ReportFatalError("Expected 'end for' at end of for loop statement.");
			return false;
		}
	}
}

// <return_statement> ::= return
bool Parser::ReturnStatement() {
	if (CheckToken(T_RETURN)) {
		return true;
	}
	else return false;
}

/*	<expression> ::=
 *		{ not } <arithOp> <expression'>
 */
bool Parser::Expression(int& type, int& size) {
	// Flag used to determine if an expression is required following a 'NOT' token
	bool notOperation;
	if (CheckToken(T_NOT)) notOperation = true;
	else notOperation = false;

	// Get type and size of first arithOp
	if (ArithOp(type, size)) {
		if ((notOperation) && (type != TYPE_BOOL) && (type != TYPE_INTEGER))ReportError("'NOT' operator is defined only for type Bool and Integer.");
		ExpressionPrime(type, size, true, true);
		return true;
	}
	else if (notOperation) {
		ReportFatalError("Expected an integer / boolean ArithOp following 'NOT'.");
		return true;
	}
	else {
		return false;
	}
}

/*  <expression'> ::=
 *  | 	& <arithOp> <expression'>
 *  | 	| <arithOp> <expression'>
 *  |	null
 */
bool Parser::ExpressionPrime(int& inputType, int& inputSize, bool catchTypeError, bool catchSizeError) {
	int arithOpType, arithOpSize;

	// Get the operator to pass to the codegenerator evaluation stack
	string op = token->ascii;

	if (CheckToken(T_BITWISE)) {
		CheckToken(T_NOT); // 'NOT' is always optional and will be good for both integer-bitwise and boolean-boolean expressions.
		if (ArithOp(arithOpType, arithOpSize)) {
			if (catchTypeError) {
				if (inputType == TYPE_INTEGER) {
					if (arithOpType != TYPE_INTEGER) {
						ReportError("Only integer ArithOps can be used for bitwise operators '&' and '|'.");
						catchTypeError = false;
					}
				}
				else if (inputType == TYPE_BOOL) {
					if (arithOpType != TYPE_BOOL) {
						ReportError("Only boolean ArithOps can be used for boolean operators '&' and '|'.");
						catchTypeError = false;
					}
				}
				else ReportError("Only integer / boolean ArithOps can be used for bitwise / boolean operators '&' and '|'.");
			}
			if (catchSizeError) {
				// Ensure compatible sizes are used.
				if ((inputSize != arithOpSize) && (inputSize != 0) && (arithOpSize != 0)) {
					ReportError("Expected ArithOp of size " + to_string(inputSize) + ", but found one of size " + to_string(arithOpSize) + ".");
					catchSizeError = false;
				}
				// Set new inputSize if the next ArithOp's size was non-zero and inputSize was zero.
				else if (arithOpSize != 0) inputSize = arithOpSize;
			}
		}
		else {
			ReportError("Expected ArithOp after '&' or '|' operator.");
			catchTypeError = false;
			catchSizeError = false;
		}
		ExpressionPrime(inputType, inputSize, catchTypeError, catchSizeError);

		return true;
	}
	else return false;
}

/*	<arithOp> ::=
 *		<relation> <arithOp'>
 */
bool Parser::ArithOp(int& type, int& size) {
	if (Relation(type, size)) {
		ArithOpPrime(type, size, true, true);
		return true;
	}
	else return false;
}

/*  <ArithOp'> ::=
 *		|	+ <relation> <arithOp'>
 *		|	- <relation> <arithOp'>
 *		|	null
 */
bool Parser::ArithOpPrime(int& inputType, int& inputSize, bool catchTypeError, bool catchSizeError) {
	// Size and type of next Relation in the ArithOp sequence
	int relationType, relationSize;

	// Get operation to pass to the codegenerator
	string op = token->ascii;

	// If '+' or '-' can't be found then return false ('null'). Otherwise continue function.
	if (CheckToken(T_ADD));
	else if (CheckToken(T_SUBTRACT));
	else return false;

	// Get next Relation. Otherwise report Missing Relation error.
	if (Relation(relationType, relationSize)) {
		// Only allow number (integer / float) relations in arithmetic operations.
		if (catchTypeError) {
			if (!isNumber(relationType) || !isNumber(inputType)) {
				ReportError("Only integer and float values are allowed for arithmetic operations.");
				catchTypeError = false;
			}
		}
		// Ensure compatible sizes are used in ArithOp.
		if (catchSizeError) {
			// Error if sizes are not identical and both are non-zero
			if ((inputSize != relationSize) && (inputSize != 0) && (relationSize != 0)) {
				ReportError("Expected Relation of size " + to_string(inputSize) + ", but found one of size " + to_string(relationSize) + ".");
				catchSizeError = false;
			}
			// Assign inputSize as exprSize if inputSize is non-zero.
			else if (relationSize != 0) inputSize = relationSize;
		}
	}
	else {
		ReportError("Expected relation after arithmetic operator.");
		catchTypeError = false;
		catchSizeError = false;
	}
	// Continue to look for other +/- <relation> in case there is a missing arithOp or doubled up arithmetic operators
	ArithOpPrime(inputType, inputSize, catchTypeError, catchSizeError);

	return true;
}

/*	<relation> ::=
 *		| <term> <relation'>
 */
bool Parser::Relation(int& type, int& size) {
	if (Term(type, size)) {
		// Get relational operator if one exists. All relational operators return type bool.
		if (RelationPrime(type, size, true, true)) type = TYPE_BOOL;
		return true;
	}
	else return false;
}

/* <relation'> ::=
 *		| <  <term> <relation'>
 *		| <= <term> <relation'>
 *		| >  <term> <relation'>
 *		| >= <term> <relation'>
 *		| == <term> <relation'>
 *		| != <term> <relation'>
 *		| null
 */
bool Parser::RelationPrime(int& inputType, int& inputSize, bool catchTypeError, bool catchSizeError) {
	// Type and size of relation being compared to.
	int termType, termSize;

	// Get relationsal op to pass to the codegenerator evaluation stack
	string op = token->ascii;

	if (CheckToken(T_COMPARE)) {
		// Get next term, otherwise report missing term error.
		if (Term(termType, termSize)) {
			if (catchTypeError) {
				// Ensure both terms are of type integer or bool. The two types can be compared against each other.
				if (((inputType != TYPE_BOOL) && (inputType != TYPE_INTEGER)) || ((termType != TYPE_BOOL) && (termType != TYPE_INTEGER))) {
					ReportError("Relational operators are only valid for terms of type bool or integers '0' and '1'.");
					catchTypeError = false;
				}
			}
			if (catchSizeError) {
				// Ensure compatible sizes are used.
				if ((inputSize != termSize) && ((inputSize != 0) && (termSize != 0))) {
					ReportError("Expected term of size " + to_string(inputSize) + ", but found one of size " + to_string(termSize) + ".");
					catchSizeError = false;
				}
				// Assign termSize to inputSize if termSize is non-zero.
				else if (termSize != 0) inputSize = termSize;
			}
		}
		else {
			ReportError("Expected term after relational operator.");
			catchTypeError = false;
			catchSizeError = false;
		}
		// Check for another relational operator and term.
		RelationPrime(inputType, inputSize, catchTypeError, catchSizeError);

		return true;
	}
	else return false;
}

/*	<term> ::=
 *		<factor> <term'>
 */
bool Parser::Term(int& type, int& size) {
	if (Factor(type, size)) {
		TermPrime(type, size, true, true);
		return true;
	}
	else return false;
}

/* <term'> ::=
 *		| * <factor> <term'>
 *		| / <factor> <term'>
 *		| null
 */
bool Parser::TermPrime(int& inputType, int& inputSize, bool catchTypeError, bool catchSizeError) {
	// Next factor type and size.
	int factorType, factorSize;

	// Get term op to pass to the code generator
	string op = token->ascii;

	// Check for '*' or '/' token, otherwise return false ('null').
	if (CheckToken(T_MULTIPLY));
	else if (CheckToken(T_DIVIDE));
	else return false;

	// Get next factor, otherwise report missing factor error.
	if (Factor(factorType, factorSize)) {
		// Ensure both factors are numbers for arithmetic operators.
		if (!isNumber(inputType) || !isNumber(factorType)) {
			ReportError("Only integer and float factors are defined for arithmetic operations in term.");
			catchTypeError = false;
		}
		if ((inputSize != factorSize) && ((inputSize != 0) && (factorSize != 0))) {
			ReportError("Expected factor of size " + to_string(inputSize) + ", but found one of size " + to_string(factorSize) + ".");
			catchSizeError = false;
		}
		else if (factorSize != 0) inputSize = factorSize;
	}
	else {
		ReportError("Expected factor after arithmetic operator in term.");
		catchTypeError = false;
		catchSizeError = false;
	}
	TermPrime(inputType, inputSize, catchTypeError, catchSizeError);

	return true;
}

/*	<factor> ::=
 *		 ( <expression> )
 *			TODO: Add <procedure_call>
 *		|{-} <name>
 *		|{-} <number>
 *		|<string>
 *		|<char> TODO: Remove char
 *		|true
 *		|false
 */
bool Parser::Factor(int& type, int& size) {
	int tempType, tempSize;
	if (CheckToken(T_LPAREN)) {
		if (Expression(tempType, tempSize)) {
			type = tempType;
			size = tempSize;
			if (CheckToken(T_RPAREN)) {
				return true;
			}
			else ReportFatalError("expected ')' in factor around the expression");
		}
		else ReportFatalError("expected expression within parenthesis of factor");
	}
	else if (CheckToken(T_SUBTRACT)) {
		if (Integer()) {
			type = TYPE_INTEGER;
			size = 0;
			return true;
		}
		else if (Float()) {
			type = TYPE_FLOAT;
			size = 0;
			return true;
		}
		else if (Name(tempType, tempSize)) {
			type = tempType;
			size = tempSize;
			if (!isNumber(type)) ReportError(" negation '-' before variable name is valid only for integers and floats.");
			return true;
		}
		else return false;
	}
	else if (Name(tempType, tempSize)) {
		type = tempType;
		size = tempSize;
		return true;
	}
	else if (Integer()) {
		type = TYPE_INTEGER;
		size = 0;
		return true;
	}
	else if (Float()) {
		type = TYPE_FLOAT;
		size = 0;
		return true;
	}
	else if (String()) {
		type = TYPE_STRING;
		size = 0;
		return true;
	}
	else if (Char()) {
		type = TYPE_CHAR;
		size = 0;
		return true;
	}
	else if (Bool()) {
		type = TYPE_BOOL;
		size = 0;
		return true;
	}
	else {
		return false;
	}
}

// <name> ::= <identifier> { [ <expression> ] }
bool Parser::Name(int& type, int& size) {
	string id;
	scopeInfo nameValue;
	bool isGlobal;
	if (Identifier(id)) {
		bool symbolExists = scopes->checkSymbol(id, nameValue, isGlobal);
		if (symbolExists) {
			if (nameValue.type == TYPE_PROCEDURE) {
				ReportError(id + " is a procedure in this scope, not a variable.");
			}
			else {
				size = nameValue.size;
				type = nameValue.type;
			}
		}
		else {
			ReportError(id + " has not been declared in this scope.");
			size = 0;
			type = T_UNKNOWN;
		}

		if (CheckToken(T_LBRACKET)) {
			if (nameValue.size == 0 && nameValue.type != TYPE_PROCEDURE) ReportError(id + " is not an array.");
			int type2, size2;
			if (Expression(type2, size2)) {
				if ((size2 > 1) || ((type2 != TYPE_INTEGER) && (type2 != TYPE_FLOAT) && (type2 != TYPE_BOOL)))
					ReportError("Array index must be a scalar numeric value.");
				size = 0;
				if (CheckToken(T_RBRACKET)) {
					return true;
				}
				else ReportError("Expected ']' after expression in name.");
			}
			else ReportFatalError("Expected expression between brackets.");
		}
		else {
			return true;
		}
	}
	else return false;
}

// <number> ::= [0-9][0-9]*[.[0-9]*] TODO: Make this include underscores.
bool Parser::Number() {
	if (Integer()) return true;
	else if (Float()) return true;
	else return false;
}

// <integer> ::= [0-9][0-9]* TODO: Make this include underscores.
bool Parser::Integer() {
	string val = token->ascii;
	if (CheckToken(TYPE_INTEGER)) {
		return true;
	}
	else return false;
}

// <float> ::= [0-9][0-9]*[.[0-9]+] TODO: Make this include underscores.
bool Parser::Float() {
	string val = token->ascii;
	if (CheckToken(TYPE_FLOAT)) {
		return true;
	}
	else return false;
}

// <string> ::= "[a-zA-Z0-9_,;:.']*" TODO: Adjust regex
bool Parser::String() {
	string val = token->ascii;
	if (CheckToken(TYPE_STRING)) {
		return true;
	}
	else return false;
}

// <char> ::= '[a-zA-Z0-9_,;:."]' TODO: Remove this type.
bool Parser::Char() {
	string val = token->ascii;
	if (CheckToken(TYPE_CHAR)) {
		return true;
	}
	else return false;
}

bool Parser::Bool() {
	string val = token->ascii;
	if (CheckToken(T_TRUE)) {
		return true;
	}
	else if (CheckToken(T_FALSE)) {
		return true;
	}
	else return false;
}

// <identifier> ::= [a-zA-Z][a-zA-Z0-9_]*
bool Parser::Identifier(string& id) {
	string tmp = token->ascii;
	bool ret_val = CheckToken(TYPE_IDENTIFIER);
	if (ret_val) {
		id = tmp;
		return true;
	}
	else return false;
}

// Check if token is an integer or float.
bool Parser::isNumber(int& type_value) {
	if ((type_value == TYPE_INTEGER) || (type_value == TYPE_FLOAT)) return true;
	else return false;
}