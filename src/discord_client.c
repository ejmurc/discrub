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
    free(body);
    return 1;
  }
  char *raw_response = ssl_request(ssl, "POST", "/api/v9/auth/login", "discord.com", body);
  free(body);
  if (!raw_response) {
    return 1;
  }
  struct HTTPResponse *response = parse_raw_response(raw_response);
  if (!response) {
    return 1;
  }
  yyjson_doc *doc = yyjson_read(response->body, strlen(response->body), 0);
  if (!doc) {
    free_http_response(response);
    return 1;
  }
  yyjson_val *root = yyjson_doc_get_root(doc);
  if (!root || !yyjson_is_obj(root)) {
    yyjson_doc_free(doc);
    free_http_response(response);
    return 1;
  }
  yyjson_val *uid_val = yyjson_obj_get(root, "user_id");
  yyjson_val *token_val = yyjson_obj_get(root, "token");
  if (!uid_val || !token_val) {
    yyjson_doc_free(doc);
    free_http_response(response);
    return 1;
  }
  const char *uid_str = yyjson_get_str(uid_val);
  const char *token_str = yyjson_get_str(token_val);
  if (!uid_str || !token_str) {
    yyjson_doc_free(doc);
    free_http_response(response);
    return 1;
  }
  *uid = strdup(uid_str);
  if (!*uid) {
    yyjson_doc_free(doc);
    free_http_response(response);
    return 1;
  }
  *token = strdup(token_str);
  if (!*token) {
    free(*uid);
    *uid = NULL;
    yyjson_doc_free(doc);
    free_http_response(response);
    return 1;
  }
  yyjson_doc_free(doc);
  free_http_response(response);
  return 0;
}
