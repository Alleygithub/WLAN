/*
 * 解析口令参数，生成当天的口令，更新口令参数。
 */

#include <net/if.h>
#include "password_parameters_handler.h"

using namespace std;

password_parameters_handler::password_parameters_handler(uint32_t size, uint32_t interval)
{
    this->size = size;
    this->interval = interval;
}

int password_parameters_handler::parse_private_parameter(void)
{
    ifstream private_parameters_file;
    private_parameters_file.open(private_parameters_path.c_str());
    if (!private_parameters_file.is_open()) {
        return -1; // 配置文件打开失败。
    }

    string line, private_parameter;
    getline(private_parameters_file, line); // 读取位于第一行的逆向私有参数。
    stringstream first_line(line);
    first_line >> start_time >> private_parameter;
    if (start_time == 0 || base64_decode(private_parameter.data(), private_parameter.length(), backward_private_parameter) != 32) {
        private_parameters_file.close();
        return -2; // 参数非法。
    }

    while (private_parameters_file.peek() != EOF)
        getline(private_parameters_file, line); // 读取位于最后一行的正向私有参数。
    private_parameters_file.close();
    private_parameters_file.clear();
    stringstream last_line(line);
    last_line >> previous_time >> private_parameter;
    if (previous_time == 0 || base64_decode(private_parameter.data(), private_parameter.length(), previous_forward_private_parameter) != 32) {
        private_parameters_file.close();
        return -2; // 参数非法。
    }
    if (previous_time < start_time - interval || previous_time >= start_time + size * interval || (previous_time - start_time) % interval != 0) { // start_time = 0 previous_time = -1, 0, 1, ..., 364
        private_parameters_file.close();
        return -3; // 参数已过期或未生效。
    }

    private_parameters_file.close();
    return 0; // 解析完毕。
}

int password_parameters_handler::parse_public_parameter(void)
{
    current_time = start_time + (time(NULL) - start_time) / interval * interval; // 当前时间向前取整，前闭后开区间。
    if (current_time < start_time || current_time >= start_time + size * interval || current_time < previous_time) // start_time = 0 previous_time = -1, 0, 1, ..., 364 current_time = 0, 1, ..., 364
        return -3; // 参数已过期或未生效。

    ifstream public_parameters_file;
    public_parameters_file.open(public_parameters_path.c_str());
    if (!public_parameters_file.is_open())
        return -1; // 配置文件打开失败。

    string line;
    for (time_t i = start_time; i <= previous_time; i += interval) {
        if (public_parameters_file.peek() == EOF) {
            public_parameters_file.close();
            return -2; // 参数非法。
        }
        getline(public_parameters_file, line);
    }

    if (current_time >= previous_time + 2 * interval) {  // 之前连续多天没有更新正向私有参数，因此还要更新至昨天。
        ofstream private_parameters_file;
        private_parameters_file.open(private_parameters_path.c_str(), ios::app | ios::out);
        if (!private_parameters_file.is_open()) {
            public_parameters_file.close();
            return -1; // 配置文件打开失败。
        }

        string forward_private_parameter;
        while (current_time >= previous_time + 2 * interval) {
            if (public_parameters_file.peek() == EOF) {
                public_parameters_file.close();
                private_parameters_file.close();
                return -2; // 参数非法。
            }
            getline(public_parameters_file, line);
            if (base64_decode(line.data(), line.length(), public_parameter) != 32) {
                public_parameters_file.close();
                private_parameters_file.close();
                return -2; // 参数非法。
            }
            for (int i = 0; i < 32; i++)
                previous_forward_private_parameter[i] ^= public_parameter[i];
            sm3_hash(previous_forward_private_parameter, 32, current_forward_private_parameter);
            memcpy(previous_forward_private_parameter, current_forward_private_parameter, 32);
            previous_time += interval;
            base64_encode(previous_forward_private_parameter, 32, forward_private_parameter);
            private_parameters_file << "\r\n" << previous_time << " " << forward_private_parameter;
        }

        private_parameters_file.flush();
        private_parameters_file.close();
    }

    if (public_parameters_file.peek() == EOF) {
        public_parameters_file.close();
        return -2; // 参数非法。
    }
    getline(public_parameters_file, line);
    if (base64_decode(line.data(), line.length(), public_parameter) != 32) {
        public_parameters_file.close();
        return -2; // 参数非法。
    }

    public_parameters_file.close();
    return 0;
}

int password_parameters_handler::capture_public_parameter(const string& specific_ssid)
{
    current_time = 0;
    current_time = start_time + (time(NULL) - start_time) / interval * interval; // 当前时间向前取整，前开后闭区间。
    for (int i = 0; i < 32; i++)
        public_parameter[i] = 0;

    int ret = capture_public_parameter_and_system_time_from_80211_management_frame(interface_name.c_str(), specific_ssid.c_str(), public_parameter, current_time);
    if (ret != 0 || current_time == 0)
        return -4; // 无法捕捉到公开参数。

    if ((current_time - start_time) % interval != 0 || current_time - time(NULL) > 600 || current_time - time(NULL) < -600) // 移动终端和无线接入点的系统时间的间隔不能过大。
        return -2; // 参数非法。
    if (current_time < start_time || current_time >= start_time + size * interval || current_time < previous_time || current_time >= previous_time + 2 * interval) // start_time = 0 previous_time = -1, 0, 1, ..., 364 current_time = 0, 1, ..., 364
        return -3; // 参数已过期或未生效。

    return 0;
}

void password_parameters_handler::calculate_password(byte password[32])
{
    if (current_time == previous_time)
        return; // 当天口令已生成。

    for (int i = 0; i < 32; i++)
        previous_forward_private_parameter[i] ^= public_parameter[i];
    sm3_hash(previous_forward_private_parameter, 32, current_forward_private_parameter); // 计算正向私有参数。

    int hash_times = size - (current_time - start_time) / interval;
    byte old_backward_private_parameter[32], new_backward_private_parameter[32];
    memcpy(old_backward_private_parameter, backward_private_parameter, 32);
    for (int i = 0; i < hash_times; i++) {
        sm3_hash(old_backward_private_parameter, 32, new_backward_private_parameter); // 计算反向私有参数。
        memcpy(old_backward_private_parameter, new_backward_private_parameter, 32);
    }

    for (int i = 0; i < 32; i++)
        password[i] = current_forward_private_parameter[i] ^ new_backward_private_parameter[i]; // 计算口令。
    return; // 更新完毕。
}

int password_parameters_handler::authenticator_init(const string& private_parameters_path, const string& public_parameters_path)
{
    if (private_parameters_path.empty() || public_parameters_path.empty())
        return -2; // 参数非法。

    this->private_parameters_path = private_parameters_path;
    int ret = parse_private_parameter();
    if (ret != 0)
        return ret;

    this->public_parameters_path = public_parameters_path;
    ifstream public_parameters_file;
    public_parameters_file.open(public_parameters_path.c_str());
    if (!public_parameters_file.is_open())
        return -1; // 配置文件打开失败。

    string line;
    for (int i = 0; i < size; i++) {
        if (public_parameters_file.peek() == EOF) {
            public_parameters_file.close();
            return -2; // 参数非法。
        }
        getline(public_parameters_file, line);
    }

    return 0;
}

int password_parameters_handler::supplicant_init(const string& private_parameters_path, const string& interface_name)
{
    if (private_parameters_path.empty() || interface_name.empty())
        return -2; // 参数非法。

    this->private_parameters_path = private_parameters_path;
    int ret = parse_private_parameter();
    if (ret != 0)
        return ret;

    signed long long devidx = if_nametoindex(interface_name.c_str());
    if (devidx == 0)
        return -2; // 参数非法。
    this->interface_name = interface_name;

    return 0;
}

int password_parameters_handler::get_password(const string& specific_ssid, string& password)
{
    if (interface_name.empty() || specific_ssid.empty())
        return -2; // 参数非法。

    int ret = capture_public_parameter(specific_ssid);
    if (ret != 0)
        return ret;

    byte tmp_password[32];
    calculate_password(tmp_password);
    if (current_time == previous_time)
        return 1; // 口令已生成。
    password.clear();
    base64_encode(tmp_password, 32, password);
    return 0;
}

int password_parameters_handler::get_password(string& password, time_t& current_time, byte public_parameter[32])
{
    if (public_parameters_path.empty())
        return -2; // 参数非法。

    int ret = parse_public_parameter();
    if (ret != 0)
        return ret;

    current_time = this->current_time;
    for (int i = 0; i < 32; i++)
        public_parameter[i] = this->public_parameter[i];

    byte tmp_password[32];
    calculate_password(tmp_password);
    if (this->current_time == previous_time)
        return 1; // 口令已生成。
    password.clear();
    base64_encode(tmp_password, 32, password);
    return 0;
}

int password_parameters_handler::update_forward_private_parameter(void)
{
    if (private_parameters_path.empty())
        return -2; // 参数非法。

    ofstream private_parameters_file;
    private_parameters_file.open(private_parameters_path.c_str(), ios::app | ios::out);
    if (!private_parameters_file.is_open())
        return -1; // 配置文件打开失败。

    string forward_private_parameter;
    memcpy(previous_forward_private_parameter, current_forward_private_parameter, 32);
    previous_time += interval;
    base64_encode(previous_forward_private_parameter, 32, forward_private_parameter);
    private_parameters_file << "\r\n" << previous_time << " " << forward_private_parameter;

    private_parameters_file.flush();
    private_parameters_file.close();
    return 0;
}
