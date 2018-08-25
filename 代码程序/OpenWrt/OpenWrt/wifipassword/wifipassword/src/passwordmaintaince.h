#ifndef PASSWORDMAINTAINCE_H
#define PASSWORDMAINTAINCE_H

#include <string.h>
#include <inttypes.h>
#include "base64.h"
#include "sm3.h"
#include "wificonfiguration.h"

class PasswordMaintaince
{
public:
    std::string private_parameters_path;
    std::string public_parameters_path;
    std::string wifi_configuration_path;
    std::string ssid;

    uint32_t size;
    uint32_t interval;

    time_t previous_time;
    byte previous_forward_private_parameter[32];
    time_t start_time;
    byte backward_private_parameter[32];
    WiFiConfiguration wifi_configuration;

public:
    PasswordMaintaince(uint32_t size = 365, uint32_t interval = 86400, std::string private_parameters_path = "/etc/config/private_parameters", std::string public_parameters_path = "/etc/config/public_parameters", std::string wifi_configuration_path = "/etc/config/wireless", std::string ssid = "");
    int init();
    int update_wifi_password(time_t now);
};

#endif // PASSWORDMAINTAINCE_H
