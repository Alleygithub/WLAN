#include "passwordmaintaince.h"

using namespace std;

PasswordMaintaince::PasswordMaintaince(uint32_t size, uint32_t interval, string private_parameters_path, string public_parameters_path, string wifi_configuration_path, string ssid)
{
    this->size = size;
    this->interval = interval;
    this->private_parameters_path = private_parameters_path;
    this->public_parameters_path = public_parameters_path;
    this->wifi_configuration_path = wifi_configuration_path;
    this->ssid = ssid;
}

int PasswordMaintaince::init()
{
    fstream private_parameters_file;
    private_parameters_file.open(private_parameters_path.c_str(), ios::in);
    if (!private_parameters_file.is_open()) {
        return -1; // 配置文件打开失败。
    }

    string line, private_parameter;
    getline(private_parameters_file, line); // 读取位于第一行的逆向私有参数。
    stringstream first_line(line);
    first_line >> start_time >> private_parameter;
    if (start_time == 0 || base64_decode(private_parameter.data(), private_parameter.length(), backward_private_parameter) != 32) {
        return -2; // 参数非法。
    }

    while (private_parameters_file.peek() != EOF)
        getline(private_parameters_file, line); // 读取位于最后一行的正向私有参数。
    private_parameters_file.close();
    private_parameters_file.clear();
    stringstream last_line(line);
    last_line >> previous_time >> private_parameter;
    if (previous_time == 0 || base64_decode(private_parameter.data(), private_parameter.length(), previous_forward_private_parameter) != 32) {
        return -2; // 参数非法。
    }
    if (previous_time < start_time || previous_time >= start_time + size * interval || (previous_time - start_time) % interval != 0)
        return -2; // 参数非法。

    int ret = wifi_configuration.parse(wifi_configuration_path);
    if (ret != 0)
        return ret; // 口令配置文件解析失败。

   string wifi_password;
   wifi_configuration.get_wifi_password(ssid, wifi_password);
   if (wifi_password.empty()) {
       return -3; // SSID不存在。
   }

   return 0; // 初始化完毕。
}

int PasswordMaintaince::update_wifi_password(time_t now) {
    if (now < start_time + interval || now >= start_time + (size + 1) * interval || now < previous_time) {
        return -3; // 参数已过期或未生效。
    }
    else if (now < previous_time + interval) {
        return 0; // 口令已更新。
    }
    else if (now >= previous_time + (interval << 1)) { // 之前连续多天没有更新正向私有参数，因此还要更新至昨天。
        ofstream private_parameters_file;
        private_parameters_file.open(private_parameters_path.c_str(), ios::app | ios::out);
        if (!private_parameters_file.is_open()) {
            return -1; // 配置文件打开失败。
        }
        byte new_forward_private_parameter[32];
        string forward_private_parameter;
        while (now >= previous_time + (interval << 1)) {
            sm3_hash(previous_forward_private_parameter, 32, new_forward_private_parameter); // 更新正向私有参数，后续还要加上公开参数的影响。
            previous_time += interval;
            memcpy(previous_forward_private_parameter, new_forward_private_parameter, 32);
            base64_encode(previous_forward_private_parameter, 32, forward_private_parameter);
            private_parameters_file << "\r\n" << previous_time << " " << forward_private_parameter;
        }
        private_parameters_file.flush();
        private_parameters_file.close();
        private_parameters_file.clear();
    }

    byte new_forward_private_parameter[32];
    sm3_hash(previous_forward_private_parameter, 32, new_forward_private_parameter); // 更新正向私有参数，后续还要加上公开参数的影响。

    int hash_times = size - (now - start_time) / interval;
    byte old_backward_private_parameter[32], new_backward_private_parameter[32];
    memcpy(old_backward_private_parameter, backward_private_parameter, 32);
    for (int i = 0; i < hash_times; i++) {
        sm3_hash(old_backward_private_parameter, 32, new_backward_private_parameter); // 计算反向私有参数。
        memcpy(old_backward_private_parameter, new_backward_private_parameter, 32);
    }

    byte new_wifi_password[32];
    for (int i = 0; i < 32; i++) {
        new_wifi_password[i] = new_backward_private_parameter[i] ^ new_forward_private_parameter[i]; // 计算口令。
    }

    string wifi_password;
    base64_encode(new_wifi_password, 32, wifi_password);
    wifi_configuration.set_wifi_password(ssid, wifi_password); // 更新口令。
    int ret = wifi_configuration.print(wifi_configuration_path);
    if (ret != 0)
        return ret;

    ofstream private_parameters_file;
    private_parameters_file.open(private_parameters_path.c_str(), ios::app | ios::out);
    if (!private_parameters_file.is_open()) {
        return -1; // 配置文件打开失败。
    }
    previous_time += interval;
    memcpy(previous_forward_private_parameter, new_forward_private_parameter, 32);
    string forward_private_parameter;
    base64_encode(previous_forward_private_parameter, 32, forward_private_parameter);
    private_parameters_file << "\r\n" << previous_time << " " << forward_private_parameter;
    private_parameters_file.flush();
    private_parameters_file.close();
    private_parameters_file.clear();

    return 0; // 更新完毕。
}
