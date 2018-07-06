#ifndef CAPTURE_H
#define CAPTURE_H

#include <time.h>
#include "typedef.h"

int capture_physical_parameter_and_system_time_from_80211_management_frame(const char* ifname, const char* specific_ssid, int& capture_res, byte physical_param[], time_t& ap_cur_time);

#endif // CAPTURE_H
