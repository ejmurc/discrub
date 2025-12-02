#include "credentials.h"
#include "discord_client.h"
#include "log.h"
#include "openssl_client.h"

int main(void) {
  const char *appname = "discrub";
  char *uid = NULL;
  char *token = NULL;
  char *credentials_filepath = NULL;
  LOG_INFOF("App Password (press Enter to skip cache): ");
  char *app_password = get_password();
  if (app_password && strlen(app_password) > 0) {
    credentials_filepath = get_credentials_filepath(appname);
    if (!credentials_filepath) {
      LOG_ERR("Failed to get credentials filepath");
      free(app_password);
      return 1;
    }
    LOG_INFO("Loading credentials from '%s'", credentials_filepath);
    char *credentials = load_credentials(credentials_filepath, app_password);
    if (credentials) {
      char *uid_str = strtok(credentials, "|");
      char *token_str = strtok(NULL, "|");
      if (uid_str && token_str) {
        uid = strdup(uid_str);
        token = strdup(token_str);
        if (!uid || !token) {
          free(uid);
          free(token);
          uid = NULL;
          token = NULL;
          LOG_ERR("Failed to allocate memory for credentials");
        }
      }
      free(credentials);
    }
    if (!token) {
      LOG_ERR("Failed to load credentials from cache");
    } else {
      LOG_OK("Loaded credentials successfully");
    }
  }
  SSL_CTX *ctx = ssl_ctx_new();
  if (!ctx) {
    LOG_ERR("Failed to create SSL context");
    free(credentials_filepath);
    free(app_password);
    return 1;
  }
  SSL *ssl = ssl_new(ctx, "discord.com", "443");
  if (!ssl) {
    LOG_ERR("Failed to connect to domain");
    SSL_CTX_free(ctx);
    free(credentials_filepath);
    free(app_password);
    return 1;
  }
  LOG_OK("Connected to discord.com:443");
  if (!token) {
    LOG_INFO("Trying Discord authentication");
    LOG_INFOF("Email: ");
    char *email = get_email();
    if (!email) {
      ssl_free(ssl);
      SSL_CTX_free(ctx);
      free(credentials_filepath);
      free(app_password);
      return 1;
    }
    flush_stdin();
    LOG_INFOF("Password: ");
    char *password = get_password();
    if (!password) {
      ssl_free(ssl);
      SSL_CTX_free(ctx);
      free(credentials_filepath);
      free(app_password);
      return 1;
    }
    if (discord_login(ssl, email, password, &uid, &token)) {
      LOG_ERR("Failed to authenticate with Discord");
      free(password);
      ssl_free(ssl);
      SSL_CTX_free(ctx);
      free(credentials_filepath);
      free(app_password);
      return 1;
    }
    free(password);
    if (!app_password || strlen(app_password) == 0) {
      LOG_INFOF("Cache credentials? Enter App Password (press Enter to skip): ");
      free(app_password);
      app_password = get_password();
      if (app_password && strlen(app_password) > 0) {
        credentials_filepath = get_credentials_filepath(appname);
        if (!credentials_filepath) {
          LOG_ERR("Failed to get credentials filepath");
        }
      }
    }
    if (app_password && strlen(app_password) > 0 && credentials_filepath) {
      const char *credentials_fmt = "%s|%s";
      int credentials_len = snprintf(NULL, 0, credentials_fmt, uid, token);
      char *credentials = malloc((size_t)credentials_len + 1);
      if (!credentials) {
        LOG_ERR("Failed to allocate memory for credentials");
      } else {
        if (snprintf(credentials, (size_t)credentials_len + 1, credentials_fmt, uid, token) !=
            credentials_len) {
          LOG_ERR("Failed to format credentials");
        } else if (save_credentials(credentials_filepath, credentials, app_password)) {
          LOG_WARN("Failed to cache credentials at '%s'", credentials_filepath);
        } else {
          LOG_OK("Credentials cached successfully");
        }
        free(credentials);
      }
    }
  }
  free(credentials_filepath);
  free(app_password);
  LOG_INFO("%s %s", token, uid);
  free(uid);
  free(token);
  ssl_free(ssl);
  SSL_CTX_free(ctx);
  return 0;
}
