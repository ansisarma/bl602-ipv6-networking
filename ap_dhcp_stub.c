#include <stdio.h>
#include <lwip/netif.h>

#ifdef __cplusplus
extern "C" {
#endif

void dhcpd_start(struct netif *netif)
{
   
    printf("[DHCPD] FSM-compatible DHCP stub active\n");
    (void)netif;
}

void dhcp_server_stop(struct netif *netif)
{
    (void)netif;
}

#ifdef __cplusplus
}
#endif

