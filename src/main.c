#include <stdio.h>
#include <yyjson.h>

#include "credentials.h"
#include "discord_client.h"
#include "log.h"
#include "openssl_client.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    LOG_ERR("Usage: %s <password>", argv[0]);
    return 1;
  }
  const char *app_password = argv[1];
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
  LOG_OK("Connected to discord.com:443");
  const char *appname = "discrub";
  char *filepath = credentials_filepath(appname);
  LOG_INFO("Loading credentials from '%s'", filepath);
  char *credentials = load_credentials(filepath, app_password);
  free(filepath);
  if (credentials) {
    LOG_OK("Loaded credentials successfully");
    printf("%s\n", credentials);
    free(credentials);
  } else {
    LOG_WARN("Credentials not found or invalid password");
  }
  ssl_free(ssl);
  SSL_CTX_free(ctx);
}
