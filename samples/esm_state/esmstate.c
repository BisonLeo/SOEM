#include "soem/soem.h"

#include <stdio.h>
#include <string.h>

#define IFNAMESIZE 256

static uint8 IOmap[4096];

static void dump_slave_errors(void)
{
    int i;

    for (i = 1; i <= ec_slavecount; i++)
    {
        printf("Slave %d state=0x%02x, ALstatus=0x%04x (%s)\n",
               i, ec_slave[i].state, ec_slave[i].ALstatuscode,
               ec_ALstatuscode2string(ec_slave[i].ALstatuscode));
    }
}

int main(int argc, char *argv[])
{
    char ifname[IFNAMESIZE] = "\\Device\\NPF_{YOUR-ADAPTER-GUID}";
    uint16 controlword = 0x000F;
    int expectedWKC;
    int chk;
    int wkc;

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

    printf("Adapter init OK\n");

    if (ec_config_init(FALSE) <= 0)
    {
        printf("No EtherCAT slaves found.\n");
        ec_close();
        return -1;
    }

    printf("%d slaves found.\n", ec_slavecount);

    ec_config_map(IOmap);
    ec_configdc();

    printf("Request SAFE_OP state...\n");
    ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);

    expectedWKC = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC;
    printf("Expected WKC: %d\n", expectedWKC);

    printf("Request OP state...\n");
    ec_slave[0].state = EC_STATE_OPERATIONAL;
    ec_send_processdata();
    ec_receive_processdata(EC_TIMEOUTRET);
    ec_writestate(0);

    chk = 40;
    do
    {
        ec_send_processdata();
        wkc = ec_receive_processdata(EC_TIMEOUTRET);
        ec_statecheck(0, EC_STATE_OPERATIONAL, 50000);
    } while (chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));

    if (ec_slave[0].state != EC_STATE_OPERATIONAL)
    {
        printf("Not all slaves reached OPERATIONAL.\n");
        ec_readstate();
        dump_slave_errors();
        ec_close();
        return -1;
    }

    printf("Slaves are now in OP state.\n");

    while (1)
    {
        if ((ec_slavecount >= 1) && (ec_slave[1].outputs != NULL))
        {
            *((uint16 *)ec_slave[1].outputs) = controlword;
        }

        ec_send_processdata();
        wkc = ec_receive_processdata(EC_TIMEOUTRET);

        if (wkc < expectedWKC)
        {
            printf("WKC too low: %d (expected %d)\n", wkc, expectedWKC);
        }

        osal_usleep(5000);
    }

    ec_close();
    return 0;
}
