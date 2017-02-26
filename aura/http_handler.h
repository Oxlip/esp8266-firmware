#ifndef HTTP_HANDLER_H

#define HTTP_HANDLER_H

typedef enum http_request_type {
    REQUEST_TYPE_GET,
    REQUEST_TYPE_HEAD,
    REQUEST_TYPE_PUT,
    REQUEST_TYPE_POST,
    REQUEST_TYPE_DELETE,
    REQUEST_TYPE_CONNECT,
}http_request_type_t;

typedef struct http_request {
    int type;
    char *url;
    char *headers;
    char *body;
} http_request_t;

typedef struct http_response {
    int status;
    char *reason;
    char *content_type;
    char *body;
} http_response_t;

typedef int (*http_handler_fn)(http_request_t *request, http_response_t *response);

typedef struct http_req_handler
{
    http_request_type_t type;
    char* url;
    http_handler_fn fn;
} http_req_handler_t;

int http_listen_forever(int port, http_req_handler_t *handlers, int handler_count);

#endif