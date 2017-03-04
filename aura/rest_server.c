#include <FreeRTOS.h>
#include <task.h>

#include "http_handler.h"

/*
 * Hanlder for URL : /networks
 *
 * Returns list of ssid and rssi in JSON format.
 */
static int GET_networks(http_request_t *request, http_response_t *response)
{
    response->status = 200;
    response->reason = "OK";
    response->content_type = "application/json";
    //TODO - Start wifi scanning and get the list.
    response->body = "[{\"ssid\": \"test\", \"rssi\": 3}";
    return 0;
}

static int PUT_settings(http_request_t *request, http_response_t *response)
{
    response->status = 200;
    response->reason = "OK";
    response->content_type = "plain/text";
    //TODO - parse the requst body and store the ssid and password in flash
    response->body = "Saved";
    return 0;
}

http_req_handler_t rest_handlers[] = {
    {REQUEST_TYPE_GET, "/networks", GET_networks},
    {REQUEST_TYPE_PUT, "/settings", PUT_settings}
};

static void rest_server_task (void *pvParameters) {
    http_listen_forever(80, rest_handlers, sizeof(rest_handlers) / sizeof(rest_handlers[0]));
}

void rest_api_init() {
    xTaskCreate(&rest_server_task, "rest_server_task", 2048, NULL, 2, NULL);
}
