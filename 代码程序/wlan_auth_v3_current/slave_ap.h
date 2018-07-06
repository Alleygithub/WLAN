#ifndef SLAVE_AP_H
#define SLAVE_AP_H

#include <map>
#include <netinet/in.h>
#include <string>
#include "typedef.h"

class slave_ap
{
private:
    std::string interface_name; // 无线网卡名称。
    std::string specific_ssid; // 指定SSID。
    passwd current_password; // 当前口令。
    sockaddr_in master_ap_addr; // 主AP的IP地址。
    byte secret_key[32]; // 从主AP安全传输当前口令的密钥。

    handle_res init(const std::string& conf_file_path, std::map<std::string, std::string>& conf); // 解析配置文件。
    handle_res get_current_password(void); // 获取当前口令。
    handle_res restart_hostapd(void); // 启动WLAN认证程序。

public:
    slave_ap() {}

    handle_res main(std::string& conf_file_path); // 主程序。
};

#endif // SLAVE_AP_H
