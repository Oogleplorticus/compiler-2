#pragma once

#include <stddef.h>

enum Operation {
	//misc
	OPERATION_NONE, //has minimum precedence

	OPERATION_ASSIGN,
	
	//general maths
	OPERATION_ADD,
	OPERATION_SUBTRACT,
	OPERATION_MULTIPLY,
	OPERATION_DIVIDE,
};

//higher value has greater precedence
size_t operationPrecedence(enum Operation op);