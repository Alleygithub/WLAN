#include <iostream>
#include "supplicant_pwd_handler.h"

using namespace std;

int main()
{
    string conf_path = "wlan_auth.conf";
    supplicant_pwd_handler handler;
    cout << handler.main(conf_path) << endl;
    return 0;
}
