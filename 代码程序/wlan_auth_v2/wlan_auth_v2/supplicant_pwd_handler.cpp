#include <cstring>
#include <fstream>
#include <iostream>
#include <net/if.h>
#include <unistd.h>
#include "capture.h"
#include "utils.h"

#include "supplicant_pwd_handler.h"

using namespace std;

handle_res supplicant_pwd_handler::aquire_physical_parameter(void)
{
    capture_result = 0;
    access_point_current_time = 0;
    bzero(physical_parameter, 32);

    int ret = capture_physical_parameter_and_system_time_from_80211_management_frame(interface_name.c_str(), specific_ssid.c_str(), capture_result, physical_parameter, access_point_current_time);
    if (ret != 0)
        return HANDLE_RES_OPERATION_ERROR; // 无法捕捉到公开参数。

    return HANDLE_RES_SUCCESS;
}

handle_res supplicant_pwd_handler::init(const string& conf_file_path, map<string, string>& conf)
{
    handle_res res = base_pwd_handler::init(conf_file_path, conf);
    if (res != HANDLE_RES_SUCCESS)
        return res;

    return HANDLE_RES_SUCCESS;
}

handle_res supplicant_pwd_handler::main(string& conf_file_path)
{
    map<string, string> conf;
    handle_res res = init(conf_file_path, conf);
    if (res != HANDLE_RES_SUCCESS) {
        cerr << "程序初始化失败。" << endl;
        return HANDLE_RES_OPERATION_ERROR;
    }
    cout << "程序初始化成功。" << endl;

    int err_cnt = 0;
    while (((res = aquire_physical_parameter()) != HANDLE_RES_SUCCESS || (capture_result != 1 && capture_result != 3)) && err_cnt++ < 5)
        sleep(10);
    if (res != HANDLE_RES_SUCCESS) {
        cerr << "捕捉Wi-Fi信标帧异常。" << endl;
        return res;
    }
    if (capture_result != 1 && capture_result != 3) {
        cerr << "提取无线接入点当前口令更新周期失败。" << endl;
        return HANDLE_RES_OPERATION_ERROR;
    }

    while (true) {
        cout << "无线接入点当前时间：" << access_point_current_time << "；上一次更新口令时间：" << current_time << endl;
        if (access_point_current_time < current_time || access_point_current_time - current_time >= 2 * update_interval) {
            cerr << "无法更新Wi-Fi口令。" << endl;
            return HANDLE_RES_OPERATION_ERROR;
        }
        if (access_point_current_time != current_time && access_point_current_time - update_interval != current_time) {
            cerr << "物理认证参数更新周期和Wi-Fi口令更新周期不一致。" << endl;
            return HANDLE_RES_OPERATION_ERROR;
        }
        if (access_point_current_time - update_interval != current_time && capture_result != 3)
            cerr << "请通过物理认证，进入受控物理环境。" << endl;
        if (access_point_current_time == current_time || capture_result == 3){
            if (access_point_current_time - update_interval == current_time)
                calculate_password();

            ofstream wpa_supplicant_conf_file;
            wpa_supplicant_conf_file.open("wpa_supplicant.conf");
            if (!wpa_supplicant_conf_file.is_open()) {
                cerr << "wpa_supplicant配置文件打开失败。" << endl;
                return HANDLE_RES_OPERATION_ERROR;
            }
            wpa_supplicant_conf_file << "network={" << endl;
            wpa_supplicant_conf_file << "    ssid=\"" << specific_ssid << "\"" << endl;
            wpa_supplicant_conf_file << "    auth_alg=OPEN" << endl;
            wpa_supplicant_conf_file << "    proto=RSN" << endl;
            wpa_supplicant_conf_file << "    key_mgmt=WPA-PSK" << endl;
            wpa_supplicant_conf_file << "    pairwise=CCMP" << endl;
            char psk[65] {'\0'};
            byte_to_hex_string(password, 32, psk);
            wpa_supplicant_conf_file << "    psk=" << psk << endl;
            wpa_supplicant_conf_file << "}" << endl;
            string cmd = "wpa_supplicant -B -D nl80211 -c wpa_supplicant.conf -i " + interface_name  + " >/dev/null 2>&1 &";
            system("pkill -9 wpa_supplicant");
            if (system(cmd.c_str()) != 0) {
                cerr << "wpa_supplicant启动失败。";
                system("pkill -9 wpa_supplicant");
                return HANDLE_RES_OPERATION_ERROR;
            }
            cout << "启动Wi-Fi接入程序。" << endl;

            res = update_password_file(); // 保存新口令。
            if (res != HANDLE_RES_SUCCESS) {
                cerr << "保存新口令失败。" << endl;
                return res;
            }
            cout << "保存新口令。" << endl;

            cout << "周期" << current_time << "的口令更新成功，新口令：" << psk << endl;
        }

        do {
            sleep(2);
            err_cnt = 0;
            while (((res = aquire_physical_parameter()) != HANDLE_RES_SUCCESS || (capture_result != 1 && capture_result != 3)) && err_cnt++ < 5)
                sleep(10);
            if (res != HANDLE_RES_SUCCESS) {
                cerr << "捕捉Wi-Fi信标帧异常。" << endl;
                return res;
            }
            if (capture_result != 1 && capture_result != 3) {
                cerr << "提取无线接入点当前口令更新周期失败。" << endl;
                return HANDLE_RES_OPERATION_ERROR;
            }
        } while (access_point_current_time == current_time);
    }
}
