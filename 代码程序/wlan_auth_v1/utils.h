#ifndef UTILS_H
#define UTILS_H

#include <string>
#include "typedef.h"

void base64_encode(const byte input[], size_t length, std::string& output);

size_t base64_decode(const char input[], size_t input_length, byte output[]);

void sm3_hash(const byte message[], size_t length, byte digest[]);

int capture_public_parameter_and_system_time_from_80211_management_frame(const char* ifname, const char* specific_ssid, byte public_parameter[], time_t& system_time);

#endif // UTILS_H
