#include "bytecode_gen.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "byte_array.h"
#include "token.h"

static FILE* srcPtr = NULL;

//bytecode sections
//header
static char staticHeaderData[] = {
	0x78, 0x70, 0x62, 0xC0, //magic number
	0x00, 0x00, 0x00, 0x00, //version major
	0x01, 0x00, 0x00, 0x00, //version minor
	0x00, 0x00, 0x00, 0x00, //version patch
	//section table, to be filled in at finalisation
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //Function table
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //Static variables
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //Program logic

};
static struct ByteArray staticHeader = {staticHeaderData, sizeof(staticHeaderData)}; //should never be freed (its static so duh)

static struct ByteArray functionTable = {NULL, 0};
//data
static struct ByteArray staticVariables = {NULL, 0};
static struct ByteArray programLogic = {NULL, 0};

static size_t functionCount = 0;

static size_t nextStaticID = (size_t)-1;

static size_t currentFunctionIndex = 0;
static size_t currentBlockIndex = 0;



//intended primarily for specification functions
void appendToFunctionTableDirect(const char* identifier, size_t ID) {
	size_t identifierLength = strlen(identifier);

	//create new function
	size_t newFunctionLength = identifierLength + 12; //8 bytes for identifier length, 4 for ID
	struct ByteArray newFunction = allocByteArray(newFunctionLength);

	//fill data
	//potential errors here if size_t is less than 64 bits
	memcpy(newFunction.ptr, &ID, 4); //function id
	memcpy(newFunction.ptr + 4, &identifierLength, 8); //length of identifier
	memcpy(newFunction.ptr + 12, identifier, identifierLength); //identifier

	//make new table
	struct ByteArray newTable = combineByteArrays(functionTable, newFunction);

	//free old data
	freeByteArray(&functionTable);
	freeByteArray(&newFunction);

	functionTable = newTable;
	++functionCount;
}

void appendToFunctionTable(struct Token identifier, size_t ID) {
	//create new function
	size_t newFunctionLength = identifier.length + 12; //8 bytes for identifier length, 4 for ID
	struct ByteArray newFunction = allocByteArray(newFunctionLength);

	//fill data
	//potential errors here if size_t is less than 64 bits
	memcpy(newFunction.ptr, &ID, 4); //function id
	memcpy(newFunction.ptr + 4, &identifier.length, 8); //length of identifier
	fseek(srcPtr, identifier.fileIndex, SEEK_SET); //identifier
	fread(newFunction.ptr + 12, sizeof(char), identifier.length, srcPtr);

	//make new table
	struct ByteArray newTable = combineByteArrays(functionTable, newFunction);

	//free old data
	freeByteArray(&functionTable);
	freeByteArray(&newFunction);

	functionTable = newTable;
	++functionCount;
}

//this should technically return a u32, but this is C so im just going to truncate
//also DISGUSTING code here
size_t findInFunctionTable(struct Token identifier) {
	const size_t maxIndex = functionTable.length - identifier.length;
	if (maxIndex > functionTable.length) {
		return -1;
	}

	size_t length = 0;
	size_t searchIndex = 8;

	bool searching = true;
	while (searching) {
		memcpy(&length, functionTable.ptr + searchIndex, 8); //so much memory unsafety holy shit
		searchIndex += 8;
		if (length == identifier.length) {
			bool match = true;
			for (size_t i = 0; i < length; ++i) {
				fseek(srcPtr, identifier.fileIndex + i, SEEK_SET);
				if (fgetc(srcPtr) != *(functionTable.ptr + searchIndex + i)) {
					match = false;
				}			
			}
			if (match) {
				size_t functionID;
				memcpy(&functionID, functionTable.ptr + searchIndex - 12, 4); //memory unsafe
				return functionID;
			} else {
				searchIndex += length + 4;
			}
		}

		if (searchIndex >= maxIndex) {
			searching = false;
		}
	}
	
	return -1;
}

size_t createStaticData(enum IRType type, size_t sizePower, size_t count, char* data) {
	if (sizePower < 3) {
		fprintf(stderr, "ERROR: Size powers less than 3 currently not supported\n");
		exit(1);
	}

	size_t typeSize = 1 << (sizePower - 3);
	struct ByteArray staticData = allocByteArray(14 + (typeSize * count));
	memcpy(staticData.ptr, &nextStaticID, 4);
	--nextStaticID;
	memcpy(staticData.ptr + 4, &type, 1);
	memcpy(staticData.ptr + 5, &sizePower, 1);
	memcpy(staticData.ptr + 6, &count, 8);
	memcpy(staticData.ptr + 14, data, typeSize * count);

	struct ByteArray newStatic = combineByteArrays(staticVariables, staticData);
	freeByteArray(&staticVariables);
	freeByteArray(&staticData);

	staticVariables = newStatic;

	return nextStaticID + 1;
}

void insertTypeIdentifier(enum IRType type, size_t sizePower, bool isStatic) {
	if (sizePower < 3) {
		fprintf(stderr, "ERROR: Size powers less than 3 currently not supported\n");
		exit(1);
	}

	struct ByteArray* insertInto = &programLogic;
	if (isStatic) {
		insertInto = &staticVariables;
	}

	struct ByteArray identifier = allocByteArray(2);
	memcpy(identifier.ptr, &type, 1);
	memcpy(identifier.ptr + 1, &sizePower, 1);
	
	struct ByteArray newLogic = combineByteArrays(*insertInto, identifier);
	freeByteArray(insertInto);
	freeByteArray(&identifier);

	*insertInto = newLogic;
}

void insertID32(size_t ID) {
	struct ByteArray id = allocByteArray(4);
	memcpy(id.ptr, &ID, 4);

	struct ByteArray newLogic = combineByteArrays(programLogic, id);
	freeByteArray(&programLogic);
	freeByteArray(&id);

	programLogic = newLogic;
}

void insertConstant(enum IRType type, size_t sizePower, size_t value) {
	size_t typeSize = 1 << (sizePower - 3);

	size_t constantSize = 6 + typeSize;
	//check if size type
	bool isSizeType = sizePower == (size_t)-1;
	if (isSizeType) {
		typeSize = sizeof(size_t);
		constantSize = 7 + typeSize;
	}

	struct ByteArray constant = allocByteArray(constantSize);
	memcpy(constant.ptr + 4, &type, 1);
	memcpy(constant.ptr + 5, &sizePower, 1);
	if (isSizeType) {
		//find what power of two the size_t is
		size_t sizeTSize = sizeof(size_t);
		size_t dataSizePower = 0;
		while (sizeTSize >>= 1) ++dataSizePower;
		dataSizePower += 3;
		memcpy(constant.ptr + 6, &dataSizePower, 1);
		memcpy(constant.ptr + 7, &value, typeSize);
	} else {
		memcpy(constant.ptr + 6, &value, typeSize);
	}

	struct ByteArray newLogic = combineByteArrays(programLogic, constant);
	freeByteArray(&programLogic);
	freeByteArray(&constant);

	programLogic = newLogic;
}

void initialiseFunctionDefinition(size_t ID) {
	struct ByteArray functionStart = allocByteArray(16);
	memcpy(functionStart.ptr, &ID, 4); //a bit unsafe

	currentFunctionIndex = programLogic.length; //save current function index for later

	struct ByteArray newLogic = combineByteArrays(programLogic, functionStart);
	freeByteArray(&programLogic);
	freeByteArray(&functionStart);

	programLogic = newLogic;
}

//to be used immediately after type identifiers insertion
void finaliseFunctionDefinition(size_t blockCount, size_t outCount, size_t inCount) {
	memcpy(programLogic.ptr + currentFunctionIndex + 4, &blockCount, 4); //a bit unsafe
	memcpy(programLogic.ptr + currentFunctionIndex + 8, &outCount, 4);
	memcpy(programLogic.ptr + currentFunctionIndex + 12, &inCount, 4);
}

void initialiseBlockDefinition(size_t argumentCount) {
	struct ByteArray blockStart = allocByteArray(12);
	memcpy(blockStart.ptr + 8, &argumentCount, 4); //a bit unsafe

	currentBlockIndex = programLogic.length; //save current block index for later

	struct ByteArray newLogic = combineByteArrays(programLogic, blockStart);
	freeByteArray(&programLogic);
	freeByteArray(&blockStart);

	programLogic = newLogic;
}

void finaliseBlockDefinition(size_t instructionCount) {
	memcpy(programLogic.ptr + currentBlockIndex, &instructionCount, 8); //a bit unsafe
}

void resetBytecodeGen(FILE* filePtr) {
	srcPtr = filePtr;

	//dont do memory leaks
	freeBytecodeSections();

	//allocate initial
	functionTable = allocByteArray(4); //for function count
	staticVariables = allocByteArray(4); //for static count

	//reset variables
	functionCount = 0;

	nextStaticID = (size_t)-1;

	currentFunctionIndex = 0;
	currentBlockIndex = 0;

	//set specification defined functions
	appendToFunctionTableDirect("print", (size_t)-256);
}

struct ByteArray finaliseBytecode() {
	//create a temporary deep copy of staticHeader
	struct ByteArray staticHeaderCopy = allocByteArray(staticHeader.length);
	memcpy(staticHeaderCopy.ptr, staticHeader.ptr, staticHeaderCopy.length);

	//calculate section offsets
	size_t functionTableOffset = staticHeaderCopy.length;
	size_t staticVariablesOffset = functionTableOffset + functionTable.length;
	size_t programLogicOffset = staticVariablesOffset + staticVariables.length;

	//insert section offsets
	memcpy(staticHeaderCopy.ptr + 16, &functionTableOffset, 8); //a bit unsafe
	memcpy(staticHeaderCopy.ptr + 24, &staticVariablesOffset, 8);
	memcpy(staticHeaderCopy.ptr + 32, &programLogicOffset, 8);

	//insert other variables
	memcpy(functionTable.ptr, &functionCount, 4); //a bit unsafe
	size_t staticCount = (nextStaticID * -1) - 1;
	memcpy(staticVariables.ptr, &staticCount, 4);


	
	//combine
	struct ByteArray temp0 = combineByteArrays(staticHeaderCopy, functionTable);
	struct ByteArray temp1 = combineByteArrays(staticVariables, programLogic);

	struct ByteArray bytecode = combineByteArrays(temp0, temp1);

	//free temps
	freeByteArray(&staticHeaderCopy);
	freeByteArray(&temp0);
	freeByteArray(&temp1);

	return bytecode;
}

void freeBytecodeSections() {
	freeByteArray(&functionTable);
	freeByteArray(&staticVariables);
	freeByteArray(&programLogic);
}