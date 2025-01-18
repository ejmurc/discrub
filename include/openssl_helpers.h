#ifndef OPENSSL_HELPERS_H
#define OPENSSL_HELPERS_H

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

SSL_CTX* create_ssl_ctx();

BIO* create_bio(SSL_CTX* ctx, const char* hostname);

#endif
