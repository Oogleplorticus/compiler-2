#include "parser.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "byte_array.h"
#include "bytecode_gen.h"
#include "operation.h"
#include "token.h"
#include "tokeniser.h"

static FILE* srcPtr = NULL;

//state variables
static uint32_t nextFunctionID = 0;
static uint32_t nextVariableID = 1;
static size_t scopeDepth = 0;

static uint32_t currentFunctionBlockCount = 0;
static uint64_t currentBlockInstructionCount = 0;

void resetState() {
	nextFunctionID = 0;
	nextVariableID = 1;
	scopeDepth = 0;

	currentFunctionBlockCount = 0;
	currentBlockInstructionCount = 0;
}

//forward declaration
void parse();
uint32_t parseOperand();

void unexpectedToken() {
	fprintf(stderr, "ERROR: Unexpected token of type %d at file index %zu!\n", currentToken().type, currentToken().fileIndex);
	exit(1);
}

uint32_t parseStringLiteral(struct Token literal) {
	char* buffer = calloc(literal.length - 2, sizeof(char)); //may be slightly larger than neccessary
	if (buffer == NULL) {
		fprintf(stderr, "ERROR: Could not allocate memory for string literal temp!\n");
		exit(1);
	}

	//seek to first character in string
	fseek(srcPtr, literal.fileIndex + 1, SEEK_SET);

	size_t trueLiteralLength = 0;
	size_t bufferIndex = 0;

	for (size_t i = 0; i < literal.length - 2; ++i) {
		char c = fgetc(srcPtr);

		//check if escape character
		if (c == '\\') {
			//skip escape character and replace next with relevant char
			++i;
			c = fgetc(srcPtr);
			switch (c) {
				case '\\': break;
				case 'n': c = '\n'; break;

				default:
				fprintf(stderr, "ERROR: Invalid escape character at file index %zu!\n", literal.fileIndex + i + 1);
				exit(1);
			}
		}

		buffer[bufferIndex] = c;

		++bufferIndex;
		++trueLiteralLength;
	}

	uint32_t pointerVariableID = createStaticData(IR_UNSIGNED, 3, trueLiteralLength, buffer);
	insertValue(pointerVariableID, 4);
	insertConstant(IR_UNSIGNED, -1, trueLiteralLength);

	free(buffer);

	return pointerVariableID;
}



//starts on the closing parenthesis or first parameter
void parseFunctionDefinition(struct Token identifier) {
	//cant define a function inside a function
	if (scopeDepth > 0) {
		fprintf(stderr, "ERROR: Attempted to define a function outside top scope at file index %zu!\n", currentToken().fileIndex);
		exit(1);
	}

	//add to function table if not already there (could happen if called above definition)
	uint32_t functionID = findInFunctionTable(identifier);
	if (functionID == (uint32_t)-1) {
		appendToFunctionTable(identifier, nextFunctionID);
		functionID = nextFunctionID;
		++nextFunctionID;
	}

	//create function definition bytecode
	initialiseFunctionDefinition(functionID);

	//currently assuming no parameters
	//TODO support parameters
	incrementToken();

	//confirm that its a function definition
	if (currentToken().type != TOKEN_SYMBOL_COLON) {
		unexpectedToken();
	}

	//currently assuming no descriptors
	//TODO support function descriptors
	incrementToken();

	if (currentToken().type != TOKEN_SYMBOL_BRACE_LEFT) {
		unexpectedToken();
	}
	++scopeDepth;

	//create first block header
	nextVariableID = 1; //currently assuming no parameters
	currentFunctionBlockCount = 1;
	currentBlockInstructionCount = 0;
	initialiseBlockDefinition(0); //always 0 args, inherits function params

	//parse function body
	incrementToken();
	parse();

	//finalise last block
	finaliseBlockDefinition(currentBlockInstructionCount);

	//finalise function
	finaliseFunctionDefinition(currentFunctionBlockCount, 0, 0);
	currentFunctionBlockCount = 0;
}

void parseFunctionArgument() {
	switch (currentToken().type) {
		case TOKEN_LITERAL_STRING:
		insertValue(parseStringLiteral(currentToken()), 4);
		return;

		//skip commas and closing
		case TOKEN_SYMBOL_COMMA:
		case TOKEN_SYMBOL_PARENTHESIS_RIGHT:
		return;

		default:
		unexpectedToken();
		return;
	}
}

//starts on the closing parenthesis or first parameter
uint32_t parseFunctionCall(struct Token identifier) {
	//add to function table if not already there (could happen if called above definition)
	uint32_t functionID = findInFunctionTable(identifier);
	if (functionID == (uint32_t)-1) {
		appendToFunctionTable(identifier, nextFunctionID);
		functionID = nextFunctionID;
		++nextFunctionID;
	}

	//create instruction
	insertValue(functionID, 4);
	++currentBlockInstructionCount;

	//parse arguments
	while (currentToken().type != TOKEN_SYMBOL_PARENTHESIS_RIGHT) {
		parseFunctionArgument();
		incrementToken();
	}

	incrementToken();

	return 0; //TODO, handle proper return value
}

//starts on the opening parenthesis
uint32_t parseFunction(struct Token identifier) {
	incrementToken();

	switch (currentToken().type) {
		//since both scenarios are followed by a colon in the case of a definition we can use the same code
		case TOKEN_SYMBOL_PARENTHESIS_RIGHT:
		case TOKEN_IDENTIFIER:
		if (nextToken().type == TOKEN_SYMBOL_COLON) {
			parseFunctionDefinition(identifier);
			return 0;
		} else {
			return parseFunctionCall(identifier);
		}

		//literal means a function call
		case TOKEN_LITERAL_STRING:
		case TOKEN_LITERAL_CHARACTER:
		case TOKEN_LITERAL_INT:
		case TOKEN_LITERAL_FLOAT:
		return parseFunctionCall(identifier);

		default:
		unexpectedToken();
		return 0;
	}
}

void parseTypeIdentifier() {
	if (currentToken().type != TOKEN_IDENTIFIER) {
		unexpectedToken();
	}

	fseek(srcPtr, currentToken().fileIndex, SEEK_SET);
	char typeChar = fgetc(srcPtr);

	enum IRType typeType;
	//get IR type
	switch (typeChar) {
		case 'i': typeType = IR_INTEGER; break;
		case 'u': typeType = IR_UNSIGNED; break;
		case 'f': typeType = IR_FLOAT; break;
		
		case 'b':;
		char boolBuff[4]; //confirm actually full bool name
		boolBuff[3] = '\n';
		fread(boolBuff, 1, 3, srcPtr);
		if (strcmp(boolBuff, "ool") != 0) {
			fprintf(stderr, "ERROR: Unknown type identifier at file index %zu!\n", currentToken().fileIndex);
			exit(1);
		}
		insertTypeIdentifier(IR_BOOL, 3, false); //bools are one byte always, but insert one byte as the size to be safe
		return;

		//TODO support pointers

		default:
		fprintf(stderr, "ERROR: Unknown type identifier at file index %zu!\n", currentToken().fileIndex);
		exit(1);
	}

	//get size
	size_t sizeLen = currentToken().fileIndex + currentToken().length - ftell(srcPtr);
	char sizeBuff[sizeLen + 1];
	sizeBuff[sizeLen] = '\0'; //null terminate (thanks C)
	fread(sizeBuff, 1, sizeLen, srcPtr);

	//get size exponent
	size_t typeSize = strtol(sizeBuff, NULL, 10);
	uint8_t exponent = 0;
	size_t exponentTest = 1;
	//get as exponent
	while (exponentTest != typeSize) {
		if (exponentTest == 0) {
			fprintf(stderr, "ERROR: Type identifier with non power-of-two size at index %zu!\n", currentToken().fileIndex);
			exit(1);
		}
		exponentTest <<= 1;
		++exponent;
	}

	insertTypeIdentifier(typeType, exponent, false);
}

//does not increment token, starts on first token of operation symbol
enum Operation parseOperation() {
	switch (currentToken().type) {
		case TOKEN_SYMBOL_EQUAL: return OPERATION_ASSIGN;

		case TOKEN_SYMBOL_PLUS: return OPERATION_ADD;
		case TOKEN_SYMBOL_MINUS: return OPERATION_SUBTRACT;
		case TOKEN_SYMBOL_STAR: return OPERATION_MULTIPLY;
		case TOKEN_SYMBOL_SLASH_FORWARD: return OPERATION_DIVIDE;

		default:
		unexpectedToken();
		return -1; //will never return, to make the language server shut up
	}
}

//starts on the first token of the operation, ends
uint32_t parseExpression(enum Operation lastOp) {
	//get variable
	uint32_t variableID = parseOperand();

	//do complicated parsing
	while (currentToken().type != TOKEN_SYMBOL_SEMICOLON) {
		enum Operation op = parseOperation();

		//return early if gone down in precedence
		if (operationPrecedence(op) < operationPrecedence(lastOp)) {
			return variableID;
		}
		
		//recurse
		uint32_t next = parseExpression(op);

		//emit bytecode
		uint32_t result = nextVariableID;
		++nextVariableID;

		switch (op) {
			case OPERATION_ADD: insertValue(-128, 4); break;
			case OPERATION_SUBTRACT: insertValue(-129, 4); break;
			case OPERATION_MULTIPLY: insertValue(-130, 4); break;
			case OPERATION_DIVIDE: insertValue(-131, 4); break;
			
			default:
			fprintf(stderr, "ERROR: attempted to insert unsupported instruction\n");
			exit(1);
		}
		insertValue(result, 4);
		//test for constants
		if (variableID == 0) {

		}
		
		++currentBlockInstructionCount;

		//prepare for next loop
		variableID = result;
	}

	//return if end of statement
	return variableID;
}

//starts on the colon
uint32_t parseVariableDefinition(struct Token identifier) {
	//technically unnecessary check but why not
	if (currentToken().type != TOKEN_SYMBOL_COLON) {
		unexpectedToken();
	}
	incrementToken();

	//create bytecode variable declaration
	insertValue((uint64_t)-1, 4); //variable declaration instruction
	parseTypeIdentifier(); //assume next token is type identifier

	uint32_t variableID = nextVariableID;
	++nextVariableID;

	incrementToken();
	switch (currentToken().type) {
		case TOKEN_SYMBOL_SEMICOLON: //just declaration
		incrementToken();
		return variableID;

		case TOKEN_SYMBOL_EQUAL: //has assignment
		incrementToken();
		uint32_t expressionResult = parseExpression(OPERATION_NONE);

		insertValue(-2, 4); //move
		insertValue(variableID, 4); //into variable
		insertValue(expressionResult, 4); //from expression
		++currentBlockInstructionCount;

		return variableID;

		default:
		unexpectedToken();
		return 0;
	}
}

//starts on the identifier
//returns ssa variable ID
uint32_t parseIdentifier() {
	struct Token identifier = currentToken();
	incrementToken();
	
	switch (currentToken().type) {
		case TOKEN_SYMBOL_PARENTHESIS_LEFT: return parseFunction(identifier); //function call/definition
		case TOKEN_SYMBOL_COLON: return parseVariableDefinition(identifier);

		default:
		unexpectedToken();
		return 0;
	}
}

//returns the ssa variable ID of whatever the operand refers to
//ends on the token after the operand
uint32_t parseOperand() {
	switch (currentToken().type) {
		case TOKEN_IDENTIFIER: return parseIdentifier();
		case TOKEN_LITERAL_STRING: return parseStringLiteral(currentToken());

		case TOKEN_LITERAL_CHARACTER:
		case TOKEN_LITERAL_INT:
		case TOKEN_LITERAL_FLOAT:
		return 0; //inform that this will have to be processed later

		default:
		unexpectedToken();
	}
	return 0;
}

void parse() {
	while (currentToken().type != TOKEN_EOF) {
		switch (currentToken().type) {
			case TOKEN_IDENTIFIER:
			parseIdentifier();
			break;

			//end of statement
			case TOKEN_SYMBOL_SEMICOLON:
			incrementToken();
			break;

			//end of scope
			case TOKEN_SYMBOL_BRACE_RIGHT:
			--scopeDepth;
			incrementToken();
			return;
	
			default:
			unexpectedToken();
		}
	}

	//a bit of debug info
	printf("Parser exited via EOF");
}

struct ByteArray parseFile(FILE* filePtr) {
	srcPtr = filePtr;

	//setup
	resetTokeniser(filePtr);
	resetBytecodeGen(filePtr);
	resetState();

	//do the thing
	parse();

	//final checks
	if (scopeDepth > 0) {
		fprintf(stderr, "ERROR: Scope depth did not return to zero by EOF, are you missing a brace?");
		exit(1);
	}

	struct ByteArray bytecode = finaliseBytecode();
	return bytecode;
}
