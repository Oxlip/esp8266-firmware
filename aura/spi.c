#include <espressif/esp_common.h>
#include "esp/uart.h"
#include "esp/spi.h"

#include <string.h>
#include <FreeRTOS.h>
#include <task.h>


#define delay_ms(ms) vTaskDelay((ms) / portTICK_PERIOD_MS)

static void spi_task (void *pvParameters) {
    const spi_settings_t my_settings = {
     .mode = SPI_MODE0,
     .freq_divider = SPI_FREQ_DIV_125K,
     .msb = false,
     .endianness = SPI_LITTLE_ENDIAN,
     .minimal_pins = false
    };

    const char out_data [] = "HELLO";
    char in_data [sizeof(out_data)];

    spi_set_settings(1, &my_settings);
    while (true)
    {
        spi_transfer(1, out_data, in_data, sizeof(out_data), SPI_8BIT);
        in_data[sizeof(in_data) - 1] = 0;
        printf("Sent: %s, got: %s (", out_data, in_data);
        for (size_t i = 0; i < sizeof(in_data); i ++)
            printf("0x%02x ", in_data[i]);
        printf(")\n");
        delay_ms(1000);
    }
}

void spi_task_init(void) {
    xTaskCreate(&spi_task, "spi_task", 2048, NULL, 2, NULL);
}