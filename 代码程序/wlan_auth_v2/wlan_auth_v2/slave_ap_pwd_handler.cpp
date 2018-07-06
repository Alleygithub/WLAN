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

#include "slave_ap_pwd_handler.h"

using namespace std;

handle_res slave_ap_pwd_handler::aquire_physical_parameter(void)
{
    int sfd = socket(AF_INET, SOCK_STREAM, 0); // 创建套接字。
    if (sfd < 0)
        return HANDLE_RES_OPERATION_ERROR;

    if (connect(sfd, reinterpret_cast<sockaddr*>(&master_access_point_address), sizeof(sockaddr_in)) < 0) { // 与主无线接入点建立连接。
        close(sfd);
        return HANDLE_RES_OPERATION_ERROR;
    }

    string send_str = "GET / HTTP/1.1\r\nHost:";
    send_str += inet_ntoa(master_access_point_address.sin_addr);
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

    if (!getline(recv_str, line)) // 第一行是当前时间。
        return HANDLE_RES_OPERATION_ERROR;
    stringstream first_line(line);
    first_line >> master_access_point_current_time;
    if (master_access_point_current_time == 0)
        return HANDLE_RES_OPERATION_ERROR;

    if (!getline(recv_str, line)) // 第二行是物理认证参数。
        return HANDLE_RES_OPERATION_ERROR;
    if (base64_decode(line.data(), line.length(), physical_parameter) != 32)
        return HANDLE_RES_OPERATION_ERROR;

    return HANDLE_RES_SUCCESS;
}

handle_res slave_ap_pwd_handler::init(const string& conf_file_path, map<string, string>& conf)
{
    handle_res res = base_pwd_handler::init(conf_file_path, conf);
    if (res != HANDLE_RES_SUCCESS)
        return res;

    if (conf.count("master_access_point_ip") == 0 || conf["master_access_point_ip"].empty() || conf.count("secret_key") == 0 || conf["secret_key"].empty())
        return HANDLE_RES_INVALID_PARAMETER;
    if (inet_pton(AF_INET, conf["master_access_point_ip"].c_str(), &master_access_point_address.sin_addr) <= 0)
        return HANDLE_RES_INVALID_PARAMETER;
    master_access_point_address.sin_family = AF_INET;
    short port = static_cast<uint16_t>(conf.count("master_access_point_port") == 0 ? 80 : atoi(conf["master_access_point_port"].c_str()));
    if (port <= 0)
        return HANDLE_RES_INVALID_PARAMETER;
    master_access_point_address.sin_port = htons(port);
    if (base64_decode(conf["secret_key"].c_str(), conf["secret_key"].length(), secret_key) != 32)
        return HANDLE_RES_INVALID_PARAMETER;

    return HANDLE_RES_SUCCESS;
}

handle_res slave_ap_pwd_handler::main(string& conf_file_path)
{
    map<string, string> conf;
    handle_res res = init(conf_file_path, conf);
    if (res != HANDLE_RES_SUCCESS) {
        cerr << "程序初始化失败。" << endl;
        return res;
    }
    cout << "程序初始化成功。" << endl;

    int err_cnt = 0;
    while ((res = aquire_physical_parameter()) != HANDLE_RES_SUCCESS && err_cnt++ < 5)
        sleep(5);
    if (res != HANDLE_RES_SUCCESS) {
        cerr << "请求主无线接入点当前口令更新周期和物理认证参数失败。" << endl;
        return res;
    }

    while (true) {
        cout << "主无线接入点当前时间：" << master_access_point_current_time << "；上一次更新口令时间：" << current_time << endl;
        if (master_access_point_current_time < current_time || master_access_point_current_time - current_time >= 2 * update_interval) {
            cerr << "无法更新Wi-Fi口令。" << endl;
            return HANDLE_RES_OPERATION_ERROR;
        }
        if (master_access_point_current_time != current_time && master_access_point_current_time - update_interval != current_time) {
            cerr << "物理认证参数更新周期和Wi-Fi口令更新周期不一致。" << endl;
            return HANDLE_RES_OPERATION_ERROR;
        }
        if (master_access_point_current_time - current_time == update_interval) // 将Wi-Fi口令更新至下一口令更新周期。
            calculate_password();

        ofstream hostapd_conf_file;
        hostapd_conf_file.open("hostapd.conf");
        if (!hostapd_conf_file.is_open()) {
            cerr << "hostapd配置文件打开失败。" << endl;
            return HANDLE_RES_OPERATION_ERROR;
        }
        hostapd_conf_file << "interface=" << interface_name << endl;
        hostapd_conf_file << "driver=nl80211" << endl;
        hostapd_conf_file << "ssid=" << specific_ssid << endl;
        hostapd_conf_file << "hw_mode=g" << endl;
        hostapd_conf_file << "channel=1" << endl;
        hostapd_conf_file << "macaddr_acl=0" << endl;
        char cur_time[17] = {'\0'};
        byte_to_hex_string(reinterpret_cast<byte*>(&current_time), 8, cur_time);
        hostapd_conf_file << "vendor_elements=dd0b112233" << cur_time << endl;
        hostapd_conf_file << "auth_algs=1" << endl;
        hostapd_conf_file << "wpa=2" << endl;
        char psk[65] = {'\0'};
        byte_to_hex_string(password, 32, psk);
        hostapd_conf_file << "wpa_psk=" << psk << endl;
        hostapd_conf_file << "wpa_key_mgmt=WPA-PSK" << endl;
        hostapd_conf_file << "rsn_pairwise=CCMP" << endl;
        hostapd_conf_file.close();
        system("pkill -9 hostapd");
        if (system("hostapd hostapd.conf >/dev/null 2>&1 &") != 0) {
            cerr << "hostapd启动失败。";
            system("pkill -9 hostapd");
            return HANDLE_RES_OPERATION_ERROR;
        }
        cout << "启动Wi-Fi认证程序。" << endl;

        res = update_password_file(); // 保存新口令。
        if (res != HANDLE_RES_SUCCESS) {
            cerr << "保存新口令失败。" << endl;
            return res;
        }
        cout << "保存新口令。" << endl;

        cout << "周期" << current_time << "的Wi-Fi口令更新成功，新口令：" << psk << endl;
        sleep(current_time + update_interval - time(NULL) - 10); // 等待下一个周期，提前10秒轮询。
        do {
            sleep(1);
            err_cnt = 0;
            while ((res = aquire_physical_parameter()) != HANDLE_RES_SUCCESS && err_cnt++ < 5)
                sleep(5);
            if (res != HANDLE_RES_SUCCESS) {
                cerr << "请求主无线接入点当前口令更新周期和物理认证参数失败。" << endl;
                return res;
            }
        } while (master_access_point_current_time == current_time);
    }
}
