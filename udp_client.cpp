extern "C" {
#include <stdio.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#include <lwip/udp.h>
#include <lwip/ip_addr.h>
#include <lwip/netif.h>
#include <lwip/pbuf.h>
#include <lwip/err.h>
}

#define SERVER_PORT       12345
#define SEND_INTERVAL_MS  3000

extern "C" void task_udp_client(void *param)
{
    (void)param;

    printf("[UDP] Client task started\n");

    vTaskDelay(pdMS_TO_TICKS(5000));

    struct netif *netif = netif_default;
    if (!netif) {
        printf("[UDP] ERROR: netif_default is NULL\n");
        vTaskDelete(NULL);
        return;
    }

    printf("[UDP] netif=%c%c index=%d\n",
           netif->name[0], netif->name[1],
           netif_get_index(netif));

    ip_addr_t server;
    if (!ipaddr_aton("fd00::1", &server)) {
        printf("[UDP] ERROR: ipaddr_aton failed\n");
        vTaskDelete(NULL);
        return;
    }

    printf("[UDP] Target fd00::1:%d\n", SERVER_PORT);
    
    printf("[UDP] Server address type: %d\n", server.type);
    printf("[UDP] Is IPv4: %d, Is IPv6: %d\n", 
           IP_IS_V4(&server), IP_IS_V6(&server));

    struct udp_pcb *pcb = udp_new_ip6();
    if (!pcb) {
        printf("[UDP] ERROR: udp_new_ip6 failed\n");
        vTaskDelete(NULL);
        return;
    }

    ip_addr_t any6;
    ipaddr_aton("::", &any6);   

    err_t err = udp_bind(pcb, &any6, 0);
    if (err != ERR_OK) {
        printf("[UDP] udp_bind failed: %d\n", err);
        udp_remove(pcb);
        vTaskDelete(NULL);
        return;
    }

    pcb->netif_idx = netif_get_index(netif);
    printf("[UDP] PCB netif_idx set to: %d\n", pcb->netif_idx);
    
    pcb->local_ip.type = IPADDR_TYPE_V6;
    pcb->remote_ip.type = IPADDR_TYPE_V6;
    printf("[UDP] PCB local_ip.type: %d, remote_ip.type: %d\n", 
           pcb->local_ip.type, pcb->remote_ip.type);

    int count = 0;
    
    printf("[UDP] STA IPv6 addresses:\n");
    for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        const ip6_addr_t *a = netif_ip6_addr(netif, i);
        if (!ip6_addr_isany(a)) {
            char buf[64];
            printf("  [%d] %s (state=%d)\n",
                   i,
                   ip6addr_ntoa_r(a, buf, sizeof(buf)),
                   netif_ip6_addr_state(netif, i));
        }
    }

    printf("[UDP] Waiting 2 seconds for netif stabilization...\n");
    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1) {
        char msg[64];
        int len = snprintf(msg, sizeof(msg),
                           "Hello %d from BL602 STA", ++count);

        printf("[UDP] Preparing packet %d (len=%d)...\n", count, len);
        
        struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
        if (!p) {
            printf("[UDP] pbuf_alloc failed\n");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        memcpy(p->payload, msg, len);
        
        vTaskDelay(pdMS_TO_TICKS(50));

        err = udp_sendto_if(pcb, p, &server, SERVER_PORT, netif);

        const char *err_str;
        switch(err) {
            case ERR_OK:        err_str = "OK"; break;
            case ERR_MEM:       err_str = "ERR_MEM (Out of memory)"; break;
            case ERR_BUF:       err_str = "ERR_BUF (Buffer error)"; break;
            case ERR_TIMEOUT:   err_str = "ERR_TIMEOUT"; break;
            case ERR_RTE:       err_str = "ERR_RTE (No route)"; break;
            case ERR_INPROGRESS: err_str = "ERR_INPROGRESS"; break;
            case ERR_VAL:       err_str = "ERR_VAL (Invalid value)"; break;
            case ERR_WOULDBLOCK: err_str = "ERR_WOULDBLOCK (-12)"; break;
            case ERR_USE:       err_str = "ERR_USE (Already in use)"; break;
            case ERR_ISCONN:    err_str = "ERR_ISCONN (Already connected)"; break;
            case ERR_ABRT:      err_str = "ERR_ABRT (Connection aborted)"; break;
            case ERR_RST:       err_str = "ERR_RST (Connection reset)"; break;
            case ERR_CLSD:      err_str = "ERR_CLSD (Connection closed)"; break;
            case ERR_CONN:      err_str = "ERR_CONN (Not connected)"; break;
            case ERR_IF:        err_str = "ERR_IF (Netif issue)"; break;
            default:            err_str = "Unknown error"; break;
        }

        printf("[UDP] send %d → %d (%s)\n", count, err, err_str);

        pbuf_free(p);
        vTaskDelay(pdMS_TO_TICKS(SEND_INTERVAL_MS));
    }
}