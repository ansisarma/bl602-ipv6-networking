extern "C" {
#include <stdio.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include "lwip/netif.h"
#include "lwip/ip6_addr.h"

#include <aos/yloop.h>
#include <bl_wifi.h>
#include <blog.h>
#include <easyflash.h>
#include <event_device.h>
#include <hal_board.h>
#include <hal_gpio.h>
#include <hal_sys.h>
#include <hal_uart.h>
#include <hal_wifi.h>
#include <libfdt.h>
#include <looprt.h>
#include <loopset.h>
#include <vfs.h>

#include <lwip/tcpip.h>

#include <wifi_mgmr_ext.h>
}

extern "C" void task_ipv6_echo(void *param);

static wifi_conf_t wifi_conf = {
    .country_code = "US",
    .channel_nums = { 0 },
};
void ap_add_ula_ipv6(void)
{
    struct netif *netif = netif_find("ap");
if (!netif) {
    printf("[IPv6] AP netif not found by name, using default\n");
    netif = netif_default;
}
if (!netif) {
    printf("[IPv6] ERROR: No netif available\n");
    return;
}


    ip6_addr_t ula_addr;
    ip6addr_aton("fd00::1", &ula_addr);


    netif_ip6_addr_set(netif, 1, &ula_addr);
    netif_ip6_addr_set_state(netif, 1, IP6_ADDR_VALID);
    netif_set_up(netif);

    printf("[IPv6] ULA IPv6 address set on AP: fd00::1\n");
}

static void event_cb_wifi(input_event_t *event, void *private_data)
{
    (void)private_data;

    switch (event->code) {
    case CODE_WIFI_ON_INIT_DONE: {
        printf("[WIFI] INIT_DONE -> start mgmr\n");
        wifi_mgmr_start_background(&wifi_conf);
        break;
    }

    case CODE_WIFI_ON_MGMR_DONE: {
        printf("[WIFI] MGMR_DONE -> trying to start AP\n");
        
        wifi_interface_t wifi_interface = wifi_mgmr_ap_enable();
        printf("[WIFI] wifi_mgmr_ap_enable returned: %p\n", (void*)wifi_interface);
        
        if (!wifi_interface) {
            printf("[WIFI] ERROR: Got NULL interface\n");
            break;
        }
        
        printf("[WIFI] Waiting 3 seconds...\n");
        vTaskDelay(pdMS_TO_TICKS(3000));
        
        printf("\n[AP_TEST 1] Trying OPEN network...\n");
        static char test_ssid1[] = "TEST_OPEN";
        
        printf("[WIFI] Calling wifi_mgmr_ap_start()...\n");
        int rc = wifi_mgmr_ap_start(&wifi_interface,
                                   test_ssid1,
                                   0,
                                   NULL,  // NULL = open network
                                   11);   // channel 11
        printf("[WIFI] wifi_mgmr_ap_start RETURNED: %d\n", rc);
        
        if (rc != 0) {
            printf("[AP_TEST 1] Failed (rc=%d). Waiting...\n", rc);
            vTaskDelay(pdMS_TO_TICKS(5000));
            
            printf("\n[AP_TEST 2] Trying with password...\n");
            static char test_ssid2[] = "TEST_WPA2";
            static char test_pass2[] = "12345678";
            
            rc = wifi_mgmr_ap_start(&wifi_interface,
                                   test_ssid2,
                                   0,
                                   test_pass2,
                                   6);   // channel 6
            printf("[WIFI] Second attempt returned: %d\n", rc);
        }
        
        if (rc == 0) {
            printf("[WIFI] SUCCESS: AP start command accepted\n");
        } else {
            printf("[WIFI] ERROR: All attempts failed (last rc=%d)\n", rc);
        }
        break;
    }

    case CODE_WIFI_ON_AP_STARTED: {
        printf("\n[WIFI] ========== AP_STARTED RECEIVED! ==========\n");
        printf("[WIFI] AP should now be visible\n");
      ap_add_ula_ipv6();

        vTaskDelay(pdMS_TO_TICKS(2000));
        
        BaseType_t task_rc = xTaskCreate(
            task_ipv6_echo,
            "ipv6_echo",
            1024,
            nullptr,
            10,
            nullptr
        );
        
        if (task_rc != pdPASS) {
            printf("[WIFI] Failed to create IPv6 task\n");
        } else {
            printf("[WIFI] IPv6 task created\n");
        }
        break;
    }

    case CODE_WIFI_ON_AP_STOPPED: {
        printf("[WIFI] AP stopped\n");
        break;
    }

    case CODE_WIFI_ON_AP_STA_ADD: {
        printf("[WIFI] Station connected\n");
        break;
    }

    case CODE_WIFI_ON_AP_STA_DEL: {
        printf("[WIFI] Station disconnected\n");
        break;
    }

    default: {
        printf("[WIFI] Unknown event: %d\n", event->code);
        break;
    }
    }
}

extern "C" void task_wifi(void *param)
{
    (void)param;

    printf("[WIFI] WiFi task starting\n");

    printf("[WIFI] Starting lwIP TCP/IP stack...\n");
    tcpip_init(nullptr, nullptr);
    printf("[WIFI] TCP/IP stack ready\n");

    constexpr uint16_t LOOPRT_STACK_SIZE = 512;
    static StackType_t looprt_stack[LOOPRT_STACK_SIZE];
    static StaticTask_t looprt_task;
    looprt_start(looprt_stack, LOOPRT_STACK_SIZE, &looprt_task);
    loopset_led_hook_on_looprt();
    printf("[WIFI] looprt started\n");

    easyflash_init();
    vfs_init();
    vfs_device_init();
    printf("[WIFI] Storage initialized\n");

    aos_loop_init();
    aos_register_event_filter(EV_WIFI, event_cb_wifi, nullptr);
    printf("[WIFI] Event loop ready\n");

    printf("[WIFI] Starting WiFi firmware...\n");
    hal_wifi_start_firmware_task();

    aos_post_event(EV_WIFI, CODE_WIFI_ON_INIT_DONE, 0);
    printf("[WIFI] Firmware started\n");

    aos_loop_run();

    printf("[WIFI] ERROR: Event loop exited\n");
    vTaskDelete(nullptr);
}
