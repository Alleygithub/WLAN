#ifndef SM3_H
#define SM3_H

#include <stddef.h>
#include "typedef.h"

void sm3_hash(const byte message[], size_t length, byte digest[]);

#endif // SM3_H
