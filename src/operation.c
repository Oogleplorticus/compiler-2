#include "operation.h"

#include <stddef.h>

//higher value has greater precedence
size_t operationPrecedence(enum Operation op) {
	switch (op) {
		case OPERATION_NONE: return 0;

		case OPERATION_ASSIGN: return 1;

		case OPERATION_ADD: return 2;
		case OPERATION_SUBTRACT: return 2;
		case OPERATION_MULTIPLY: return 3;
		case OPERATION_DIVIDE: return 3;

		default: return 0;
	}
}