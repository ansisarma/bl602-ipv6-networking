
extern "C" {
#include <stdio.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#include <lwip/udp.h>
#include <lwip/ip6_addr.h>
#include <lwip/netif.h>
#include <lwip/ip_addr.h>
#include <lwip/tcpip.h>
}

static void ipv6_echo_recv(void *arg,
                           struct udp_pcb *pcb,
                           struct pbuf *p,
                           const ip_addr_t *addr,
                           u16_t port)
{
    (void)arg;
    if (!p || !pcb || !addr) return;

#if LWIP_IPV6
    if (IP_IS_V6(addr)) {
        char addr_str[IP6ADDR_STRLEN_MAX];
        ip6addr_ntoa_r(ip_2_ip6(addr), addr_str, sizeof(addr_str));
        printf("[IPv6] UDP RX from [%s]:%u, len=%d\r\n",
               addr_str, port, p->len);
    }
#endif

    udp_sendto(pcb, p, addr, port);
    pbuf_free(p);
}

extern "C" void task_ipv6_echo(void *param)
{
    (void)param;

    printf("[IPv6] IPv6 echo task starting...\r\n");

    vTaskDelay(pdMS_TO_TICKS(3000));

#if LWIP_IPV6

struct netif *netif = netif_find("ap");
if (!netif) {
    printf("[IPv6] AP netif not found by name, using default\r\n");
    netif = netif_default;
}
if (!netif) {
    printf("[IPv6] ERROR: No netif available\r\n");
    vTaskDelete(nullptr);
    return;
}


    printf("[IPv6] Using netif: %c%c\r\n", netif->name[0], netif->name[1]);

    ip6_addr_t ula_addr;
ip6addr_aton("fd00::1", &ula_addr);


    netif_ip6_addr_set(netif, 1, &ula_addr);
    netif_ip6_addr_set_state(netif, 1, IP6_ADDR_VALID);
    netif_set_up(netif);

    printf("[IPv6] ULA IPv6 address set: fd00::1\r\n");

    struct udp_pcb *pcb = udp_new_ip6();
    if (!pcb) {
        printf("[IPv6] ERROR: udp_new_ip6 failed\r\n");
        vTaskDelete(nullptr);
        return;
    }

ip_addr_t bind_addr;
ipaddr_aton("fd00::1", &bind_addr);

if (udp_bind(pcb, &bind_addr, 12345) != ERR_OK) {
    printf("[IPv6] ERROR: udp_bind failed\r\n");
    udp_remove(pcb);
    vTaskDelete(nullptr);
    return;
}



    udp_recv(pcb, ipv6_echo_recv, nullptr);

    printf("[IPv6] UDP IPv6 echo server listening on fd00::1:12345\r\n");

    for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        const ip6_addr_t *addr = netif_ip6_addr(netif, i);
        if (!ip6_addr_isany(addr)) {
            char addr_str[IP6ADDR_STRLEN_MAX];
            ip6addr_ntoa_r(addr, addr_str, sizeof(addr_str));
            printf("[IPv6] Address %d: %s (state=%d)\r\n",
                   i, addr_str, netif_ip6_addr_state(netif, i));
        }
    }

#else
    printf("[IPv6] ERROR: LWIP_IPV6 is disabled\r\n");
#endif

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

