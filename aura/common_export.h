#ifndef COMMON_EXPORT_H

void wifi_start_ap(char *ssid, char *password);
void wifi_start_station(char *ssid, char *password);
char *wifi_ap_get_ssid_list_as_json();

void spi_task_init();
void rest_api_init();
void aws_iot_init();

extern int wifi_alive;

#endif

