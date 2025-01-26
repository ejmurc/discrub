#include "discrub_client.h"

struct LoginResponse* discrub_login(BIO* bio, const char* email,
                                    const char* password) {
  if (bio == NULL || email == NULL || password == NULL) {
    fprintf(stderr, "discrub_login: Null argument(s)\n");
    return NULL;
  }
  const char* request_fmt =
      "POST /api/v9/auth/login HTTP/1.1\r\n"
      "Host: discord.com\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: %d\r\n"
      "Connection: close\r\n"
      "\r\n"
      "{"
      "\"gift_code_sku_id\":null,"
      "\"login\":\"%s\","
      "\"login_source\":null,"
      "\"password\":\"%s\","
      "\"undelete\":false"
      "}";
  size_t json_size = snprintf(NULL, 0,
                              "{"
                              "\"gift_code_sku_id\":null,"
                              "\"login\":\"%s\","
                              "\"login_source\":null,"
                              "\"password\":\"%s\","
                              "\"undelete\":false"
                              "}",
                              email, password);
  size_t request_size =
      snprintf(NULL, 0, request_fmt, json_size, email, password) + 1;
  char* request_string = malloc(request_size);
  if (request_string == NULL) {
    fprintf(stderr,
            "discrub_login: Memory allocation error with request_string\n");
    return NULL;
  }
  snprintf(request_string, request_size, request_fmt, json_size, email,
           password);
  struct HTTPResponse* response = perform_http_request(bio, request_string);
  free(request_string);
  if (response == NULL) {
    return NULL;
  }
  if (response->code != 200) {
    fprintf(stderr, "discrub_login: Response code was %hu\n", response->code);
    free_http_response(response);
    return NULL;
  }
  struct LoginResponse* login_response = malloc(sizeof(struct LoginResponse));
  if (login_response == NULL) {
    perror("Failed to allocate memory for login_response");
    free_http_response(response);
    return NULL;
  }
  enum JsonError err = JSON_ENOERR;
  struct JsonToken* body_json = jsontok_parse(response->body, &err);
  if (err != JSON_ENOERR) {
    fprintf(stderr, "Error parsing response JSON (%s): %s\n",
            jsontok_strerror(err), response->body);
    free(login_response);
    free_http_response(response);
    return NULL;
  }
  size_t i;
  char *uid = NULL, *token = NULL;
  for (i = 0; i < body_json->as_object->count; i++) {
    if (uid != NULL && token != NULL) {
      break;
    }
    struct JsonEntry* entry = body_json->as_object->entries[i];
    if (entry->value->type != JSON_STRING) {
      continue;
    }
    if (uid == NULL && strcmp(entry->key, "user_id") == 0) {
      uid = strdup(entry->value->as_string);
    } else if (token == NULL && strcmp(entry->key, "token") == 0) {
      token = strdup(entry->value->as_string);
    }
  }
  jsontok_free(body_json);
  if (uid == NULL || token == NULL) {
    if (uid) {
      free(uid);
    }
    if (token) {
      free(token);
    }
    free(login_response);
    free_http_response(response);
    return NULL;
  }
  login_response->uid = uid;
  login_response->token = token;
  return login_response;
}
