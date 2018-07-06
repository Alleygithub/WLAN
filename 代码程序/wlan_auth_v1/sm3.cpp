/*
 *  SM3 Hash
 */

#include "typedef.h"
#include <stddef.h>

#include "utils.h"

inline word rotate_left(word x, unsigned int y)
{
    return ((x << (y & (32 - 1))) | (x >> (32 - (y & (32 - 1)))));
}

void message_expand(word W[132], const word BB[16]) // 消息扩展
{
    for (int j = 0; j < 16; j++)
        W[j] = BB[j];
    for (int j = 16; j < 68; j++) {
        W[j] = W[j - 16] ^ W[j - 9] ^ rotate_left(W[j - 3], 15);
        W[j] ^= rotate_left(W[j], 15) ^ rotate_left(W[j], 23);
        W[j] ^= rotate_left(W[j - 13], 7) ^ W[j - 6];
    }
    for (int j = 0; j < 64; j++)
        W[j + 68] = W[j] ^ W[j + 4];

    return;
}

void compression_function(word V[8], const byte message[64]) // 压缩函数
{
    word A = V[0];
    word B = V[1];
    word C = V[2];
    word D = V[3];
    word E = V[4];
    word F = V[5];
    word G = V[6];
    word H = V[7];

    word BB[16];
    for (int i = 0; i < 16; i++)
        BB[i] = (message[4 * i] << 24) | (message[4 * i + 1] << 16) | (message[4 * i + 2]) << 8 | (message[4 * i + 3]);
    word W[132];
    message_expand(W, BB); // 消息扩展

    word SS1, SS2, TT1, TT2;
    for (int j = 0; j < 16; j++) {
        SS1 = rotate_left(rotate_left(A, 12) + E + rotate_left(0x79CC4519, j), 7);
        SS2 = SS1 ^ rotate_left(A, 12);
        TT1 = ((A ^ B ^ C) + D + SS2) + W[j + 68];
        TT2 = ((E ^ F ^ G) + H + SS1) + W[j];
        D = C;
        C = rotate_left(B, 9);
        B = A;
        A = TT1;
        H = G;
        G = rotate_left(F, 19);
        F = E;
        E = TT2 ^ rotate_left(TT2, 9) ^ rotate_left(TT2, 17);
    }
    for (int j = 16; j < 64; j++) {
        SS1 = rotate_left(rotate_left(A, 12) + E + rotate_left(0x7A879D8A, j), 7);
        SS2 = SS1 ^ rotate_left(A, 12);
        TT1 = (((A & B) | (A & C) | (B & C)) + D + SS2) + W[j + 68];
        TT2 = (((E & F) | ((~E) & G)) + H + SS1) + W[j];
        D = C;
        C = rotate_left(B, 9);
        B = A;
        A = TT1;
        H = G;
        G = rotate_left(F, 19);
        F = E;
        E = TT2 ^ rotate_left(TT2, 9) ^ rotate_left(TT2, 17);
    }

    V[0] ^= A;
    V[1] ^= B;
    V[2] ^= C;
    V[3] ^= D;
    V[4] ^= E;
    V[5] ^= F;
    V[6] ^= G;
    V[7] ^= H;

    return;
}

size_t padding(byte padding_message[128], const byte message[], size_t length) // 填充
{
    for (int i = 0; i < 128; i++)
        padding_message[i] = 0;

    size_t start_index = length >> 6 << 6;
    size_t remained_bytes = length & 0x3F;
    for (size_t i = 0; i < remained_bytes; i++)
        padding_message[i] = message[start_index + i];

    padding_message[remained_bytes] = 0x80;

    size_t padding_message_length = remained_bytes < 56 ? 64 : 128;
    size_t bit_length = length << 3;
    for (int i = 1; i <= 8; i++) {
        padding_message[padding_message_length - i] = (byte)bit_length;
        bit_length >>= 8;
    }

    return padding_message_length;
}

void sm3_hash(const byte message[], size_t length, byte digest[])
{
    word V[8] = {
        0x7380166F,
        0x4914B2B9,
        0x172442D7,
        0xDA8A0600,
        0xA96F30BC,
        0x163138AA,
        0xE38DEE4D,
        0xB0FB0E4E
    };


    // 按512比特进行分组
    for (size_t i = 0; i < (length >> 6); i++)
        compression_function(V, message + i * 64); // 压缩函数

    byte padding_message[128];
    if (padding(padding_message, message, length) == 64) {
        compression_function(V, padding_message);
    }
    else {
        compression_function(V, padding_message);
        compression_function(V, padding_message);
    }

    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 4; j++)
            digest[i * 4 + j] = (byte)(V[i] >> (24 - j * 8));

    return;
}
