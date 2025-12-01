#include <stdio.h>
#include <yyjson.h>

#include "credentials.h"
#include "discord_client.h"
#include "log.h"
#include "openssl_client.h"

int main(void) {
  const char *appname = "discrub";
  char *uid = NULL, *token = NULL;
  LOG_INFOF("App Password (press Enter to skip cache): ");
  char *app_password = get_password();
  if (app_password && strlen(app_password) > 0) {
    char *filepath = credentials_filepath(appname);
    LOG_INFO("Loading credentials from '%s'", filepath);
    char *credentials = load_credentials(filepath, app_password);
    uid = strdup(strtok(credentials, "\0"));
    token = strdup(strtok(NULL, "\0"));
    if (!uid || !token) {
      uid = NULL;
      token = NULL;
    }
    free(filepath);
    free(app_password);
  }
  if (!token) {
    LOG_INFO("Attempting Discord login");
    SSL_CTX *ctx = ssl_ctx_new();
    if (!ctx) {
      LOG_ERR("Failed to create SSL context");
      return 1;
    }
    SSL *ssl = ssl_new(ctx, "discord.com", "443");
    if (!ssl) {
      LOG_ERR("Failed to connect to domain");
      SSL_CTX_free(ctx);
      return 1;
    }
    LOG_OK("Connected to discord.com:443");
    LOG_INFOF("Email: ");
    char *email = get_email();
    if (!email) {
      ssl_free(ssl);
      SSL_CTX_free(ctx);
      return 1;
    }
    flush_stdin();
    LOG_INFOF("Password: ");
    char *password = get_password();
    if (!password) {
      free(email);
      ssl_free(ssl);
      SSL_CTX_free(ctx);
      return 1;
    }

    // TODO: Call Discord API to get credentials
    // uid, token = discord_login(ssl, email, password);

    free(password);
    ssl_free(ssl);
    SSL_CTX_free(ctx);
    if (!token) {
      LOG_ERR("Discord login failed");
      return 1;
    }
  }
  LOG_OK("Credentials obtained successfully");
  printf("%s %s\n", token, uid);
  free(uid);
  free(token);
  return 0;
}
