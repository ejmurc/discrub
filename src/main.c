#include <stdio.h>
#include <yyjson.h>

#include "credentials.h"
#include "discord_client.h"
#include "log.h"
#include "openssl_client.h"

int main(int argc, char **argv) {
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
  if (argc < 2) {

  } else {
    char *filepath = credentials_filepath(appname);
    LOG_INFO("Checking cache for authentication credentials at '%s'", filepath);
    char *credentials = load_credentials(filepath, argv[1]);
    free(filepath);
    if (credentials) {
      LOG_OK("Cached credentials loaded successfully");
      printf("%s\n", credentials);
      free(credentials);
    } else {
      LOG_INFO("Failed to load credentials from cache");
    }
  }
  ssl_free(ssl);
  SSL_CTX_free(ctx);
}
