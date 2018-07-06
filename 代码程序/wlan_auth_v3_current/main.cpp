#include <iostream>
#include <string>

#ifdef MASTER_AP
#include "master_ap.h"
#endif

#ifdef SLAVE_AP
#include "slave_ap.h"
#endif

using namespace std;

int main()
{
#ifdef MASTER_AP
    string conf_path = "wlan_auth.conf";
    master_ap ap;
    cout << ap.main(conf_path) << endl;
#elif SLAVE_AP
    string conf_path = "wlan_auth.conf";
    slave_ap ap;
    cout << ap.main(conf_path) << endl;
#else
    cout << "Hello World" << endl;
#endif
    return 0;
}
