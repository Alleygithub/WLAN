#ifndef TYPEDEF_H
#define TYPEDEF_H

#include <stdint.h>
#include <time.h>

typedef unsigned char byte;
typedef unsigned int word;

enum handle_res
{
    HANDLE_RES_SUCCESS = 0,
    HANDLE_RES_INVALID_PARAMETER = -1,
    HANDLE_RES_EXPIRED_PARAMETER = -2,
    HANDLE_RES_OPERATION_ERROR = -3,
    HANDLE_RES_NO_CHANGE = -4,
};

struct passwd
{
    uint64_t serial_number;
    byte password[32];
    time_t expired_time;
};

#endif // TYPEDEF_H
