#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "operation.h"
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

void appendToFunctionTable(struct Token identifier, uint32_t ID);
//returns -1 if not in table
uint32_t findInFunctionTable(struct Token identifier);

//type is of the pointee, returns the variable ID of the pointer to it, IT IS NOT THE ID OF THE DATA ITSELF
uint32_t createStaticData(enum IRType type, uint8_t sizeExp, uint64_t count, char* data);

//the size is 2^sizeExp, isStatic is true when inserting into static data, false when inserting into program logic
void insertTypeIdentifier(enum IRType type, uint8_t sizeExp, bool isStatic);

void insertValue(uint64_t value, size_t bytes);
void insertConstant(enum IRType type, uint8_t sizeExp, uint64_t value);

void initialiseFunctionDefinition(uint32_t ID);
void finaliseFunctionDefinition(uint64_t blockCount, uint32_t inCount, uint32_t outCount);

void initialiseBlockDefinition(uint32_t argumentCount);
void finaliseBlockDefinition(uint64_t instructionCount);

//MUST be called before using other functions
void resetBytecodeGen(FILE* filePtr);

struct ByteArray finaliseBytecode();
void freeBytecodeSections();