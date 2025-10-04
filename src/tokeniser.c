#include "tokeniser.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "token.h"

static FILE* srcPtr = NULL;

static struct Token tokenCache[2];
//swapped on token increment
static struct Token* currentTokenPtr = &tokenCache[0];
static struct Token* nextTokenPtr = &tokenCache[1];

//returns true if whitespace was skipped
bool skipWhitespace() {
	bool skipped = false;
	char c = fgetc(srcPtr);
	while (isspace(c)) {
		c = fgetc(srcPtr);
		skipped = true;
	}
	//seek back as to not skip characters
	//if eof dont seek so that it can be properly detected later
	if(c != EOF) {
		fseek(srcPtr, -1, SEEK_CUR);
	}
	
	return skipped;
}

//literalType can be either " or '
void skipTextLiteral(char literalType) {
	bool endFound = false;
	bool escape = false;
	char c = 0;
	while (!endFound) {
		c = fgetc(srcPtr);

		//this is horrible
		if (c == EOF) {
			endFound = true;
		} else if (c == literalType && !escape) {
			endFound = true;
		} else if (c == '\\') {
			//this method will correctly handle the situation where you are escaping the escape character
			escape = !escape;
		} else if (escape) {
			escape = false;
		}
	}
}

enum TokenType skipNumberLiteral(char firstChar) {
	//test for different base
	bool based = false;
	if (firstChar == '0') {
		char nextChar = fgetc(srcPtr);
		//no octal because octal is STUPID (maybe later)
		if (nextChar == 'x' || nextChar == 'b') {
			based = true;
			//start after the x
			firstChar = fgetc(srcPtr);
		}
	}

	//do the skipping
	bool decimal = false;
	char c = firstChar;
	while (isdigit(c) || c == '.') {
		c = fgetc(srcPtr);
		if (c == '.') {
			decimal = true;
		}
	}

	//smidge of error checking
	if (decimal && based) {
		fprintf(stderr, "ERROR: Number literal with base and decimal point at file index %zu\n", ftell(srcPtr));
		exit(1);
	}
	
	//return token type
	if (decimal) {
		return TOKEN_LITERAL_FLOAT;
	}
	return TOKEN_LITERAL_INT;
}

void skipIdentifier() {
	char c = '_'; //fullfills while condition
	while (isalnum(c) || c == '_') {
		c = fgetc(srcPtr);
	}
	//seek back as to not skip characters
	fseek(srcPtr, -1, SEEK_CUR);
}

struct Token getToken(size_t searchIndex) {
	struct Token token;
	//initialise default values
	token.type = TOKEN_UNDEFINED;
	token.fileIndex = 0;
	token.length = 1;
	token.seperatedFromPrevious = false;
	
	//seek to search index
	fseek(srcPtr, searchIndex, SEEK_SET);

	//skip whitespace and set some token data
	token.seperatedFromPrevious = skipWhitespace();
	token.fileIndex = ftell(srcPtr);

	//determine token type and length
	char firstChar = fgetc(srcPtr);

	//check for eof
	if (firstChar == EOF) {
		token.type = TOKEN_EOF;
		token.length = 0;
		return token;
	}

	//check if symbol token
	token.type = charToSymbolTokenType(firstChar);
	if (token.type != TOKEN_UNDEFINED) {
		token.length = 1;
		return token;
	}

	//check for text literals
	if (firstChar == '"' || firstChar == '\'') {
		skipTextLiteral(firstChar);
		if (firstChar == '"') {token.type = TOKEN_LITERAL_STRING;}
		else {token.type = TOKEN_LITERAL_CHARACTER;}
		token.length = ftell(srcPtr) - token.fileIndex;
		return token;
	}

	//check if number literal
	if (isdigit(firstChar)) {
		token.type = skipNumberLiteral(firstChar);
		token.length = ftell(srcPtr) - token.fileIndex;
		return token;
	}

	//check if identifier or keyword
	if (isalpha(firstChar) || firstChar == '_') {
		skipIdentifier();
		token.length = ftell(srcPtr) - token.fileIndex;
		token.type = TOKEN_IDENTIFIER; //TODO check for keywords
		//last so no premature return
	}

	return token;
}

void resetTokeniser(FILE* filePtr) {
	srcPtr = filePtr;
	
	//cache initial tokens
	*currentTokenPtr = getToken(0);
	size_t searchIndex = currentTokenPtr->fileIndex + currentTokenPtr->length;
	*nextTokenPtr = getToken(searchIndex);
}

void incrementToken() {
	//swap pointers
	struct Token* tokenPtrTemp = currentTokenPtr;
	currentTokenPtr = nextTokenPtr;
	nextTokenPtr = tokenPtrTemp;

	//cache next token
	size_t searchIndex = currentTokenPtr->fileIndex + currentTokenPtr->length;
	*nextTokenPtr = getToken(searchIndex);
}

struct Token currentToken() {
	return *currentTokenPtr;
}

struct Token nextToken() {
	return *nextTokenPtr;
}

void setTokeniserIndex(size_t index) {
	//cache initial tokens
	*currentTokenPtr = getToken(index);
	index = currentTokenPtr->fileIndex + currentTokenPtr->length;
	*nextTokenPtr = getToken(index);
}