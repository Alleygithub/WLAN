#include <iostream>
#include <unistd.h>
#include "passwordmaintaince.h"

using namespace std;

int main(int argc, char *argv[])
{
    PasswordMaintaince password_maintaince(10, 30, "/home/chenyikai/private_parameters", "/home/chenyikai/public_parameters", "/home/chenyikai/wireless", "cyk_WLAN");
    cout << password_maintaince.init() << endl;
    while (true) {
        cout << password_maintaince.update_wifi_password(time(NULL)) << endl;
        sleep(30);
    }

    return 0;
}
