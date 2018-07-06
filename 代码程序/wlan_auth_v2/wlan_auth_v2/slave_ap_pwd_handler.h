#ifndef SLAVE_AP_PWD_HANDLER_H
#define SLAVE_AP_PWD_HANDLER_H

#include <netinet/in.h>
#include "base_pwd_handler.h"

class slave_ap_pwd_handler : public base_pwd_handler
{
private:
    sockaddr_in master_access_point_address;
    time_t master_access_point_current_time;
    byte secret_key[32];

public:
    slave_ap_pwd_handler() {}

    virtual handle_res init(const std::string& conf_file_path, std::map<std::string, std::string>& conf); // 解析配置文件。
    virtual handle_res aquire_physical_parameter(void); // 获取物理认证参数：从无线接入点通过HTTP协议向主无线接入点请求物理认证参数。
    virtual handle_res main(std::string& conf_file_path); // 入口。
};

#endif // SLAVE_AP_PWD_HANDLER_H
