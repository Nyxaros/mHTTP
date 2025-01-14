#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../include/mhttp.h" 

req_t* req_init(method_t method,const char *host)
{
	req_t *req = malloc(sizeof(req_t));
	req->method = strdup(method ? "POST" : "GET");
	req->host = (host != NULL ? strdup(host) : NULL);
	req->path = strdup("/");
	req->headers = NULL;
	req->header_count = 0;
	req->body = NULL;
	req->body_len = 0;
	//req->timeout = 0;
	return req;
}

void req_free(req_t *req)
{
	free(req->method);
	free(req->host);
	free(req->path);
	for (int i = 0; i < req->header_count; i++) {
		free(req->headers[i].key);
		free(req->headers[i].value);
	}
	free(req->headers);
	free(req->body);
	free(req);
}

resp_t* resp_init() 
{
	resp_t *resp = malloc(sizeof(resp_t));

	resp->version = NULL;
	resp->status_code = 0;
	resp->status_message = NULL;
	resp->headers = NULL;
	resp->header_count = 0;
	resp->body = NULL;
	resp->body_len = 0;
	resp->raw = NULL;

	return resp;
}

void resp_free(resp_t *resp)
{
	for (int i = 0; i < resp->header_count; i++) {
		free(resp->headers[i].key);
		free(resp->headers[i].value);
	}
	free(resp->headers);
	free(resp->body);
	free(resp->raw);
	free(resp->status_message);
	free(resp->version);
	free(resp);
}

static void req_add_header (req_t *req, const char *key,const char *value)
{
    req->headers = realloc(req->headers, sizeof(header_t) * (req->header_count + 1));
	
    req->headers[req->header_count].key = strdup(key);
    req->headers[req->header_count].value = strdup(value);
    req->header_count++;
}

http_ctx_t *mhttp_init_ctx()
{
	http_ctx_t *ctx = malloc(sizeof(http_ctx_t));
	ctx->request = req_init(HTTP_GET, NULL); 
	ctx->response = resp_init();
	ctx->packet = NULL;
	ctx->packet_len = 0;
	ctx->sockfd = -1;
	return ctx;
}

void http_set_method(http_ctx_t *ctx,method_t method)
{
	if (ctx->request->method != NULL) {
		free(ctx->request->method);
		ctx->request->method = NULL;
	}

	ctx->request->method = strdup(method ? "POST" : "GET");
}

void http_set_path(http_ctx_t *ctx,const char *path)
{
	if (ctx->request->path != NULL) {
		free(ctx->request->path);
		ctx->request->path = NULL;
	}

	ctx->request->path = strdup(path);

}

void http_set_host(http_ctx_t *ctx,const char *host)
{
	if (ctx->request->host != NULL) {
		free(ctx->request->host);
		ctx->request->host = NULL;
	}

	ctx->request->host = strdup(host);
}

void http_add_header(http_ctx_t *ctx,const char *key,const char *value)
{
	req_add_header(ctx->request, key, value);
}

void http_set_body(http_ctx_t *ctx,const char *body,size_t body_len)
{
    if (ctx->request->body != NULL) {
	    free(ctx->request->body);
	    ctx->request->body = NULL;
    }
    ctx->request->body = strdup(ctx->request->body); //malloc(body_len);
    ctx->request->body_len = body_len;
}

void print_packet(http_ctx_t *ctx)
{
	printf("%s", ctx->packet);
}

void http_build_packet(http_ctx_t *ctx)
{
    const req_t *req = ctx->request;

    size_t initial_size = strlen(req->method) + strlen(req->path) + strlen(req->host) + 50; // Safe estimate
    ctx->packet = malloc(initial_size);
    if (!ctx->packet) {
        DPRINT("malloc failed");
		return;
    }

    size_t current_length = snprintf(ctx->packet, initial_size, 
        "%s %s HTTP/1.1\r\nHost: %s\r\n", req->method, req->path, req->host);

    // Append headers
    for (int i = 0; i < req->header_count; i++) {
        size_t header_size = strlen(req->headers[i].key) + strlen(req->headers[i].value) + 4; // ": " and "\r\n"
        ctx->packet = realloc(ctx->packet, current_length + header_size + 1);
        if (!ctx->packet) {
            DPRINT("realloc failed");
            exit(EXIT_FAILURE);
        }
        current_length += snprintf(ctx->packet + current_length, header_size + 1, 
            "%s: %s\r\n", req->headers[i].key, req->headers[i].value);
    }

    ctx->packet = realloc(ctx->packet, current_length + 3); // +3 for "\r\n\0"
    if (!ctx->packet) {
        DPRINT("realloc failed");
        exit(EXIT_FAILURE);
    }
    current_length += snprintf(ctx->packet + current_length, 3, "\r\n");

    if (req->body && req->body_len > 0) {
        ctx->packet = realloc(ctx->packet, current_length + req->body_len + 1);
        if (!ctx->packet) {
            DPRINT("realloc failed");
            exit(EXIT_FAILURE);
        }
        memcpy(ctx->packet + current_length, req->body, req->body_len);
        current_length += req->body_len;
        ctx->packet[current_length] = '\0';
    }

    ctx->packet_len = current_length;
}

void http_req_send(http_ctx_t *ctx)
{
	struct sockaddr_in addr = {0};

	ctx->sockfd = socket(AF_INET, SOCK_STREAM, 0);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(HTTP_PORT);

	addr.sin_addr.s_addr = inet_addr(ctx->request->host);

	if (connect(ctx->sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		DPRINT("failed connecting to host\n");
		return;
	}

	send(ctx->sockfd, ctx->packet, ctx->packet_len, 0);

	char buffer[1024];
	ssize_t bytes_received = recv(ctx->sockfd, buffer, sizeof(buffer) - 1, 0);
	if (bytes_received > 0) 
	{
		buffer[bytes_received] = 0x00;

		if (ctx->response->raw != NULL)
			free(ctx->response->raw);
		ctx->response->raw = strdup(buffer);

		//printf("Raw Response: %s\n", ctx->response->raw);
	}
	
	close(ctx->sockfd);
}

static char* get_line(char *start)
{
    char *end = strstr(start, "\r\n");
    if (!end)
        return NULL;

    size_t len = end - start;
    return strndup(start, len);
}

static void parse_status_line(resp_t *response,char *line)
{
    if (!line) return;

    char *version = strtok(line, " ");
    char *status_code_str = strtok(NULL, " ");
    char *status_message = strtok(NULL, "\r\n");

    if (!version || !status_code_str || !status_message) {
        fprintf(stderr, "[parse_status_line] Invalid HTTP status line: %s\n", line);
        response->version = strdup("HTTP/1.1");  // Default to HTTP/1.1
        response->status_code = 0;
        response->status_message = strdup("Invalid Status");
        return;
    }

    response->version = strdup(version);
    response->status_code = atoi(status_code_str);
    response->status_message = strdup(status_message);
}



static void parse_header_line(header_t *header,char *line)
{
	char *colon_pos = strstr(line, ": ");
	if (colon_pos) {
		size_t key_len = colon_pos - line;
		header->key = malloc(key_len + 1);
		strncpy(header->key, line, key_len);
		header->key[key_len] = 0;

		header->value = strdup(colon_pos + 2);
	}
}

static void parse_headers(resp_t *response,char *header_str)
{
	response->headers = NULL;
	response->header_count = 0;

	char *line = get_line(header_str);
	while (line && strlen(line) > 0) {
		response->header_count++;
		response->headers = realloc(response->headers, sizeof(header_t) * response->header_count);
		if (response->headers == NULL) {
			perror("realloc");
			return;
		}
		
		parse_header_line(&response->headers[response->header_count - 1], line);
		header_str += strlen(line) + 2; // move to the next header line
		free(line);
		line = get_line(header_str);
	}

	if (line != NULL) free(line);
}

void http_parse_response(http_ctx_t *ctx)
{
	char *raw = ctx->response->raw;

	char *header_end = strstr(raw, "\r\n\r\n");
	if (!header_end) {
		DPRINT("Invalid http response: missing header/body delimiter\n");
		return ;
	}

	char *status_line = get_line(raw);
	parse_status_line(ctx->response, status_line);

    parse_headers(ctx->response, raw + strlen(status_line) + 2);  // Skip status line and \r\n

	if (ctx->response->body != NULL) {
		free(ctx->response->body);	
		ctx->response->body = NULL;
	}

	// extract body
	if (*(header_end + 4) != '\0') {
		ctx->response->body = strdup(header_end + 4);  // Body starts after \r\n\r\n
	}

	free(status_line);
	status_line = NULL;
}

void print_http_response(resp_t *response)
{
    printf("HTTP Version: %s\n", response->version);
    printf("Status Code: %d\n", response->status_code);
    printf("Status Message: %s\n", response->status_message);

    printf("\nHeaders:\n");
    for (int i = 1; i < response->header_count; i++) // FIX 
        printf("%s: %s\n", response->headers[i].key, response->headers[i].value);

    if (response->body) {
        printf("\nBody:\n%s\n", response->body);
    }
}


void http_close(http_ctx_t *ctx)
{
    if (ctx->sockfd != -1) close(ctx->sockfd);

    if (ctx->request != NULL) {
        req_free(ctx->request);
        ctx->request = NULL;
    }
    if (ctx->response != NULL) {
        resp_free(ctx->response);
        ctx->response = NULL;
    }
    if (ctx->packet != NULL) {
        free(ctx->packet);
        ctx->packet = NULL;
    }
	ctx->packet_len = 0;
    free(ctx);
}
