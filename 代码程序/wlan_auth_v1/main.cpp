#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include "password_parameters_handler.h"

using namespace std;

int supplicant(int itv = 600, int size = 10, const string& prv_param_path = "/home/cyk/Qt Projects/supplicant/conf/private_parameter_supplicant", const string& ifname = "wlp3s0", const string& ssid)
{
    password_parameters_handler handler(size, itv);

    int res;
    ;
    ;
    res = handler.supplicant_init(prv_param_path, ifname);
    if (res != 0)
        return res;

    string password;
    string ssid = "DACAS";
    res = handler.get_password(ssid, password);
    if (res == 1) {
        cout << "run wpa_supplicant!" << endl;
        return 1;
    }
    if (res != 0)
        return res;

    cout << password << endl << "run wpa_supplicant!" << endl;

    res = handler.update_forward_private_parameter();
    if (res != 0)
        return res;

    return 0;
}

int authenticator(void)
{
    int itv = 600;
    int size = 10;
    password_parameters_handler handler(size, itv);

    int res;
    string prv_param_path = "/home/cyk/Qt Projects/supplicant/conf/private_parameter_authenticator";
    string pub_param_path = "/home/cyk/Qt Projects/supplicant/conf/public_parameters";
    res = handler.authenticator_init(prv_param_path, pub_param_path);
    if (res != 0)
        return res;

    while (true) {
        string password;
        time_t cur_time;
        byte pub_param[32];
        res = handler.get_password(password, cur_time, pub_param);
        if (res == 1) {
            cout << "restart hostadp!" << endl;
            sleep(cur_time + itv - time(NULL));
            continue;
        }
        if (res != 0)
            return res;

        cout << password << endl << "restart hostapd!" << endl;

        res = handler.update_forward_private_parameter();
        if (res != 0)
            return res;

        sleep(cur_time + itv - time(NULL));
    }

    return 0;
}

int main()
{
    cout << supplicant() << endl;
    return 0;
}
