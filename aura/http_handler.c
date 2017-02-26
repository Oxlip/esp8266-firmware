#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "http_handler.h"

/*
 * Find handler for given type and url in the given handler array.
 * Returns:
 *   Handler - if a matching handler found.
 *   NULLL - if not found.
 */
static http_handler_fn
find_http_handler(http_req_handler_t *handlers, int handler_count,
                  http_request_type_t type, char *url)
{
    int i;
    for(i = 0; i < handler_count; i++) {
        if (handlers[i].type == type && strcmp(handlers[i].url, url) == 0) {
            return handlers[i].fn;
        }
    }
    return NULL;
}

/*
 * Helper function to send HTTP Response on given socket.
 */
static void
write_response(int sock, http_response_t *resp)
{
    char buffer[200];
    sprintf(buffer, "HTTP/1.0 %d OK %s\r\nContent-Type: %s\r\n\r\n%s", resp->status, resp->reason, resp->content_type, resp->body);
    write(sock, buffer, strlen(buffer));
}

/*
 * Returns True if given str pointer is exactly at end of the line.
 * End of a HTTP line is defined as CRLF.
 */

static inline int
str_is_end_of_line(char *str)
{
    return (*str == '\r' && *(str + 1) == '\n');
}

/*
 * Similar to strchr() but limits the search with in a line(CRLF).
 */
static inline char *
str_find_char_in_line(char *str, char c)
{
    while(str && *str != 0) {
        if (*str == c)
            return str;
        if (str_is_end_of_line(str))
            return NULL;
        str++;
    }
    return NULL;
}

/*
 * Returns starting location of next line.
 */
static inline char *
str_find_next_line(char *str)
{
    while(str && *str != 0) {
        if (str_is_end_of_line(str))
            return str + 2;
        str++;
    }
    return NULL;
}

/*
 * Returns request body.
 */
static char *
find_request_body(char *headers)
{
    char *body = str_find_next_line(headers);
    while(body) {
        if (str_is_end_of_line(body)) {
            return body + 2;
        }
        body = str_find_next_line(body);
    }
    return NULL;
}

/*
 * Parses given HTTP request string and fills the fields in request structure.
 */
static int
parse_request(char *req, http_request_t *request)
{
    char *url_start, *url_end, *headers_start, *headers_end;
    url_start = str_find_char_in_line(req, ' ');
    if (url_start == NULL) {
        return 1;
    }
    *url_start = 0;
    request->url = url_start + 1;

    url_end = str_find_char_in_line(url_start + 1, ' ');
    if (url_end == NULL) {
        return 2;
    }
    *url_end = 0;

    if (strcasecmp(req, "GET") == 0) {
        request->type = REQUEST_TYPE_GET;
    } else if (strcasecmp(req, "HEAD") == 0) {
        request->type = REQUEST_TYPE_HEAD;
    } else if (strcasecmp(req, "PUT") == 0) {
        request->type = REQUEST_TYPE_PUT;
    } else if (strcasecmp(req, "POST") == 0) {
        request->type = REQUEST_TYPE_POST;
    } else if (strcasecmp(req, "DELETE") == 0) {
        request->type = REQUEST_TYPE_DELETE;
    } else if (strcasecmp(req, "CONNECT") == 0) {
        request->type = REQUEST_TYPE_CONNECT;
    }

    headers_start = str_find_next_line(url_end + 1);
    if (headers_start) {
        *(headers_start - 1) = 0;
    }
    request->headers = headers_start;
    request->body = NULL;
    if (headers_start) {
        headers_end = find_request_body(headers_start);
        if (headers_end) {
            *(headers_end - 1) = 0;
            request->body = headers_end;
        }
    }
    return 0;
}


/*
 * Handles a single request.
 */
static void
handle_http_request(int sock, http_req_handler_t *handlers, int handler_count)
{
    #define REQ_BUF_SIZE 1000
    char *buffer = malloc(REQ_BUF_SIZE);
    int ret;
    http_request_t request;
    http_response_t response;

    ret = read(sock, buffer, REQ_BUF_SIZE);
    ret = parse_request(buffer, &request);
    if (ret) {
        return;
    }

    printf("type %d url = [%s]", request.type, request.url);
    printf("Headers = [%s]", request.headers);
    printf("Body = %s\n", request.body);

    http_handler_fn fn = find_http_handler(handlers, handler_count,
                                           request.type, request.url);
    if (fn) {
        fn(&request, &response);
        write_response(sock, &response);
    } else {
        response.status = 404;
        response.reason = "Not Found";
        response.content_type = "plain/text";
        write_response(sock, &response);
    }

    free(buffer);
}


/*
 * Listens forever on the given port and calls appropraiate handlers.
 */
int http_listen_forever(int port, http_req_handler_t *handlers, int handler_count)
{
    socklen_t length;
    int listen_sock=0;
    static struct sockaddr_in cli_addr;
    static struct sockaddr_in serv_addr;

    printf("httpd:: Starting on port %d", port);

    listen_sock = socket(AF_INET, SOCK_STREAM,0);
    if(listen_sock <0) {
        printf("httpd:: socket() failed");
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr  = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

	int true = 1;
	setsockopt(listen_sock,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int));

    if(bind(listen_sock, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0) {
        printf("httpd:: bind() failed");
        return 1;
    }
    if(listen(listen_sock, 64) <0) {
        printf("httpd:: listen() failed");
        return 0;
    }

    while(1) {
        length = sizeof(cli_addr);
        int sock;
        sock = accept(listen_sock, (struct sockaddr *)&cli_addr, &length);
        if(sock < 0) {
            printf("httpd:: accpet() failed");
            continue;
        }

        handle_http_request(sock, handlers, handler_count);
        shutdown(sock, 2);
        close(sock);
    }
}
