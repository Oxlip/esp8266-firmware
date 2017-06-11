#include <FreeRTOS.h>
#include <task.h>
#include <string.h>
#include <stdio.h>
#include <espressif/esp_system.h>

#include "http_handler.h"
#include "common_export.h"
#include "jsmn/jsmn.h"

/** Helper function to fill http_response_t
 */
static void fill_response(http_response_t *response,
                          int status, char *reason,
                          char *content_type, char *body)
{
    response->status = status;
    response->reason = reason;
    response->content_type = content_type;
    response->body = body;
}

/** Helper function compare whether given jsmn token's key matches with
 *  given key.
 */
static int json_key_equal(const char *json, jsmntok_t *tok, const char *s)
{
    if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

/* Extracts and copies string of given token from the json string.
 */
static void json_extract_string(const char *json, jsmntok_t *tok, size_t max_len,
                                char *result)
{
    int len = tok->end - tok->start;
    if (len > MAX_SSID_SIZE - 1) {
        len = MAX_SSID_SIZE - 1;
    }
    strncpy(result, json + tok->start, len);
    result[len] = 0;
}

/*
 * Hanlder for URL : /networks
 *
 * Returns list of ssid and rssi in JSON format.
 */
static int GET_networks(http_request_t *request, http_response_t *response)
{
    fill_response(response, 200, "OK", "application/json",
                  wifi_ap_get_ssid_list_as_json());
    return 0;
}

/*
 * Hanlder for URL : /setting
 *
 * Saves the given settings in flash.
 * Expects SSID and Password in the PUT request.
 * Implicitly sets the next boot mode as station.
 *
 * Once the settings is stored usually a reboot command has to be issued from
 * SPI channel.
 */
static int PUT_settings(http_request_t *request, http_response_t *response)
{
    aura_settings_t settings;
    int i, keys, result;
    char ssid[MAX_SSID_SIZE] = {0};
    char password[MAX_WIFI_PWD] = {0};

    jsmn_parser parser;
    jsmntok_t tokens[50];

    jsmn_init(&parser);
    keys = jsmn_parse(&parser, request->body, strlen(request->body),
                      tokens, sizeof(tokens) / sizeof(tokens[0]));
    if (keys < 0) {
        fill_response(response, 404, "Invalid request", "plain/text", "Invalid JSON");
        return 0;
    }
    /* Loop over all keys of the root object */
    for (i = 1; i < keys; i++) {
        if (json_key_equal(request->body, &tokens[i], "ssid") == 0) {
            json_extract_string(request->body, &tokens[i+1], MAX_SSID_SIZE, ssid);
            i++;
        }else if (json_key_equal(request->body, &tokens[i], "password") == 0) {
            json_extract_string(request->body, &tokens[i+1], MAX_WIFI_PWD, password);
            i++;
        } else {
            printf("Unexpected key: %.*s\n", tokens[i].end - tokens[i].start,
                    request->body + tokens[i].start);
        }
    }

    // read old settings to preserve values other than that we are touching.
    flash_write_settings(&settings);
    strcpy(settings.ssid, ssid);
    strcpy(settings.password, password);
    settings.mode = BOOT_WIFI_MODE_STATION;
    result = flash_write_settings(&settings);
    if (result) {
        fill_response(response, 500, "Internal failure", "plain/text", "Flash write failed");
    } else{
        fill_response(response, 200, "OK", "plain/text", "Success");
    }

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
