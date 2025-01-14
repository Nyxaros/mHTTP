#ifndef GHTTPLIB_H
#define GHTTPLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HTTP_PORT    80
//#define HTTP_GET     "GET"
//#define HTTP_POST    "POST"
#ifdef DEBUG
#define DPRINT(fmt, ...) fprintf(stderr, "[DEBUG] " fmt "\n" ##__VA_ARGS__)
#else
#define DPRINT(fmt, ...) ()
#endif

typedef uint16_t port_t;

typedef enum {
	HTTP_GET, 
	HTTP_POST,
} method_t ;

typedef struct {
    char *key;
    char *value;
} header_t;

typedef struct {
    char *method; /* GET/POST */
    char *path; 
    char *host; /* IPv4 support only */
    header_t *headers;
    int header_count;
    char *body;
    size_t body_len;
    //int timeout; /* Not yet implemented */
} req_t;

typedef struct {
    char *version;
    int status_code;
    char *status_message;
    header_t *headers;
    int header_count;
    char *body;
    size_t body_len;
    
    char *raw; // raw response 
} resp_t;

typedef struct {
    req_t *request;
    resp_t *response;
    char *packet;
    size_t packet_len;
    int sockfd;
} http_ctx_t;

// Function declarations
req_t *req_init(method_t , const char *);
void req_free(req_t *);
resp_t *resp_init(void);
void resp_free(resp_t *);

// Main library functions
http_ctx_t *mhttp_init_ctx(void);
void http_set_body(http_ctx_t *ctx, const char *body, size_t body_len); // Set body field.
void http_set_path(http_ctx_t *, const char *);
void http_set_host(http_ctx_t *ctx, const char *host); // Sets host field.
void http_set_method(http_ctx_t *ctx, method_t ); // Sets method field.
void http_add_header(http_ctx_t *ctx, const char *key, const char *value); 
void http_build_packet(http_ctx_t *ctx);
void http_req_send(http_ctx_t *ctx);
void http_close(http_ctx_t *ctx);

// response functions
void http_parse_response(http_ctx_t *ctx);
void print_http_response(resp_t *);


void print_packet(http_ctx_t *ctx);


#ifdef _cplusplus
}
#endif

#endif // GHTTPLIB_H
