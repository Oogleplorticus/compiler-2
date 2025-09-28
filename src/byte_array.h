#pragma once

#include <stddef.h>

//reccomend to only have one copy of a byte array at a time
struct ByteArray {
	char* ptr;
	size_t length;
};

struct ByteArray allocByteArray(size_t length);
void freeByteArray(struct ByteArray* byteArray); //nulls the pointer and zeroes the length

//will truncate if smaller
void resizeByteArray(struct ByteArray* byteArray, size_t newLength);

//does not free the original arrays
struct ByteArray combineByteArrays(struct ByteArray first, struct ByteArray second);