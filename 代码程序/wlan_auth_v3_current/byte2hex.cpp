#include "utils.h"

void byte_to_hex_string(const byte input[], size_t length, char output[])
{
    for (size_t i = 0; i < length; i++) {
        if ((input[i] >> 4) > 9)
            output[2 * i] = 'a' + (input[i] >> 4) - 10;
        else
            output[2 * i] = '0' + (input[i] >> 4);
        if ((input[i] & 0xF) > 9)
            output[2 * i + 1] = 'a' + (input[i] & 0xF) - 10;
        else
            output[2 * i + 1] = '0' + (input[i] & 0xF);
    }
    output[2 * length] = '\0';
    return;
}

size_t hex_string_to_byte(const char input[], size_t length, byte output[])
{
    for (size_t i = 0; i < length; i += 2) {
        if (input[i] >= 'a' && input[i] <= 'f')
            output[i / 2] = input[i] - 'a' + 10;
        else if (input[i] >= 'A' && input[i] <= 'F')
            output[i / 2] = input[i] - 'A' + 10;
        else if (input[i] >= '0' && input[i] <= '9')
            output[i / 2] = input[i] - '0';
        else
            return i / 2;
        output[i / 2] <<= 4;
        if (input[i + 1] >= 'a' && input[i + 1] <= 'f')
            output[i / 2] |= input[i + 1] - 'a' + 10;
        else if (input[i + 1] >= 'A' && input[i + 1] <= 'F')
            output[i / 2] |= input[i + 1] - 'A' + 10;
        else if (input[i + 1] >= '0' && input[i + 1] <= '9')
            output[i / 2] |= input[i + 1] - '0';
        else
            return i / 2;
    }
    return length / 2;
}
