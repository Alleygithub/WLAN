#ifndef UTILS_H
#define UTILS_H

#include <string>
#include "typedef.h"

void sm3_hash(const byte message[], size_t length, byte digest[]);

void byte_to_hex_string(const byte input[], size_t length, char output[]);

size_t hex_string_to_byte(const char input[], size_t length, byte output[]);

#endif // UTILS_H
