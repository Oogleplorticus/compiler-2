#include "parser.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "byte_array.h"
#include "bytecode_gen.h"
#include "token.h"
#include "tokeniser.h"

static FILE* srcPtr = NULL;

//state variables
static size_t nextFunctionID = 0;
static size_t scopeDepth = 0;

static size_t currentFunctionBlockCount = 0;
static size_t currentBlockInstructionCount = 0;

void resetState() {
	nextFunctionID = 0;
	scopeDepth = 0;

	currentFunctionBlockCount = 0;
	currentBlockInstructionCount = 0;
}

//forward declaration
void parse();

void unexpectedToken() {
	fprintf(stderr, "ERROR: Unexpected token of type %d at file index %zu!\n", currentToken().type, currentToken().fileIndex);
	exit(1);
}

void parseStringLiteral(struct Token literal) {
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

	size_t pointerVariableID = createStaticData(IR_UNSIGNED, 3, trueLiteralLength, buffer);
	insertID32(pointerVariableID);
	insertConstant(IR_UNSIGNED, (size_t)-1, trueLiteralLength);

	free(buffer);
}

//starts on the closing parenthesis or first parameter
void parseFunctionDefinition(struct Token identifier) {
	//cant define a function inside a function
	if (scopeDepth > 0) {
		fprintf(stderr, "ERROR: Attempted to define a function outside top scope at file index %zu!\n", currentToken().fileIndex);
		exit(1);
	}

	//add to function table if not already there (could happen if called above definition)
	size_t functionID = findInFunctionTable(identifier);
	if (functionID == (size_t)-1) {
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

	//continue parsing
	parse();
}

void parseFunctionArgument() {
	switch (currentToken().type) {
		case TOKEN_LITERAL_STRING:
		parseStringLiteral(currentToken());
		return;

		//skip commas and closing
		case TOKEN_SYMBOL_COMMA:
		case TOKEN_SYMBOL_PARENTHESIS_RIGHT:
		return;

		default:
		unexpectedToken();
	}
}

//starts on the closing parenthesis or first parameter
void parseFunctionCall(struct Token identifier) {
	//add to function table if not already there (could happen if called above definition)
	size_t functionID = findInFunctionTable(identifier);
	if (functionID == (size_t)-1) {
		appendToFunctionTable(identifier, nextFunctionID);
		functionID = nextFunctionID;
		++nextFunctionID;
	}

	//create instruction
	insertID32(functionID);
	++currentBlockInstructionCount;

	//parse arguments
	while (currentToken().type != TOKEN_SYMBOL_PARENTHESIS_RIGHT) {
		parseFunctionArgument();
		incrementToken();
	}

	incrementToken();
	parse();
}

//starts on the opening parenthesis
void parseFunction(struct Token identifier) {
	incrementToken();

	switch (currentToken().type) {
		//since both scenarios are followed by a colon in the case of a definition we can use the same code
		case TOKEN_SYMBOL_PARENTHESIS_RIGHT:
		case TOKEN_IDENTIFIER:
		if (nextToken().type == TOKEN_SYMBOL_COLON) {
			parseFunctionDefinition(identifier);
		} else {
			parseFunctionCall(identifier);
		}
		return;

		//literal means a function call
		case TOKEN_LITERAL_STRING:
		case TOKEN_LITERAL_CHARACTER:
		case TOKEN_LITERAL_INT:
		case TOKEN_LITERAL_FLOAT:
		parseFunctionCall(identifier);
		return;

		default:
		unexpectedToken();
	}
}

//starts on the identifier
void parseIdentifier() {
	struct Token identifier = currentToken();
	incrementToken();
	
	switch (currentToken().type) {
		case TOKEN_SYMBOL_PARENTHESIS_LEFT: //function call/definition
		parseFunction(identifier);
		return;

		default:
		unexpectedToken();
	}
}

void parse() {
	switch (currentToken().type) {
		case TOKEN_IDENTIFIER:
		parseIdentifier();
		return;

		//end of scope
		case TOKEN_SYMBOL_BRACE_RIGHT:
		--scopeDepth;
		incrementToken();
		return;

		//end of statement
		case TOKEN_SYMBOL_SEMICOLON:
		incrementToken();
		parse();
		return;

		//finish
		case TOKEN_EOF:
		if (scopeDepth > 0) {
			fprintf(stderr, "ERROR: Scope depth did not return to zero by EOF, are you missing a brace?");
			exit(1);
		}
		return;

		default:
		unexpectedToken();
	}
}

struct ByteArray parseFile(FILE* filePtr) {
	srcPtr = filePtr;

	//setup
	resetTokeniser(filePtr);
	resetBytecodeGen(filePtr);
	resetState();

	//do the thing
	parse();

	struct ByteArray bytecode = finaliseBytecode();
	return bytecode;
}
