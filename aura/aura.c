#include <espressif/esp_common.h>
#include "esp/uart.h"

#include <string.h>
#include <FreeRTOS.h>
#include <task.h>

#include "common_export.h"

#define GPIO_LED 2

void user_init(void)
{
    int result;
    aura_settings_t settings;

    uart_set_baud(0, 115200);
    printf("SDK version: %s, free heap %u\n", sdk_system_get_sdk_version(),
            xPortGetFreeHeapSize());

    gpio_enable(GPIO_LED, GPIO_OUTPUT);
    gpio_write(GPIO_LED, 1);

    result = flash_read_settings(&settings);
    if (result == 0 && settings.mode == BOOT_WIFI_MODE_STATION)
    {
        spi_task_init();
        wifi_start_station(settings.ssid, settings.password);
        aws_iot_init();
    } else {
        wifi_start_ap(settings.ap_ssid, settings.ap_password);
    }
}
