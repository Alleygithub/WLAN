#include <iostream>
#include "master_ap_pwd_handler.h"

using namespace std;

int main()
{
    string conf_path = "wlan_auth.conf";
    master_ap_pwd_hander handler;
    cout << handler.main(conf_path) << endl;
    return 0;
}
