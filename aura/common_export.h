#ifndef COMMON_EXPORT_H

void wifi_start_ap(char *ssid, char *password);
void wifi_start_station(char *ssid, char *password);
char *wifi_ap_get_ssid_list_as_json();

void spi_task_init();
void rest_api_init();
void aws_iot_init();

extern int wifi_alive;


#define MAX_SSID_SIZE		32
#define MAX_WIFI_PWD 		63

typedef enum {
	BOOT_WIFI_MODE_STATION = 0,
	BOOT_WIFI_MODE_AP,
} boot_wifi_mode_t;

typedef struct {
	char 				ssid[MAX_SSID_SIZE];
	char    			password[MAX_WIFI_PWD];
	boot_wifi_mode_t 	mode;
	char 				ap_ssid[MAX_SSID_SIZE];
	char    			ap_password[MAX_WIFI_PWD];
}  __attribute__ ((aligned (4))) aura_settings_t;

int flash_write_settings(aura_settings_t *settings);
int flash_read_settings(aura_settings_t *settings);

#endif

