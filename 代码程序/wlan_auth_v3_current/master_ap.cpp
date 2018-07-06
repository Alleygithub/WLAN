#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include "utils.h"

#include <sys/timeb.h>

#include "master_ap.h"

using namespace std;

handle_res master_ap::init(const string& conf_file_path, map<string, string>& conf) // 解析配置文件。从初始口令文件中读取WLAN口令。
{
    if (conf_file_path.empty())
        return HANDLE_RES_INVALID_PARAMETER;
    ifstream conf_file;
    conf_file.open(conf_file_path.c_str());
    if (!conf_file.is_open())
        return HANDLE_RES_INVALID_PARAMETER;
    string line;
    while (getline(conf_file, line)) {
        if (line.empty() || (line.length() == 1 && line[0] == '\r'))
            continue;
        size_t separator_idx = line.find(':', 0);
        if (separator_idx == string::npos || line.find(':', separator_idx + 1) != string::npos) {
            conf_file.close();
            return HANDLE_RES_INVALID_PARAMETER;
        }
        conf[line.substr(0, separator_idx)] = line.substr(separator_idx + 1, line.length() - separator_idx - 1); // 拆分为键值对。
    }
    conf_file.close();

    if (conf.count("interface_name") == 0 || conf["interface_name"].empty() || conf.count("specific_ssid") == 0 || conf["specific_ssid"].empty() || conf.count("password_file_path") == 0 || conf["password_file_path"].empty() || conf.count("update_interval") == 0 || conf.count("secret_key") == 0 || conf["secret_key"].empty())
        return HANDLE_RES_INVALID_PARAMETER;
    specific_ssid = conf["specific_ssid"];
    interface_name = conf["interface_name"];
    password_file_path = conf["password_file_path"];
    ifstream pwd_file;
    pwd_file.open(password_file_path.c_str());
    if (!pwd_file.is_open())
        return HANDLE_RES_OPERATION_ERROR;
    if (!getline(pwd_file, line))
        return HANDLE_RES_OPERATION_ERROR;
    stringstream first_line(line);
    first_line >> current_password.serial_number; // 第一行：序列号。
    if (current_password.serial_number < 0) {
        pwd_file.close();
        return HANDLE_RES_OPERATION_ERROR;
    }
    if (!getline(pwd_file, line))
        return HANDLE_RES_OPERATION_ERROR;
    if (hex_string_to_byte(line.data(), line.length(), current_password.password) != 32) { // 第二行：当前口令。
        pwd_file.close();
        return HANDLE_RES_OPERATION_ERROR;
    }
    if (!getline(pwd_file, line))
        return HANDLE_RES_OPERATION_ERROR;
    stringstream third_line(line);
    third_line >> current_password.expired_time; // 第三行：失效时间。
    if (!getline(pwd_file, line)) {
        pwd_file.close();
        return HANDLE_RES_OPERATION_ERROR;
    }
    if (hex_string_to_byte(line.data(), line.length(), physical_parameter) != 32) { // 第四行：当前物理认证参数。
        pwd_file.close();
        return HANDLE_RES_OPERATION_ERROR;
    }
    pwd_file.close();
    stringstream interval(conf["update_interval"]);
    interval >> update_interval;
    if (update_interval <= 0)
        return HANDLE_RES_INVALID_PARAMETER;
    if (hex_string_to_byte(conf["secret_key"].c_str(), conf["secret_key"].length(), secret_key) != 32)
        return HANDLE_RES_INVALID_PARAMETER;

    return HANDLE_RES_SUCCESS;
}

void master_ap::generate_physical_parameter(void) // 生成伪随机数作为物理认证参数。
{
    srand(static_cast<unsigned int>(time(NULL)));
    for (int i = 0; i < 8; i++) { // 生成32字节随机数。
        int r = rand();
        memcpy(physical_parameter + i * 4, &r, 4);
    }
    return;
}

void master_ap::calculate_password(void) // 计算新口令。
{
    current_password.serial_number++;
    for (int i = 0; i < 32; i++) {
        current_password.password[i] ^= physical_parameter[i];
    }
    byte new_pwd[32];
    sm3_hash(current_password.password, 32, new_pwd);
    memcpy(current_password.password, new_pwd, 32);
    current_password.expired_time += update_interval;
    return;
}

handle_res master_ap::update_password_file(void) // 更新初始口令文件。
{
    if (password_file_path.empty())
        return HANDLE_RES_OPERATION_ERROR;

    ofstream pwd_file;
    pwd_file.open(password_file_path.c_str());
    if (!pwd_file.is_open()) {
        return HANDLE_RES_OPERATION_ERROR;
    }

    pwd_file << current_password.serial_number << endl; // 第一行：序列号。
    char buff[65] = { '\0' };
    byte_to_hex_string(current_password.password, 32, buff);
    pwd_file << buff << endl; // 第二行：当前口令。
    pwd_file << current_password.expired_time << endl; // 第三行：失效时间。
    byte_to_hex_string(physical_parameter, 32, buff);
    buff[65] = '\0';
    pwd_file << buff << endl; // 第四行：当前物理认证参数。
    pwd_file.flush();
    pwd_file.close();

    return HANDLE_RES_SUCCESS;
}

handle_res master_ap::restart_hostapd(void) // 启动WLAN认证程序，发布物理认证参数至受控物理环境。
{
    ofstream hostapd_conf_file;
    hostapd_conf_file.open("hostapd.conf");
    if (!hostapd_conf_file.is_open())
        return HANDLE_RES_OPERATION_ERROR;
    hostapd_conf_file << "interface=" << interface_name << endl;
    hostapd_conf_file << "driver=nl80211" << endl;
    hostapd_conf_file << "ssid=" << specific_ssid << endl;
    hostapd_conf_file << "hw_mode=g" << endl;
    hostapd_conf_file << "channel=1" << endl;
    hostapd_conf_file << "macaddr_acl=0" << endl;
    char sn[17] = {'\0'};
    byte_to_hex_string(reinterpret_cast<byte*>(&current_password.serial_number), 8, sn);
    char physical_param[65] = {'\0'};
    byte_to_hex_string(physical_parameter, 32, physical_param);
    hostapd_conf_file << "vendor_elements=dd0c54386201" << sn << "dd2454386202" << physical_param << endl;
    hostapd_conf_file << "auth_algs=1" << endl;
    hostapd_conf_file << "wpa=2" << endl;
    char psk[65] = {'\0'};
    byte_to_hex_string(current_password.password, 32, psk);
    hostapd_conf_file << "wpa_psk=" << psk << endl;
    hostapd_conf_file << "wpa_key_mgmt=WPA-PSK" << endl;
    hostapd_conf_file << "rsn_pairwise=CCMP" << endl;
    hostapd_conf_file.close();
    timeb now;
    ftime(&now);
    cout << "主AP开始重启无线网卡，时间：" << now.time << " " << now.millitm << endl;
    system("pkill -9 hostapd");
    system(("ifconfig " + interface_name + " down").c_str());
    system(("iwconfig " + interface_name + " mode managed").c_str());
    system(("ifconfig " + interface_name + " up").c_str());
    system("hostapd hostapd.conf -B >/dev/null 2>&1");
    return HANDLE_RES_SUCCESS;
}

handle_res master_ap::transmit_new_password_to_slave_aps(void) // 向各从无线接入点传输新口令。
{
    ofstream psk_file;
    psk_file.open("/var/www/html/index.html");
    if (!psk_file.is_open())
        return HANDLE_RES_OPERATION_ERROR;
    char buff[65] = { '\0' };
    byte_to_hex_string(current_password.password, 32, buff);
    psk_file << current_password.serial_number << endl << buff << endl << current_password.expired_time << endl;
    psk_file.close();
    return HANDLE_RES_SUCCESS;
}

handle_res master_ap::main(string& conf_file_path) // 入口。
{
    cout << "初始化程序。" << endl;
    map<string, string> conf;
    handle_res res = init(conf_file_path, conf); // 解析配置文件和口令文件，获取初始口令及其失效时间、口令更新周期等信息。
    if (res != HANDLE_RES_SUCCESS) {
        cerr << "程序初始化失败。" << endl;
        return res;
    }

    time_t now = time(0);
    while (true) {
        while (now >= current_password.expired_time) { // 将WLAN口令更新至当前时间。
           generate_physical_parameter();
           calculate_password();
        }
        cout << "已生成物理认证参数：" << hex;
        for (int i = 0; i < 32; i++)
            cout << static_cast<int>(physical_parameter[i]);
        cout << endl << "得到新口令：";
        for (int i = 0; i < 32; i++)
            cout << static_cast<int>(current_password.password[i]);
        cout << dec << endl;

        cout << "保存当前口令。" << endl;
        res = update_password_file();
        if (res != HANDLE_RES_SUCCESS) {
            cerr << "保存新口令失败。" << endl;
            return res;
        }

        cout << "启动WLAN认证程序，发布物理认证参数至受控物理环境。" << endl;
        res = restart_hostapd();
        if (res != HANDLE_RES_SUCCESS) {
            cerr << "WLAN认证程序启动失败。" << endl;
            return res;
        }

        cout << "向各从无线接入点传新口令。" << endl;
        res = transmit_new_password_to_slave_aps(); // 向各从无线接入点传输新口令。
        if (res != HANDLE_RES_SUCCESS) {
            cerr << "向各从无线接入点传输新口令失败。" << endl << endl;
            return res;
        }

        cout << "等待下一个周期。" << endl << endl;
        while (now < current_password.expired_time) {
            sleep(current_password.expired_time - now);
            now = time(0);
        }
    }

    return HANDLE_RES_SUCCESS;
}
