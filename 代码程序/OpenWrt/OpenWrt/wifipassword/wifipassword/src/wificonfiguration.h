#ifndef WIFICONFIGURATION_H
#define WIFICONFIGURATION_H

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

class WiFiConfiguration
{
    struct InterfaceConfiguration
    {
        std::string interface_type;
        std::string interface_name;
        std::map<std::string, std::string> options;
    };

    std::vector<InterfaceConfiguration> configs;

public:
    int parse(const std::string& wifi_configuration_file_path);
    int print(const std::string& wifi_configuration_file_path);
    void parse(std::istream& wifi_configuration_stream);
    void print(std::ostream& wifi_configuration_stream);
    void get_wifi_password(const std::string& wifi_ssid, std::string& wifi_password);
    void set_wifi_password(const std::string& wifi_ssid, const std::string& wifi_password);
};

#endif // WIFICONFIGURATION_H
