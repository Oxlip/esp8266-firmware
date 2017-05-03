#include "espressif/esp_common.h"

#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <ssid_config.h>
#include <dhcpserver.h>

#include "common_export.h"

int wifi_alive = 0;

// cached SSIDs in JSON format.
static char *json_ssid_list = NULL;

char * wifi_ap_get_ssid_list_as_json()
{
    return json_ssid_list;
}

/* Create JSON from given SSID list
 */
static char *create_json_from_ssid_list(struct sdk_bss_info *ssid_list)
{
    struct sdk_bss_info *s = ssid_list;
    int remaining_ssid, total_ssid = 0;
    char *json_output;

    while(s) {
        total_ssid++;
        s = s->next.stqe_next;
    }
    s = ssid_list;
    remaining_ssid = total_ssid;

    size_t single_entry_size, total_size;
    single_entry_size = sizeof("{") + \
                        sizeof("\"ssid\": ") + 32 + sizeof(",\n") + \
                        sizeof("\"rssi\": ") + sizeof("NN") + sizeof("\n") + \
                        sizeof("}, ");
    total_size = sizeof "[" + (single_entry_size * total_ssid) + sizeof "]" + 1;
    json_output = (char *) malloc(total_size);
    if (json_output == NULL) {
        printf("No memory(%d * %d = ~%d bytes) to create JSON", single_entry_size, total_ssid, total_size);
        return NULL;
    }

    strcpy(json_output, "[\n");
    while (s != NULL)
    {
        char temp[single_entry_size];
        char ssid[33]; // max SSID length + zero byte
        size_t len = strlen((const char *)s->ssid);
        memcpy(ssid, s->ssid, len);
        ssid[len] = 0;

        snprintf(temp, sizeof(temp), "{\"ssid\":\"%s\" , \"rssi\" :%d}",
                                     ssid, s->rssi);
        strcat(json_output, temp);
        if (remaining_ssid > 1) {
            strcat(json_output, ",");
        }
        remaining_ssid --;
        strcat(json_output, "\n");

        s = s->next.stqe_next;
    }
    strcat(json_output, "]");

    return json_output;
}


static void wifi_station_scan_done_cb(void *arg, sdk_scan_status_t status)
{
    if (status != SCAN_OK)
    {
        printf("Error: WiFi scan failed\n");
        return;
    }

    struct sdk_bss_info *bss = (struct sdk_bss_info *)arg;
    // first one is invalid
    bss = bss->next.stqe_next;

    // free existing JSON buffer
    if (json_ssid_list) {
        free(json_ssid_list);
    }
    // create JSON output from found SSID list
    json_ssid_list = create_json_from_ssid_list(bss);
}

static void start_ssid_scan()
{
    printf("Starting SSID Scan");
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_scan(NULL, wifi_station_scan_done_cb);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    printf("Scan complete");
}

static void wifi_station_task(void *pvParameters) {
    uint8_t status = 0;
    uint8_t retries = 30;
    struct sdk_station_config config = { .ssid = WIFI_SSID, .password =
            WIFI_PASS, };

    printf("%s: Connecting to WiFi\n\r", __func__);
    sdk_wifi_set_opmode(STATION_MODE);
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
static void wifi_ap_task()
{
    start_ssid_scan();

    printf("Starting in AP mode");
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
    sdk_wifi_softap_set_config_current(&ap_config);

    ip_addr_t first_client_ip;
    IP4_ADDR(&first_client_ip, 172, 16, 0, 2);
    dhcpserver_start(&first_client_ip, 4);
    rest_api_init();

    while(1) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

void wifi_start_ap(char *ssid, char *password) {
    xTaskCreate(&wifi_ap_task, "wifi_ap", 512, NULL, 2, NULL);
}

void wifi_start_station(char *ssid, char *password) {
    xTaskCreate(&wifi_station_task, "wifi_task", 256, NULL, 2, NULL);
}
