#include <stdio.h>
#include <yyjson.h>

#include "credentials.h"
#include "discord_client.h"
#include "log.h"
#include "openssl_client.h"

int main(void) {
    LOG_OK("TCP connection established to discord.com:443");
    SSL_CTX *ctx = ssl_ctx_new();
    if (!ctx) {
        LOG_ERR("Failed to create SSL context\n");
        return 1;
    }
    SSL *ssl = ssl_new(ctx, "discord.com", "443");
    if (!ssl) {
        LOG_ERR("Failed to connect to domain");
        SSL_CTX_free(ctx);
        return 1;
    }
    LOG_OK("TLS handshake completed");
    const char *appname = "discrub";
    char *filepath = credentials_filepath(appname);
    LOG_OK("Inspecting credential cache: %s", filepath);
    char *credentials = load_credentials(filepath);
    if (credentials) {
        LOG_OK("Cached credentials loaded successfully");
        printf("%s\n", credentials);
        free(credentials);
    } else {
        LOG_INFO("No cached credentials present; authentication required");
    }
    ssl_free(ssl);
    SSL_CTX_free(ctx);
}
