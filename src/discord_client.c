#include "discord_client.h"

int discord_login(SSL *ssl, const char *email, const char *password, char **uid, char **token) {
  if (!ssl || !email || !password || !uid || !token) {
    return 1;
  }
  const char *body_fmt = "{"
                         "\"gift_code_sku_id\":null,"
                         "\"login\":\"%s\","
                         "\"login_source\":null,"
                         "\"password\":\"%s\","
                         "\"undelete\":false"
                         "}";
  int body_size = snprintf(NULL, 0, body_fmt, email, password);
  char *body = malloc((size_t)body_size + 1);
  if (!body) {
    return 1;
  }
  if (snprintf(body, body_size + 1, body_fmt, email, password) != body_size) {
    return 1;
  }
  char *raw_response = ssl_request(ssl, "POST", "/api/v9/auth/login", "discord.com", body);
  if (!raw_response) {
    return 1;
  }
  struct HTTPResponse *response = parse_raw_response(raw_response);
  if (!response) {
    free(raw_response);
    return 1;
  }
  printf("%s\n", response->body);
  free_http_response(response);
  return 0;
}
