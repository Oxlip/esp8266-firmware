#include "espressif/esp_common.h"

#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <ssid_config.h>
#include <dhcpserver.h>

#include "common_export.h"

int wifi_alive = 0;

static void wifi_station_mode_task(void *pvParameters) {
    uint8_t status = 0;
    uint8_t retries = 30;
    struct sdk_station_config config = { .ssid = WIFI_SSID, .password =
            WIFI_PASS, };

    printf("%s: Connecting to WiFi\n\r", __func__);
    sdk_wifi_set_opmode (STATION_MODE);
    sdk_wifi_station_set_config(&config);

    while (1) {
        wifi_alive = 0;

        while ((status != STATION_GOT_IP) && (retries)) {
            status = sdk_wifi_station_get_connect_status();
            printf("%s: status = %d\n\r", __func__, status);
            if (status == STATION_WRONG_PASSWORD) {
                printf("WiFi: wrong password\n\r");
                break;
            } else if (status == STATION_NO_AP_FOUND) {
                printf("WiFi: AP not found\n\r");
                break;
            } else if (status == STATION_CONNECT_FAIL) {
                printf("WiFi: connection failed\r\n");
                break;
            }
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            --retries;
        }

        while ((status = sdk_wifi_station_get_connect_status())
                == STATION_GOT_IP) {
            if (wifi_alive == 0) {
                printf("WiFi: Connected\n\r");
                wifi_alive = 1;
            }
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }

        wifi_alive = 0;
        printf("WiFi: disconnected\n\r");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

#define AP_SSID "aura"
#define AP_PSK "esp-open-rtos"
static void wifi_start_ap_mode()
{
    sdk_wifi_set_opmode(SOFTAP_MODE);
    struct ip_info ap_ip;
    IP4_ADDR(&ap_ip.ip, 172, 16, 0, 1);
    IP4_ADDR(&ap_ip.gw, 0, 0, 0, 0);
    IP4_ADDR(&ap_ip.netmask, 255, 255, 0, 0);
    sdk_wifi_set_ip_info(1, &ap_ip);

    struct sdk_softap_config ap_config = {
        .ssid = AP_SSID,
        .ssid_hidden = 0,
        .channel = 3,
        .ssid_len = strlen(AP_SSID),
        .authmode = AUTH_WPA_WPA2_PSK,
        .password = AP_PSK,
        .max_connection = 2,
        .beacon_interval = 100,
    };
    sdk_wifi_softap_set_config(&ap_config);

    ip_addr_t first_client_ip;
    IP4_ADDR(&first_client_ip, 172, 16, 0, 2);
    dhcpserver_start(&first_client_ip, 4);
}

void wifi_init()
{
    int start_in_ap_mode = 1;
    if (start_in_ap_mode) {
        wifi_start_ap_mode();
    } else {
        xTaskCreate(&wifi_station_mode_task, "wifi_task", 256, NULL, 2, NULL);
    }
}
