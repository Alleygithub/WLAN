#ifndef BASE_PWD_HANDLER_H
#define BASE_PWD_HANDLER_H

#include <map>
#include <string>
#include "typedef.h"

enum handle_res
{
    HANDLE_RES_SUCCESS = 0,
    HANDLE_RES_INVALID_PARAMETER = -1,
    HANDLE_RES_EXPIRED_PARAMETER = -2,
    HANDLE_RES_OPERATION_ERROR = -3,
    HANDLE_RES_NO_CHANGE = -4,
};

class base_pwd_handler
{
protected:
    uint32_t update_interval;
    std::string interface_name;
    std::string specific_ssid;
    std::string password_file_path;
    time_t current_time;
    byte password[32];
    byte physical_parameter[32];

    virtual handle_res init(const std::string& conf_file_path, std::map<std::string, std::string>& conf); // 解析配置文件。
    handle_res password_init(void); // 从初始口令文件中读取Wi-Fi口令。
    virtual handle_res aquire_physical_parameter(void) = 0; // 获取物理认证参数：主无线接入点生成伪随机数作为物理认证参数，从无线接入点通过HTTP协议向主无线接入点请求物理认证参数，移动终端捕捉并解析主无线接入点的Wi-Fi信标帧获取物理认证参数。
    void calculate_password(); // 计算新口令。
    handle_res update_password_file(bool store_physical_parameter = false); // 更新初始口令文件。

public:
    base_pwd_handler() {}

    virtual handle_res main(std::string& conf_file_path) = 0; // 入口。
};

#endif // BASE_PWD_HANDLER_H
