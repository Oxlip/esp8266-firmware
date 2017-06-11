#include <espressif/esp_common.h>
#include <espressif/spi_flash.h>

#include <string.h>
#include <FreeRTOS.h>
#include <task.h>

#include "common_export.h"

#define SETTINGS_START_SEC        0x7b

int flash_write_settings(aura_settings_t *settings)
{
	int result;

	// before writing the sector has to be erased.
    sdk_spi_flash_erase_sector(SETTINGS_START_SEC);
    result = sdk_spi_flash_write(SETTINGS_START_SEC * SPI_FLASH_SEC_SIZE,
             			         (uint32_t *)settings, sizeof(aura_settings_t));
    if (result) {
    	printf("Failed to write to flash.");
    }

    return result;
}

int flash_read_settings(aura_settings_t *settings)
{
	int result;

	result = sdk_spi_flash_read(SETTINGS_START_SEC * SPI_FLASH_SEC_SIZE,
                       		  	(uint32_t *)settings, sizeof(aura_settings_t));
	if (result) {
		printf("Flash read failed %d\n", result);
	}
	return result;
}
