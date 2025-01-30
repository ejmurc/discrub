#ifndef OPENSSL_HELPERS_H
#define OPENSSL_HELPERS_H

#include <ctype.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <stdint.h>

struct Header {
  char* key;
  char* value;
  struct Header* next;
};

struct HTTPResponse {
  char* raw;
  char* body;
  struct Header* headers;
  uint16_t code;
};

SSL_CTX* create_ssl_ctx();

BIO* create_bio(SSL_CTX* ctx, const char* hostname);

struct HTTPResponse* perform_http_request(BIO* bio, const char* request);

void free_http_response(struct HTTPResponse* response);

#endif
