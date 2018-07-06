#include <cstring>
#include <fstream>
#include <iostream>
#include <istream>
#include <sstream>
#include <string>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "utils.h"

#include <sys/timeb.h>

#include "slave_ap.h"

using namespace std;

handle_res slave_ap::init(const string& conf_file_path, map<string, string>& conf) // 解析配置文件。
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

    if (conf.count("interface_name") == 0 || conf["interface_name"].empty() || conf.count("specific_ssid") == 0 || conf["specific_ssid"].empty() || conf.count("master_access_point_ip") == 0 || conf["master_access_point_ip"].empty() || conf.count("secret_key") == 0 || conf["secret_key"].empty())
        return HANDLE_RES_INVALID_PARAMETER;
    specific_ssid = conf["specific_ssid"];
    interface_name = conf["interface_name"];
    if (inet_pton(AF_INET, conf["master_access_point_ip"].c_str(), &master_ap_addr.sin_addr) <= 0)
        return HANDLE_RES_INVALID_PARAMETER;
    master_ap_addr.sin_family = AF_INET;
    short port = static_cast<uint16_t>(conf.count("master_access_point_port") == 0 ? 80 : atoi(conf["master_access_point_port"].c_str()));
    if (port <= 0)
        return HANDLE_RES_INVALID_PARAMETER;
    master_ap_addr.sin_port = htons(port);
    if (hex_string_to_byte(conf["secret_key"].c_str(), conf["secret_key"].length(), secret_key) != 32)
        return HANDLE_RES_INVALID_PARAMETER;

    return HANDLE_RES_SUCCESS;
}

handle_res slave_ap::get_current_password(void) // 获取当前口令。
{
    int sfd = socket(AF_INET, SOCK_STREAM, 0); // 创建套接字。
    if (sfd < 0)
        return HANDLE_RES_OPERATION_ERROR;

    if (connect(sfd, reinterpret_cast<sockaddr*>(&master_ap_addr), sizeof(sockaddr_in)) < 0) { // 与主无线接入点建立连接。
        close(sfd);
        return HANDLE_RES_OPERATION_ERROR;
    }

    string send_str = "GET / HTTP/1.1\r\nHost:";
    send_str += inet_ntoa(master_ap_addr.sin_addr);
    send_str += "\r\nConnection:close\r\n\r\n"; // 构造HTTP请求。

    const char* send_start = send_str.c_str();
    size_t send_idx = 0;
    size_t send_len = send_str.length();
    while (send_idx < send_len)
        send_idx += send(sfd, send_start + send_idx, send_len - send_idx, 0); // 发送HTTP请求。

    char recv_buff[1024] = {'\0'};
    recv(sfd, recv_buff, 1024, 0); // 接收HTTP响应。
    close(sfd);

    stringstream recv_str(recv_buff);
    string line;
    while (getline(recv_str, line) && !line.empty() && !(line.length() == 1 && line[0] == '\r')); // 跳过HTTP响应的状态行和响应头。

    if (!getline(recv_str, line)) // 第一行是序列号。
        return HANDLE_RES_OPERATION_ERROR;
    stringstream first_line(line);
    first_line >> current_password.serial_number;
    if (current_password.serial_number < 0)
        return HANDLE_RES_OPERATION_ERROR;

    if (!getline(recv_str, line)) // 第二行是当前口令。
        return HANDLE_RES_OPERATION_ERROR;
    if (hex_string_to_byte(line.data(), line.length(), current_password.password) != 32)
        return HANDLE_RES_OPERATION_ERROR;

    if (!getline(recv_str, line)) // 第三行是失效时间
        return HANDLE_RES_OPERATION_ERROR;
    stringstream third_line(line);
    third_line >> current_password.expired_time;

    return HANDLE_RES_SUCCESS;
}

handle_res slave_ap::restart_hostapd(void) // 启动WLAN认证程序。
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
    hostapd_conf_file << "vendor_elements=dd0c54386201" << sn << endl;
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
    cout << "从AP开始重启无线网卡，时间：" << now.time << " " << now.millitm << endl;
    system("pkill -9 hostapd");
    system(("ifconfig " + interface_name + " down").c_str());
    system(("iwconfig " + interface_name + " mode managed").c_str());
    system(("ifconfig " + interface_name + " up").c_str());
    system("hostapd hostapd.conf -B >/dev/null 2>&1");
    return HANDLE_RES_SUCCESS;
}

handle_res slave_ap::main(string& conf_file_path)
{
    cout << "初始化程序。" << endl;
    map<string, string> conf;
    handle_res res = init(conf_file_path, conf); // 解析配置文件和口令文件。
    if (res != HANDLE_RES_SUCCESS) {
        cerr << "程序初始化失败。" << endl;
        return res;
    }

    time_t now = time(0);
    while (true) {
        int err_num = 0;
        uint64_t cur_sn = current_password.serial_number;
        while (((res = get_current_password()) == HANDLE_RES_SUCCESS && cur_sn == current_password.serial_number) || (res != HANDLE_RES_SUCCESS && err_num++ < 1000))
            usleep(500000);
        if (res != HANDLE_RES_SUCCESS) {
            cerr << "请求主无线接入点当前口令失败。" << endl;
            return res;
        }
        cout << "得到新口令：" << hex;
        for (int i = 0; i < 32; i++)
            cout << static_cast<int>(current_password.password[i]);
        cout << dec << endl;

        cout << "启动WLAN认证程序。" << endl;
        res = restart_hostapd();
        if (res != HANDLE_RES_SUCCESS) {
            cerr << "WLAN认证程序启动失败。" << endl;
            return res;
        }

        cout << "等待下一个周期。" << endl << endl;
        while (now < current_password.expired_time - 10) {
            sleep(current_password.expired_time - now - 10);
            now = time(0);
        }
    }
}
