#include "discrub_client.h"

struct LoginResponse* discrub_login(BIO* bio, const char* username,
                                    const char* password) {
  if (bio == NULL || username == NULL || password == NULL) {
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
                              username, password);
  size_t request_size =
      snprintf(NULL, 0, request_fmt, json_size, username, password) + 1;
  char* request_string = malloc(request_size);
  if (request_string == NULL) {
    fprintf(stderr,
            "discrub_login: Memory allocation error with request_string\n");
    return NULL;
  }
  snprintf(request_string, request_size, request_fmt, json_size, username,
           password);
  struct HTTPResponse* response = perform_http_request(bio, request_string);
  free(request_string);
  if (response == NULL) {
    return NULL;
  }
  printf("Response code: %hu\n", response->code);
  printf("Headers:\n");
  struct Header* header = response->headers;
  while (header) {
    printf("['%s' '%s']\n", header->key, header->value);
    header = header->next;
  }
  printf("Body: %s\n", response->body);
  if (response->code != 200) {
    fprintf(stderr, "discrub_login: Response code was %hu\n", response->code);
    free_http_response(response);
    return NULL;
  }
  free_http_response(response);
  return NULL;
}
