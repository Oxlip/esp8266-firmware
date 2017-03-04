#include <espressif/esp_common.h>
#include "esp/uart.h"

#include <string.h>
#include <FreeRTOS.h>
#include <task.h>

#include "common_export.h"

#define GPIO_LED 2

void user_init(void) {
    uart_set_baud(0, 115200);
    printf("SDK version: %s, free heap %u\n", sdk_system_get_sdk_version(),
            xPortGetFreeHeapSize());

    gpio_enable(GPIO_LED, GPIO_OUTPUT);
    gpio_write(GPIO_LED, 1);

    spi_task_init();

    wifi_init();

    rest_api_init();

    aws_iot_init();
}
