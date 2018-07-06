#ifndef SUPPLICANT_PWD_HANDLER_H
#define SUPPLICANT_PWD_HANDLER_H

#include "base_pwd_handler.h"

class supplicant_pwd_handler : public base_pwd_handler
{
private:
    time_t access_point_current_time;
    int capture_result;

public:
    supplicant_pwd_handler() {}

    virtual handle_res init(const std::string& conf_file_path, std::map<std::string, std::string>& conf); // 解析配置文件。
    virtual handle_res aquire_physical_parameter(void); // 获取物理认证参数：移动终端捕捉并解析主无线接入点的Wi-Fi信标帧获取物理认证参数。
    virtual handle_res main(std::string& conf_file_path); // 入口。
};

#endif // SUPPLICANT_PWD_HANDLER_H
