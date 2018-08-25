#include "base64.h"

using namespace std;

inline byte decode_get_byte(char ch)
{
    if(ch == '+')
        return 62;
    else if(ch == '/')
        return 63;
    else if(ch <= '9')
        return (byte)(ch - '0' + 52);
    else if(ch == '=')
        return 64;
    else if(ch <= 'Z')
        return (byte)(ch - 'A');
    else if(ch <= 'z')
        return (byte)(ch - 'a' + 26);
    else
        return 64;
}

inline char encode_get_char(byte bt)
{
        return "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[bt & 0x3F];
}

void base64_encode(const byte input[], size_t length, std::string& output)
{
    output.clear();

    for (size_t i = 0; i < (length / 3) * 3; i += 3)
    {
        output += encode_get_char(input[i] >> 2);
        output += encode_get_char(((input[i] & 0x03) << 4) | (input[i + 1] >> 4));
        output += encode_get_char(((input[i + 1] & 0x0F) << 2) | (input[i + 2] >> 6));
        output += encode_get_char(input[i + 2] & 0x3F);
    }

    if (length % 3 == 2)
    {
        output += encode_get_char(input[length - 2] >> 2);
        output += encode_get_char(((input[length - 2] & 0x03) << 4) | (input[length - 1] >> 4));
        output += encode_get_char(((input[length - 1] & 0x0F) << 2));
        output += '=';
    }
    else if (length % 3 == 1)
    {
        output += encode_get_char(input[length - 1] >> 2);
        output += encode_get_char(((input[length - 1] & 0x03) << 4));
        output += '=';
        output += '=';
    }

    return;
}

size_t base64_decode(const char input[], size_t input_length, byte output[])
{
    if (input_length % 4 != 0)
        return 0;

    if (input[input_length - 1] == '=')
        if (input[--input_length - 1] == '=')
            --input_length;

    size_t output_lenth = 0;

    for (size_t i = 0; i < input_length; i += 4) {
        byte buffer[4] = {64, 64, 64, 64};
        for (int j = 0; j < 4 && i + j < input_length; j++) {
            buffer[j] = decode_get_byte(input[i + j]);
            if (buffer[j] == 64)
                return 0;
        }

        output[output_lenth++] = (buffer[0] << 2) | (buffer[1] >> 4);
        if (buffer[2] != 64)
            output[output_lenth++] = ((buffer[1] & 0x0F) << 4) | (buffer[2] >> 2);
        else if ((buffer[1] & 0x0F) != 0)
            return 0;
        if (buffer[3] != 64)
            output[output_lenth++] = ((buffer[2] & 0x03) << 6) | buffer[3];
        else if ((buffer[2] & 0x03) != 0)
            return 0;
    }

    return output_lenth;
}
