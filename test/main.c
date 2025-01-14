#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "../include/mhttp.h"

int main() {
    http_ctx_t *ctx = mhttp_init_ctx();  	// Initialize a context

    // Build the request 
    http_set_method(ctx, HTTP_GET);		// Default method is GET if not specified
    http_set_host(ctx, "34.226.108.155");
    http_set_path(ctx, "/headers");		// Default path is "/" if not specified

    /* Headers must be specified */
    http_add_header(ctx, "User-Agent", "MinimalHTTP/1.0");
    http_add_header(ctx, "Accept", "*/*");
    
    // Build the packet
    http_build_packet(ctx);

    print_packet(ctx);

    // send the packet
    http_req_send(ctx);

    // parse response
    http_parse_response(ctx);

    print_http_response(ctx->response);

    // Cleanup
    http_close(ctx);

    return 0;
}
