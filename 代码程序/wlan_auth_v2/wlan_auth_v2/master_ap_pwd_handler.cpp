#include <cstring>
#include <fstream>
#include <iostream>
#include <time.h>
#include <unistd.h>
#include "utils.h"
#include "master_ap_pwd_handler.h"

using namespace std;

handle_res master_ap_pwd_hander::aquire_physical_parameter(void)
{
    srand(static_cast<unsigned int>(time(NULL)));
    for (int i = 0; i < 8; i++) { // 生成32字节随机数。
        int r = rand();
        memcpy(physical_parameter + i * 4, &r, 4);
    }
    return HANDLE_RES_SUCCESS;
}

handle_res master_ap_pwd_hander::init(const string& conf_file_path, map<string, string>& conf)
{
    handle_res res = base_pwd_handler::init(conf_file_path, conf);
    if (res != HANDLE_RES_SUCCESS)
        return res;

    if (conf.count("secret_key") == 0 || conf["secret_key"].empty()) // 配置文件中必须包含加密密钥。
        return HANDLE_RES_INVALID_PARAMETER;
    if (base64_decode(conf["secret_key"].c_str(), conf["secret_key"].length(), secret_key) != 32)
        return HANDLE_RES_INVALID_PARAMETER;

    return HANDLE_RES_SUCCESS;
}

handle_res master_ap_pwd_hander::main(string& conf_file_path)
{
    map<string, string> conf;
    handle_res res = init(conf_file_path, conf);
    if (res != HANDLE_RES_SUCCESS) {
        cerr << "程序初始化失败。" << endl;
        return res;
    }
    cout << "程序初始化成功。" << endl;

    system("pkill -9 hostapd");
    sleep(1);
    system("hostapd hostapd.conf >/dev/null 2>&1 &");
    sleep(1);
    while (true) {
        time_t now = time(NULL);

        while (now - current_time >= update_interval) { // 将Wi-Fi口令更新至当前时间。
            res = aquire_physical_parameter();
            if (res != HANDLE_RES_SUCCESS) {
                cerr << "更新物理认证参数失败。" << endl;
                return res;
            }
            calculate_password();
        }

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
        char physical_param[65] = {'\0'};
        byte_to_hex_string(physical_parameter, 32, physical_param);
        hostapd_conf_file << "vendor_elements=dd23332211" << physical_param << "dd0b112233" << cur_time<< endl;
        hostapd_conf_file << "auth_algs=1" << endl;
        hostapd_conf_file << "wpa=2" << endl;
        char psk[65] = {'\0'};
        byte_to_hex_string(password, 32, psk);
        hostapd_conf_file << "wpa_psk=" << psk << endl;
        hostapd_conf_file << "wpa_key_mgmt=WPA-PSK" << endl;
        hostapd_conf_file << "rsn_pairwise=CCMP" << endl;
        hostapd_conf_file.close();
        system("pkill -9 hostapd");
        sleep(1);
        system("hostapd hostapd.conf >/dev/null 2>&1 &");
        sleep(1);
        system("pkill -9 hostapd");
        sleep(1);
        if (system("hostapd hostapd.conf >/dev/null 2>&1 &") != 0) {
            cerr << "hostapd启动失败。";
            system("pkill -9 hostapd");
            return HANDLE_RES_OPERATION_ERROR;
        }
        cout << "启动Wi-Fi认证程序，发布物理认证参数至受控物理环境。" << endl;

        ofstream phy_param_file;
        phy_param_file.open("/var/www/html/index.html");
        if (!phy_param_file.is_open()) {
            cerr << "发布物理认证参数失败。" << endl;
            return HANDLE_RES_OPERATION_ERROR;
        }
        string phy_param;
        base64_encode(physical_parameter, 32, phy_param);
        phy_param_file << current_time << endl << phy_param << endl; // 发布物理认证参数。
        phy_param_file.close();
        cout << "向各从无线接入点传输物理认证参数。" << endl;

        res = update_password_file(true); // 保存当前口令。
        if (res != HANDLE_RES_SUCCESS) {
            cerr << "保存新口令失败。" << endl;
            return res;
        }
        cout << "保存新口令。" << endl;

        cout << "周期" << current_time << "的Wi-Fi口令更新成功，新口令：" << psk << endl;
        while (now - current_time < update_interval) { // 等待下一个周期。
            sleep(current_time + update_interval - now);
            now = time(NULL);
        }
    }

    return HANDLE_RES_SUCCESS;
}
