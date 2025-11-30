#include "soem/soem.h"

#include <stdio.h>
#include <string.h>

#define IFNAMESIZE 256

int main(int argc, char *argv[])
{
    char ifname[IFNAMESIZE] = "\\Device\\NPF_{YOUR-ADAPTER-GUID}";

    if (argc > 1)
    {
        strncpy(ifname, argv[1], sizeof(ifname) - 1);
        ifname[sizeof(ifname) - 1] = '\0';
    }

    if (!ec_init(ifname))
    {
        printf("Error: could not initialize adapter '%s'.\n", ifname);
        return -1;
    }

    printf("Adapter init OK, ESM is out of INIT. Keeping interface open...\n");

    while (1)
    {
        osal_usleep(1000000);
    }

    ec_close();
    return 0;
}
