#include <iostream>
#include "slave_ap_pwd_handler.h"

using namespace std;

int main()
{
    string conf_path = "wlan_auth.conf";
    slave_ap_pwd_handler handler;
    cout << handler.main(conf_path) << endl;
    return 0;
}
