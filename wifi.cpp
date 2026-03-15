extern "C" {
#include <stdio.h>

#include <FreeRTOS.h>
#include <task.h>

#include <aos/yloop.h>
#include <bl_wifi.h>
#include <blog.h>
#include <easyflash.h>
#include <event_device.h>
#include <hal_wifi.h>
#include <looprt.h>
#include <loopset.h>
#include <vfs.h>

#include <lwip/tcpip.h>
#include <lwip/netif.h>
#include <lwip/ip6_addr.h>

#include <wifi_mgmr_ext.h>
}

extern "C" void task_udp_client(void *param);

static wifi_conf_t wifi_conf = {
    .country_code = "US",
    .channel_nums = {0},
};

static void force_ipv4_and_fsm_ok(void)
{
    struct netif *n = netif_default;
    if (!n) {
        printf("[WIFI] netif not ready\n");
        return;
    }

    ip4_addr_t ip, mask, gw;
    IP4_ADDR(&ip,   192,168,11,100);
    IP4_ADDR(&mask, 255,255,255,0);
    IP4_ADDR(&gw,   192,168,11,1);

    netif_set_addr(n, &ip, &mask, &gw);
    netif_set_up(n);
    netif_set_link_up(n);

    printf("[WIFI] Dummy IPv4 injected\n");

    aos_post_event(EV_WIFI, CODE_WIFI_ON_GOT_IP, 0);
}

static void sta_add_ula_ipv6(void)
{
    struct netif *netif = netif_find("st");
    if (!netif) {
        printf("[STA][IPv6] STA netif not found by name, using default\n");
        netif = netif_default;
    }
    if (!netif) {
        printf("[STA][IPv6] ERROR: No netif available\n");
        return;
    }

    ip6_addr_t ula;
    ip6addr_aton("fd00::2", &ula);

    netif_ip6_addr_set(netif, 1, &ula);
    netif_ip6_addr_set_state(netif, 1, IP6_ADDR_VALID);
    netif_set_up(netif);

    printf("[STA][IPv6] ULA IPv6 address set: fd00::2\n");
}

static void event_cb_wifi(input_event_t *event, void *private_data)
{
    (void)private_data;

    switch (event->code) {

    case CODE_WIFI_ON_INIT_DONE:
        printf("[WIFI] Init done\n");
        wifi_mgmr_start_background(&wifi_conf);
        break;

    case CODE_WIFI_ON_MGMR_DONE: {
        printf("[WIFI] Starting STA\n");

        wifi_interface_t iface = wifi_mgmr_sta_enable();
        if (iface) {
            static char ssid[] = "TEST_OPEN";
            static char password[] = "";

            wifi_mgmr_sta_connect(
                &iface,
                ssid,
                password,
                NULL,
                NULL,
                0,
                0
            );
            printf("[WIFI] STA connect issued\n");
        }
        break;
    }

    case CODE_WIFI_ON_CONNECTED:
        printf("[WIFI] Connected\n");

        vTaskDelay(pdMS_TO_TICKS(500));

        sta_add_ula_ipv6();

        force_ipv4_and_fsm_ok();
        break;

    case CODE_WIFI_ON_GOT_IP: {
        static int started = 0;
        if (!started) {
            started = 1;
            printf("[WIFI] Network ready → starting UDP client\n");

            xTaskCreate(
                task_udp_client,
                "udp_client",
                2048,
                NULL,
                10,
                NULL
            );
        }
        break;
    }

    case CODE_WIFI_ON_DISCONNECT:
        printf("[WIFI] Disconnected\n");
        break;

    default:
        break;
    }
}

extern "C" void task_wifi(void *param)
{
    (void)param;

    printf("\n=== BL602 IPv6 UDP CLIENT START ===\n");

    tcpip_init(NULL, NULL);

    static StackType_t looprt_stack[512];
    static StaticTask_t looprt_task;
    looprt_start(looprt_stack, 512, &looprt_task);
    loopset_led_hook_on_looprt();

    easyflash_init();
    vfs_init();
    vfs_device_init();

    aos_loop_init();
    aos_register_event_filter(EV_WIFI, event_cb_wifi, NULL);

    hal_wifi_start_firmware_task();

    aos_post_event(EV_WIFI, CODE_WIFI_ON_INIT_DONE, 0);

    aos_loop_run();
}
