#include <stdio.h>
#include <yyjson.h>

#include "openssl_client.h"

int main(void) {
    SSL_CTX *ctx = ssl_ctx_new();
    if (!ctx) {
        fprintf(stderr, "Failed to create SSL context\n");
        return 1;
    }
    SSL *ssl = ssl_new(ctx, "discord.com", "443");
    if (!ssl) {
        fprintf(stderr, "Failed to connect to discord\n");
        SSL_CTX_free(ctx);
        return 1;
    }
    printf("Successfully connected to discord\n");
    ssl_free(ssl);
    SSL_CTX_free(ctx);
}
