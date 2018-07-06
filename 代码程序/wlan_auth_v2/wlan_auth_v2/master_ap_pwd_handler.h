#ifndef MASTER_AP_PWD_HANDLER_H
#define MASTER_AP_PWD_HANDLER_H

#include <pthread.h>
#include "base_pwd_handler.h"

class master_ap_pwd_hander : public base_pwd_handler
{
private:
    byte secret_key[32];

public:
    master_ap_pwd_hander() {}

    virtual handle_res init(const std::string& conf_file_path, std::map<std::string, std::string>& conf); // 解析配置文件。
    virtual handle_res aquire_physical_parameter(void); // 获取物理认证参数：主无线接入点生成伪随机数作为物理认证参数。
    virtual handle_res main(std::string& conf_file_path); // 入口。
};

#endif // MASTER_AP_PWD_HANDLER_H
