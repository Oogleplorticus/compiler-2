#pragma once

#include <stdbool.h>
#include <stddef.h>

enum TokenType {
	//misc
	TOKEN_UNDEFINED,
	TOKEN_EOF,

	//basic
	TOKEN_IDENTIFIER,

	//literal
	TOKEN_LITERAL_STRING,
	TOKEN_LITERAL_CHARACTER,
	TOKEN_LITERAL_INT,
	TOKEN_LITERAL_FLOAT,

	//keyword

	//symbol
	TOKEN_SYMBOL_PARENTHESIS_LEFT,
	TOKEN_SYMBOL_PARENTHESIS_RIGHT,
	TOKEN_SYMBOL_BRACKET_LEFT,
	TOKEN_SYMBOL_BRACKET_RIGHT,
	TOKEN_SYMBOL_BRACE_LEFT,
	TOKEN_SYMBOL_BRACE_RIGHT,

	TOKEN_SYMBOL_COLON,
	TOKEN_SYMBOL_SEMICOLON,

	TOKEN_SYMBOL_COMMA,

	TOKEN_SYMBOL_EQUAL,

	TOKEN_SYMBOL_PLUS,
	TOKEN_SYMBOL_MINUS,
	TOKEN_SYMBOL_STAR,
	TOKEN_SYMBOL_SLASH_FORWARD,

	TOKEN_SYMBOL_SLASH_BACKWARD,
};

struct Token {
	enum TokenType type;
	size_t fileIndex;
	size_t length;
	bool seperatedFromPrevious;
};

void printToken(const struct Token token);

enum TokenType charToSymbolTokenType(char c);