#ifndef UTILS_H
#define UTILS_H

#include <string>
#include "typedef.h"

void base64_encode(const byte input[], size_t length, std::string& output);

size_t base64_decode(const char input[], size_t input_length, byte output[]);

void sm3_hash(const byte message[], size_t length, byte digest[]);

void sm4_encrypt(const word plaintext[], const word key[], word ciphertext[]);

void sm4_decrypt(const word ciphertext[], const word key[], word plaintext[]);

void byte_to_hex_string(byte input[], size_t length, char output[]);

#endif // UTILS_H
