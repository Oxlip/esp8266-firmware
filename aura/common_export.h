#ifndef COMMON_EXPORT_H

void wifi_init();
void spi_task_init();
void rest_api_init();
void aws_iot_init();

extern int wifi_alive;

#endif

