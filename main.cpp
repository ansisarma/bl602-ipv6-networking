extern "C" {
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <bl_uart.h>
#include <hal_board.h>
}

extern "C" void task_wifi(void *param);

#define HEAP_SIZE (40 * 1024)
static uint8_t ucHeap[HEAP_SIZE] __attribute__((aligned(8)));

extern "C" void bfl_main(void)
{
    bl_uart_init(0, 16, 7, 255, 255, 2000000);
    hal_board_cfg(0);
    
    printf("\r\n========================================\r\n");
    printf("PineCone #2: IPv6 UDP CLIENT (STA mode)\r\n");
    printf("========================================\r\n");
    
    // Initialize heap
    HeapRegion_t xHeapRegions[] = {
        { ucHeap, HEAP_SIZE },
        { NULL, 0 }
    };
    vPortDefineHeapRegions(xHeapRegions);
    
    printf("[MAIN] Free heap: %d bytes\r\n", xPortGetFreeHeapSize());
    
    // Create WiFi task
    BaseType_t rc = xTaskCreate(
        task_wifi,
        "wifi",
        4096,
        nullptr,
        15,
        nullptr
    );
    
    if (rc != pdPASS) {
        printf("[MAIN] Failed to create WiFi task!\r\n");
        while (1) {}
    }
    
    printf("[MAIN] Starting scheduler...\r\n");
    vTaskStartScheduler();
    
    printf("[MAIN] ERROR: Scheduler exited!\r\n");
    while (1) {}
}