#include "soem/soem.h"

#include <stdio.h>
#include <string.h>

#define IFNAMESIZE 256

static uint8 IOmap[4096];
static ecx_contextt ctx;

static void dump_slave_errors(void)
{
    int i;

    for (i = 1; i <= ctx.slavecount; i++)
    {
        printf("Slave %d state=0x%02x, ALstatus=0x%04x (%s)\n",
               i, ctx.slavelist[i].state, ctx.slavelist[i].ALstatuscode,
               ec_ALstatuscode2string(ctx.slavelist[i].ALstatuscode));
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

    if (!ecx_init(&ctx, ifname))
    {
        printf("Error: could not initialize adapter '%s'.\n", ifname);
        return -1;
    }

    printf("Adapter init OK\n");

    if (ecx_config_init(&ctx) <= 0)
    {
        printf("No EtherCAT slaves found.\n");
        ecx_close(&ctx);
        return -1;
    }

    printf("%d slaves found.\n", ctx.slavecount);

    ecx_config_map_group(&ctx, IOmap, 0);
    ecx_configdc(&ctx);

    printf("Request SAFE_OP state...\n");
    ecx_statecheck(&ctx, 0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);

    expectedWKC = (ctx.group[0].outputsWKC * 2) + ctx.group[0].inputsWKC;
    printf("Expected WKC: %d\n", expectedWKC);

    printf("Request OP state...\n");
    ctx.slavelist[0].state = EC_STATE_OPERATIONAL;
    ecx_send_processdata(&ctx);
    ecx_receive_processdata(&ctx, EC_TIMEOUTRET);
    ecx_writestate(&ctx, 0);

    chk = 40;
    do
    {
        ecx_send_processdata(&ctx);
        wkc = ecx_receive_processdata(&ctx, EC_TIMEOUTRET);
        ecx_statecheck(&ctx, 0, EC_STATE_OPERATIONAL, 50000);
    } while (chk-- && (ctx.slavelist[0].state != EC_STATE_OPERATIONAL));

    if (ctx.slavelist[0].state != EC_STATE_OPERATIONAL)
    {
        printf("Not all slaves reached OPERATIONAL.\n");
        ecx_readstate(&ctx);
        dump_slave_errors();
        ecx_close(&ctx);
        return -1;
    }

    printf("Slaves are now in OP state.\n");

    while (1)
    {
        if ((ctx.slavecount >= 1) && (ctx.slavelist[1].outputs != NULL))
        {
            *((uint16 *)ctx.slavelist[1].outputs) = controlword;
        }

        ecx_send_processdata(&ctx);
        wkc = ecx_receive_processdata(&ctx, EC_TIMEOUTRET);

        if (wkc < expectedWKC)
        {
            printf("WKC too low: %d (expected %d)\n", wkc, expectedWKC);
        }

        osal_usleep(5000);
    }

    ecx_close(&ctx);
    return 0;
}
