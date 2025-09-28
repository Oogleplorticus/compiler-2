#include "byte_array.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

struct ByteArray allocByteArray(size_t length) {
	void* ptr = calloc(length, sizeof(char));
	if (ptr == NULL) {
		fprintf(stderr, "ERROR: Could not allocate memory for byte array!\n");
		exit(1);
	}

	struct ByteArray byteArray;
	byteArray.length = length;
	byteArray.ptr = ptr;

	return byteArray;
}

void freeByteArray(struct ByteArray* byteArray) {
	if (byteArray->ptr == NULL) {
		byteArray->length = 0;
		return;
	}

	free(byteArray->ptr);
	byteArray->ptr = NULL;
	byteArray->length = 0;
}

void resizeByteArray(struct ByteArray* byteArray, size_t newLength) {
	if (byteArray->ptr == NULL) {
		fprintf(stderr, "ERROR: Attempted to resize empty byte array!");
		exit(1);
	}

	struct ByteArray resized = allocByteArray(newLength);
	memcpy(resized.ptr, byteArray->ptr, resized.length);
	free(byteArray->ptr);
	*byteArray = resized;
}

struct ByteArray combineByteArrays(struct ByteArray first, struct ByteArray second) {
	if (first.ptr == NULL && second.ptr == NULL) {
		first.length = 0;
		return first;
	}
	if (first.ptr == NULL) {
		struct ByteArray copy = allocByteArray(second.length);
		memcpy(copy.ptr, second.ptr, second.length);
		return copy;
	}
	if (second.ptr == NULL) {
		struct ByteArray copy = allocByteArray(first.length);
		memcpy(copy.ptr, first.ptr, first.length);
		return copy;
	}

	size_t combinedLength = first.length + second.length;
	struct ByteArray merged = allocByteArray(combinedLength);

	memcpy(merged.ptr, first.ptr, first.length);
	memcpy(merged.ptr + first.length, second.ptr, second.length);

	return merged;
}