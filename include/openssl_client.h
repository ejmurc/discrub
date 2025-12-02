#ifndef OPENSSL_CLIENT_H
#define OPENSSL_CLIENT_H

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <ctype.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct Header {
  char *key;
  char *value;
  struct Header *next;
};

struct HTTPResponse {
  char *raw;
  char *body;
  struct Header *headers;
  uint16_t code;
};

SSL_CTX *ssl_ctx_new(void);

SSL *ssl_new(SSL_CTX *ctx, const char *hostname, const char *port);

char *ssl_request(SSL *ssl, const char *method, const char *path, const char *hostname,
                  const char *body);

struct HTTPResponse *parse_raw_response(char *raw);

void free_http_response(struct HTTPResponse *response);

void ssl_free(SSL *ssl);

#endif
