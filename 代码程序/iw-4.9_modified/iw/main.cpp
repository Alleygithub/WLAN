extern "C"
{
    #include "iw.h"
}

unsigned char public_parameter[32] = {0};
unsigned long long system_time = 0;
char* specific_ssid = NULL;
bool has_found = false;

int main(void)
{
    char ifname[] = "wlp3s0";
    char ssid[] = "hahahaha";
    unsigned char parameter[32] = {0};
    unsigned long long time = 0;

    int argc = 5;
    char argv0[] = "./iw";
    char argv1[] = "dev";
    char *argv2 = ifname;
    char argv3[] = "scan";
    char argv4[] = "-u";
    char *argv[5] = { argv0, argv1, argv2, argv3, argv4 };

    specific_ssid = (char*)calloc(strlen(ssid) + 1, sizeof(unsigned char));
    strcpy(specific_ssid, ssid);
    iw(argc, argv);
    free(specific_ssid);
    memcpy(parameter, public_parameter, 32);
    time = system_time;

    if (has_found) {
        printf("system_time = %lld, public_parameter = ", time);
        int i;
        for (i = 0; i < 32; i++) {
            printf("%.2x", parameter[i]);
        }
        printf("\n");
    }

    return 0;
}

