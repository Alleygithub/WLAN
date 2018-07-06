#include <cstring>
#include <fstream>
#include <sstream>
#include "utils.h"

#include "base_pwd_handler.h"

using namespace std;

handle_res base_pwd_handler::init(const string& conf_file_path, std::map<string, string>& conf) // 解析配置文件。
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

    if (conf.count("interface_name") == 0 || conf["interface_name"].empty() || conf.count("specific_ssid") == 0 || conf["specific_ssid"].empty() || conf.count("password_file_path") == 0 || conf["password_file_path"].empty() || conf.count("update_interval") == 0) // 配置文件中必须包含口令更新周期、指定SSID、网卡名称和初始口令文件路径。
        return HANDLE_RES_INVALID_PARAMETER;
    specific_ssid = conf["specific_ssid"];
    interface_name = conf["interface_name"];
    password_file_path = conf["password_file_path"];
    if (password_init() != HANDLE_RES_SUCCESS)
        return HANDLE_RES_OPERATION_ERROR;
    stringstream interval(conf["update_interval"]);
    interval >> update_interval;
    if (update_interval == 0)
        return HANDLE_RES_INVALID_PARAMETER;

    return HANDLE_RES_SUCCESS;
}

handle_res base_pwd_handler::password_init(void) // 从初始口令文件中读取Wi-Fi口令。
{
    ifstream pwd_file;
    pwd_file.open(password_file_path.c_str());
    if (!pwd_file.is_open()) {
        return HANDLE_RES_OPERATION_ERROR;
    }

    string line;
    if (!getline(pwd_file, line))
        return HANDLE_RES_OPERATION_ERROR;
    stringstream first_line(line);
    first_line >> current_time; // 第一行：当前时间。
    if (current_time == 0 || current_time > time(NULL)) {
        pwd_file.close();
        return HANDLE_RES_OPERATION_ERROR;
    }

    if (!getline(pwd_file, line))
        return HANDLE_RES_OPERATION_ERROR;
    if (base64_decode(line.data(), line.length(), password) != 32) { // 第二行：当前口令。
        pwd_file.close();
        return HANDLE_RES_OPERATION_ERROR;
    }

    if (!getline(pwd_file, line) || line.empty() || (line.length() == 1 && line[0] == '\r')) {
        pwd_file.close();
        return HANDLE_RES_SUCCESS;
    }

    if (base64_decode(line.data(), line.length(), physical_parameter) != 32) { // 第三行：当前物理认证参数。
        pwd_file.close();
        return HANDLE_RES_OPERATION_ERROR;
    }

    pwd_file.close();
    return HANDLE_RES_SUCCESS;
}

void base_pwd_handler::calculate_password() // 计算新口令。
{
    for (int i = 0; i < 32; i++) {
        password[i] ^= physical_parameter[i];
    }
    byte new_pwd[32];
    sm3_hash(password, 32, new_pwd);
    memcpy(password, new_pwd, 32);
    current_time += update_interval;
    return;
}

handle_res base_pwd_handler::update_password_file(bool store_physical_parameter) // 更新初始口令文件。
{
    if (password_file_path.empty())
        return HANDLE_RES_OPERATION_ERROR;

    ofstream pwd_file;
    pwd_file.open(password_file_path.c_str());
    if (!pwd_file.is_open()) {
        return HANDLE_RES_OPERATION_ERROR;
    }

    pwd_file << current_time << endl;
    string line;
    base64_encode(password, 32, line);
    pwd_file << line << endl;
    if (store_physical_parameter) {
        line.clear();
        base64_encode(physical_parameter, 32, line);
        pwd_file << line << endl;
    }
    pwd_file.flush();
    pwd_file.close();

    return HANDLE_RES_SUCCESS;
}
