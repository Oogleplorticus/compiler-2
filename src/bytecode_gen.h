#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "token.h"

enum IRType {
	IR_INTEGER = 1,
	IR_UNSIGNED = 2,
	IR_FLOAT = 3,
	IR_BOOL = 4,

	IR_POINTER_INTEGER = -1,
	IR_POINTER_UNSIGNED = -2,
	IR_POINTER_FLOAT = -3,
	IR_POINTER_BOOL = -4,

	//special case
	IR_POINTER_VOID = 0,
};

void appendToFunctionTable(struct Token identifier, size_t ID);
//returns -1 if not in table
size_t findInFunctionTable(struct Token identifier);

//type is of the pointee, returns the variable ID of the pointer to it, IT IS NOT THE ID OF THE DATA ITSELF
size_t createStaticData(enum IRType type, size_t sizePower, size_t count, char* data);

//the size is 2^sizePower, isStatic is true when inserting into static data, false when inserting into program logic
void insertTypeIdentifier(enum IRType type, size_t sizePower, bool isStatic);

void insertID32(size_t ID);
void insertConstant(enum IRType type, size_t sizePower, size_t value);

void initialiseFunctionDefinition(size_t ID);
void finaliseFunctionDefinition(size_t blockCount, size_t inCount, size_t outCount);

void initialiseBlockDefinition(size_t argumentCount);
void finaliseBlockDefinition(size_t instructionCount);

//MUST be called before using other functions
void resetBytecodeGen(FILE* filePtr);

struct ByteArray finaliseBytecode();
void freeBytecodeSections();