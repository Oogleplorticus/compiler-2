#include "token.h"

#include <stdio.h>

void printToken(const struct Token token) {
	printf("Type: %d,	Index: %zu,	Length: %zu,	Seperated: %d\n", 
		token.type, token.fileIndex, token.length, token.seperatedFromPrevious);
}

enum TokenType charToSymbolTokenType(char c) {
	switch (c) {
		case '(': return TOKEN_SYMBOL_PARENTHESIS_LEFT;
		case ')': return TOKEN_SYMBOL_PARENTHESIS_RIGHT;
		case '[': return TOKEN_SYMBOL_BRACKET_LEFT;
		case ']': return TOKEN_SYMBOL_BRACKET_RIGHT;
		case '{': return TOKEN_SYMBOL_BRACE_LEFT;
		case '}': return TOKEN_SYMBOL_BRACE_RIGHT;

		case ':': return TOKEN_SYMBOL_COLON;
		case ';': return TOKEN_SYMBOL_SEMICOLON;

		case ',': return TOKEN_SYMBOL_COMMA;

		case '=': return TOKEN_SYMBOL_EQUAL;

		case '+': return TOKEN_SYMBOL_PLUS;
		case '-': return TOKEN_SYMBOL_MINUS;
		case '*': return TOKEN_SYMBOL_STAR;
		case '/': return TOKEN_SYMBOL_SLASH_FORWARD;

		case '\\': return TOKEN_SYMBOL_SLASH_BACKWARD;

		default: return TOKEN_UNDEFINED;
	}
}