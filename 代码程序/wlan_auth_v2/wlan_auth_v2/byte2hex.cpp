#include "utils.h"

void byte_to_hex_string(byte input[], size_t length, char output[])
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
