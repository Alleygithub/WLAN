#ifndef BASE64_H
#define BASE64_H

#include <string>
#include "typedef.h"

void base64_encode(const byte input[], size_t length, std::string& output);

size_t base64_decode(const char input[], size_t input_length, byte output[]);

#endif // BASE64_H
