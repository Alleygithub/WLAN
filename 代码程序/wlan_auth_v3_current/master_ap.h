#ifndef MASTER_AP_H
#define MASTER_AP_H

#include <map>
#include <string>
#include "typedef.h"

class master_ap
{
private:
    std::string interface_name; // 无线网卡名称。
    std::string specific_ssid; // 指定SSID。
    std::string password_file_path; // 口令文件路径。
    passwd current_password; // 当前口令。
    uint32_t update_interval; // 口令更新周期。
    byte physical_parameter[32]; // 当前物理认证参数。
    byte secret_key[32]; // 向从AP安全传输当前口令的密钥。

    handle_res init(const std::string& conf_file_path, std::map<std::string, std::string>& conf); // 解析配置文件。从初始口令文件中读取WLAN口令。
    void generate_physical_parameter(void); // 生成伪随机数作为物理认证参数。
    void calculate_password(void); // 计算新口令。
    handle_res update_password_file(void);// 更新初始口令文件。
    handle_res restart_hostapd(void); // 启动WLAN认证程序，发布物理认证参数至受控物理环境。
    handle_res transmit_new_password_to_slave_aps(void); // 向各从无线接入点传输新口令。

public:
    master_ap() {}

    handle_res main(std::string& conf_file_path); // 主程序。
};

#endif // MASTER_AP_H
