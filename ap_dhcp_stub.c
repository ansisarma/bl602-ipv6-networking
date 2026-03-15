#include <stdio.h>
#include <lwip/err.h>
#include <lwip/netif.h>

#ifdef __cplusplus
extern "C" {
#endif

void dhcpd_start(struct netif *netif)
{
    (void)netif;
    printf("[DHCPD] Stub: dhcpd_start called (no DHCP server started)\r\n");
}

err_t dhcp_server_stop(struct netif *netif)
{
    (void)netif;
    printf("[DHCPD] Stub: dhcp_server_stop called\r\n");
    return ERR_OK;
}

#ifdef __cplusplus
}
#endif
