#pragma once

#include <stdio.h>

#include "token.h"

//MUST be called before using other functions
void resetTokeniser(FILE* filePtr);

void incrementToken();
struct Token currentToken();
struct Token nextToken();