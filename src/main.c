#include <stdio.h>
#include <string.h>

#include "byte_array.h"
#include "parser.h"
#include "x86_64_linux.h"

int main(int argc, char* argv[]) {
	//cl arguments checks
	if (argc != 2) {
		fprintf(stderr, "ERROR: Incorrect argument count.\n");
		return 1;
	}

	//open files
	FILE* srcPtr = fopen(argv[1], "r");
	if (srcPtr == NULL) {
		fprintf(stderr, "ERROR: File [%s] could not be opened.\n", argv[1]);
		return 1;
	}
	FILE* ssaPtr = fopen(strcat(argv[1], ".xpb"), "wb+");
	if (ssaPtr == NULL) {
		fprintf(stderr, "ERROR: File [%s] could not be opened.\n", argv[1]);
		return 1;
	}

	//parse
	struct ByteArray bytecode;
	bytecode = parseFile(srcPtr);

	//output bytecode
	fwrite(bytecode.ptr, bytecode.length, 1, ssaPtr);

	//free bytecode
	freeByteArray(&bytecode);

	//close compilation files
	fclose(srcPtr);

	//open asm file
	FILE* asmPtr = fopen(strcat(argv[1], ".asm"), "w");
	if (ssaPtr == NULL) {
		fprintf(stderr, "ERROR: File [%s] could not be opened.\n", argv[1]);
		return 1;
	}

	generateASM(ssaPtr, asmPtr);

	return 0;
}