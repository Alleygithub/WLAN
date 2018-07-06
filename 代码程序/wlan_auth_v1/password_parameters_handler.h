#ifndef PASSWORD_PARAMETERS_HANDLER_H
#define PASSWORD_PARAMETERS_HANDLER_H

#include <fstream>
#include <sstream>
#include <string.h>
#include <time.h>
#include "utils.h"

class password_parameters_handler
{
private:
    uint32_t size;
    uint32_t interval;

    std::string private_parameters_path;
    std::string public_parameters_path;
    std::string interface_name;

    time_t start_time;
    byte backward_private_parameter[32];
    time_t previous_time;
    byte previous_forward_private_parameter[32];
    byte current_forward_private_parameter[32];
    time_t current_time;
    byte public_parameter[32];

    int parse_private_parameter(void);
    int parse_public_parameter(void);
    int capture_public_parameter(const std::string& specific_ssid);
    void calculate_password(byte password[32]);

public:
    password_parameters_handler(uint32_t size = 365, uint32_t interval = 86400);

    int authenticator_init(const std::string& private_parameters_path, const std::string& public_parameters_path);
    int supplicant_init(const std::string& private_parameters_path, const std::string& interface_name);
    int get_password(const std::string& specific_ssid, std::string& password);
    int get_password(std::string& password, time_t& current_time, byte public_parameter[32]);
    int update_forward_private_parameter(void);
};

#endif // PASSWORD_PARAMETERS_HANDLER_H
