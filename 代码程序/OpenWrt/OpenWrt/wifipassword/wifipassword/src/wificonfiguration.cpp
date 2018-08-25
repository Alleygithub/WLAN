#include "wificonfiguration.h"

using namespace std;

int WiFiConfiguration::parse(const string& wifi_configuration_file_path)
{
    ifstream wifi_configuration_file;
    wifi_configuration_file.open(wifi_configuration_file_path.c_str());
    if (!wifi_configuration_file.is_open()) {
        return -1;
    }
    parse(wifi_configuration_file);
    wifi_configuration_file.close();
    return 0;
}

int WiFiConfiguration::print(const string& wifi_configuration_file_path)
{
    ofstream wifi_configuration_file;
    wifi_configuration_file.open(wifi_configuration_file_path.c_str());
    if (!wifi_configuration_file.is_open()) {
        return -1;
    }
    print(wifi_configuration_file);
    wifi_configuration_file.close();
    return 0;
}

void WiFiConfiguration::parse(istream& wifi_configuration_stream)
{
    string line;
    while (getline(wifi_configuration_stream, line))
    {
        stringstream ss(line);
        string head, key, value;
        ss >> head >> key >> value;
        if (head == "config")
        {
            InterfaceConfiguration config;
            config.interface_type = key;
            config.interface_name = value;
            configs.push_back(config);
        }
        else if (head == "option")
        {
            InterfaceConfiguration& config = configs.back();
            config.options[key] = value;
        }
    }
    return;
}

void WiFiConfiguration::print(ostream& wifi_configuration_stream)
{
    wifi_configuration_stream << endl;
    for (vector<InterfaceConfiguration>::iterator config = configs.begin(); config != configs.end(); config++)
    {
        wifi_configuration_stream << "config " << config->interface_type << " " << config->interface_name << endl;

        map<string, string>& options = config->options;
        for (map<string, string>::iterator option = options.begin(); option != options.end(); option++)
        {
            wifi_configuration_stream << "\toption " << option->first << " " << option->second << endl;
        }

        wifi_configuration_stream << endl;
    }
    return;
}

void WiFiConfiguration::set_wifi_password(const string& wifi_ssid, const string& wifi_password)
{
    for (vector<InterfaceConfiguration>::iterator config = configs.begin(); config != configs.end(); config++)
    {
        map<string, string>& options = config->options;
        if (options.count("ssid") != 0 && options["ssid"] == "'" + wifi_ssid +"'" && options.count("key") != 0)
        {
            options["key"] = "'" + wifi_password + "'";
            break;
        }
    }
    return;
}

void WiFiConfiguration::get_wifi_password(const std::string& wifi_ssid, string& wifi_password)
{
    for (vector<InterfaceConfiguration>::iterator config = configs.begin(); config != configs.end(); config++)
    {
        map<string, string>& options = config->options;
        if (options.count("ssid") != 0 && options["ssid"] == "'" + wifi_ssid +"'" && options.count("key") != 0)
        {
            wifi_password = options["key"];
            wifi_password.erase(0, 1);
            wifi_password.erase(wifi_password.length() - 1);
            break;
        }
    }
    return;
}
