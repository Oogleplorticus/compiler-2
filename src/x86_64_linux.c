#include "x86_64_linux.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//file details
static FILE* ssaPtr = NULL;
static FILE* asmPtr = NULL;

static size_t functionTableOffset = 0;
static size_t staticVariablesOffset = 0;
static size_t programLogicOffset = 0;

static size_t lowestStaticID = 4294967295;

//register variables
static bool registerUsage[16];
static const int registerPriorities[] = {
	12, 13, 14, 15, 1, //fancier register allocation later, all callee saved
};

int allocateRegister() {
	for (size_t i = 0; i < sizeof(registerPriorities) / sizeof(int); ++i) {
		if (!registerUsage[registerPriorities[i]]) {
			registerUsage[registerPriorities[i]] = true;
			return registerPriorities[i];
		}
	}
	return -1;
}

void generateStaticVariable() {
	size_t variableID;
	fread(&variableID, 4, 1, ssaPtr);

	//set label
	fprintf(asmPtr, "	sv_%zu ", variableID);

	//skip type, it doesnt matter here
	fseek(ssaPtr, 1, SEEK_CUR);
	//get size, it does matter here
	size_t sizeExp = 0;
	fread(&sizeExp, 1, 1, ssaPtr);
	if (sizeExp < 3) {
		fprintf(stderr, "ERROR: Size exponents less than 3 currently not supported\n");
		exit(1);
	}

	//set data size
	size_t dataSize = 1 << (sizeExp - 3);
	switch (sizeExp) {
		case 3: fprintf(asmPtr, "db "); break;
		case 4: fprintf(asmPtr, "d2 "); break;
		case 5: fprintf(asmPtr, "dd "); break;
		case 6: fprintf(asmPtr, "dq "); break;

		default:
		fprintf(stderr, "ERROR: Unsupported static variable data size\n");
		exit(1);
	}

	//get data count
	size_t dataCount = 0;
	fread(&dataCount, 8, 1, ssaPtr);
	dataCount *= dataSize;

	//output data
	char byte;
	for (size_t i = 0; i < dataCount; ++i) {
		byte = fgetc(ssaPtr);
		fprintf(asmPtr, "%d,", byte);
	}

	//newline
	fseek(asmPtr, -1, SEEK_CUR);
	fputc('\n', asmPtr);
}

void generateDataSection() {
	fprintf(asmPtr, "section .data\n");
	fseek(ssaPtr, staticVariablesOffset, SEEK_SET);

	size_t staticCount = 0;
	fread(&staticCount, 4, 1, ssaPtr);

	for (size_t i = 0; i < staticCount; ++i) {
		generateStaticVariable();
	}

	//bonus newline
	fputc('\n', asmPtr);
}

//memory must be freed after use, will affect file index
char* getFunctionIdentifier(size_t ID) {
	fseek(ssaPtr, functionTableOffset, SEEK_SET);

	size_t functionCount = 0;
	fread(&functionCount, 4, 1, ssaPtr);

	for (size_t i = 0; i < functionCount; ++i) {
		size_t functionID = 0;
		fread(&functionID, 4, 1, ssaPtr);

		if (functionID == ID) {
			size_t identifierLength = 0;
			fread(&identifierLength, 8, 1, ssaPtr);
			char* identifier = calloc(identifierLength + 1, 1);
			if (identifier == NULL) {
				fprintf(stderr, "ERROR: Could not allocate memory for function identifier!\n");
				exit(1);
			}
			fread(identifier, 1, identifierLength, ssaPtr);
			identifier[identifierLength] = '\0';
			return identifier;
		}

		size_t identifierLength = 0;
		fread(&identifierLength, 8, 1, ssaPtr);
		fseek(ssaPtr, identifierLength, SEEK_CUR);
	}

	return NULL;
}

void generateConstant() {
	//skip type byte
	fseek(ssaPtr, 1, SEEK_CUR);

	//get size
	size_t sizeExp = 0;
	fread(&sizeExp, 1, 1, ssaPtr);
	if (sizeExp < 3) {
		fprintf(stderr, "ERROR: Size exponents less than 3 currently not supported!\n");
		exit(1);
	}
	size_t dataSize = 1 << (sizeExp - 3);
	if (sizeExp == 255) {
		fread(&sizeExp, 1, 1, ssaPtr);
		dataSize = 1 << (sizeExp - 3);
	}

	//insert data
	if (dataSize <= sizeof(size_t)) {
		size_t data = 0;
		fread(&data, dataSize, 1, ssaPtr);
		fprintf(asmPtr, "%zu", data);
	} else {
		fprintf(stderr, "ERROR: Constant data size larger than currently supported!\n");
		exit(1);
	}
}

void generateSpecFuncPrint() {
	fprintf(asmPtr, "	mov rax, 1 ; print\n	mov rdi, 1\n");

	//find argument ID
	size_t argumentID = 0;
	fread(&argumentID, 4, 1, ssaPtr);

	if (argumentID >= lowestStaticID) {
		fprintf(asmPtr, "	mov rsi, sv_%zu\n", argumentID);
	} else if (argumentID == 0) {
		fprintf(stderr, "ERROR: Can't have constant pointer in print!\n");
		exit(1);
	} else {
		//handle properly
	}

	//print start of next line
	fprintf(asmPtr, "	mov rdx, ");

	//next argument ID
	fread(&argumentID, 4, 1, ssaPtr);

	if (argumentID >= lowestStaticID) {
		//handle properly
	} else if (argumentID == 0) {
		generateConstant();
	} else {
		//handle properly
	}

	//print finishing touches
	fprintf(asmPtr, "\n	syscall\n");
}

void generateInstruction() {
	//find instruction ID
	size_t instructionID = 0;
	fread(&instructionID, 4, 1, ssaPtr);

	switch (instructionID) {
		case 4294967040:
		generateSpecFuncPrint();
		return;

		default: break;
	}

	//do default func handling here
}

void generateBlock() {
	//find argument count
	size_t instructionCount = 0;
	fread(&instructionCount, 8, 1, ssaPtr);

	//TODO allocate registers for arguments, currently assume no arguments
	fseek(ssaPtr, 4, SEEK_CUR);

	//generate instructions
	for (size_t i = 0; i < instructionCount; ++i) {
		generateInstruction();
	}
}

//closer to generateFunction since theres no count in the definitions but whatever
void generateTextSection() {
	fprintf(asmPtr, "section .text\n	global _start\n\n");
	fseek(ssaPtr, programLogicOffset, SEEK_SET);

	//find ID
	size_t functionID = 0;
	fread(&functionID, 4, 1, ssaPtr);

	//save file index
	size_t fileIndex = ftell(ssaPtr);

	//get identifier
	char* identifier = getFunctionIdentifier(functionID);

	//restore file index
	fseek(ssaPtr, fileIndex, SEEK_SET);

	//create label
	bool mainFunc = false;
	if (identifier == NULL) {
		fprintf(stderr, "ERROR: Could not find function identifier!\n");
		exit(1);
	}
	if (strcmp(identifier, "main") == 0) {
		mainFunc = true;
		fprintf(asmPtr, "_start:\n");
		free(identifier);
	} else {
		fprintf(asmPtr, "_%s:\n", identifier);
		free(identifier);
	}

	//find block count
	size_t blockCount = 0;
	fread(&blockCount, 4, 1, ssaPtr);

	//TODO allocate registers for parameters, currently assume no parameters
	fseek(ssaPtr, 8, SEEK_CUR);

	//generate blocks
	for (size_t i = 0; i < blockCount; ++i) {
		generateBlock();
	}

	//if main, call exit syscall
	if (mainFunc) {
		fprintf(asmPtr, "	mov rax, 60 ; exit\n	mov rdi, 0\n	syscall");
	}
}

void loadBytecodeData() {
	fseek(ssaPtr, 0, SEEK_SET);

	//check magic number
	static char mNumBuff[4];
	fread(mNumBuff, 1, 4, ssaPtr);
	static const char mNum[] = {0x78, 0x70, 0x62, 0xc0};
	if (strncmp(mNumBuff, mNum, 4) != 0) {
		fprintf(stderr, "ERROR: Bytecode file magic number incorrect!\n");
		exit(1);
	}

	//skip over version, cant be bothered to check it
	fseek(ssaPtr, 16, SEEK_SET);

	//load offsets
	fread(&functionTableOffset, 8, 1, ssaPtr);
	fread(&staticVariablesOffset, 8, 1, ssaPtr);
	fread(&programLogicOffset, 8, 1, ssaPtr);
	
	//set lowestStaticID
	fseek(ssaPtr, staticVariablesOffset, SEEK_SET);
	size_t staticCount = 0;
	fread(&staticCount, 4, 1, ssaPtr);
	lowestStaticID = 4294967295 - staticCount;
}

void generateASM(FILE* inPtr, FILE* outPtr) {
	ssaPtr = inPtr;
	asmPtr = outPtr;

	loadBytecodeData();

	generateDataSection();
	generateTextSection();
}