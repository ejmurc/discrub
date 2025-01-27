#include "discrub_client.h"

static void add_param(char** params, const char* key, const char* value) {
  if (params == NULL || key == NULL || value == NULL) {
    return;
  }
  size_t length = strlen(*params) + strlen(key) + strlen(value) + 2;
  char* new_params = realloc(*params, length);
  if (new_params == NULL) {
    return;
  }
  strcat(new_params, key);
  strcat(new_params, "=");
  strcat(new_params, value);
  strcat(new_params, "&");
  new_params[length] = '\0';
  *params = new_params;
}

static char* get_params(const struct SearchOptions* options) {
  if (options == NULL) {
    return NULL;
  }
  char* params = malloc(1);
  params[0] = '\0';
  add_param(&params, "author_id", options->author_id);
  add_param(&params, "channel_id", options->channel_id);
  add_param(&params, "content", options->content);
  add_param(&params, "mentions", options->mentions);
  add_param(&params, "include_nsfw", options->include_nsfw ? "true" : "false");
  add_param(&params, "pinned", options->pinned ? "true" : "false");
  if (options->offset) {
    char uint_str[12];
    snprintf(uint_str, sizeof(uint_str), "%lu", options->offset);
    add_param(&params, "offset", uint_str);
  }
  if (strlen(params) == 1) {
    free(params);
    return NULL;
  }
  params[strlen(params) - 1] = '\0';
  return params;
}

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

struct SearchResponse* discrub_search(BIO* bio, struct SearchOptions* options) {
  char* params = get_params(options);
  printf("%s\n", params);
  return (struct SearchResponse*)bio;
}

struct SearchOptions* options_from_json(const char* json_string) {
  struct JsonToken* options_object = NULL;
  if (json_string == NULL) {
    return NULL;
  }
  struct SearchOptions* options = malloc(sizeof(struct SearchOptions));
  if (options == NULL) {
    goto cleanup;
  }
  enum JsonError err = JSON_ENOERR;
  options_object = jsontok_parse(json_string, &err);
  if (options_object == NULL || options_object->type != JSON_OBJECT) {
    goto cleanup;
  }
  struct JsonToken* server_id =
      jsontok_get(options_object->as_object, "server_id");
  if (server_id == NULL || server_id->type != JSON_STRING) {
    goto cleanup;
  }
  struct JsonToken* channel_id =
      jsontok_get(options_object->as_object, "channel_id");
  if (channel_id == NULL || channel_id->type != JSON_STRING) {
    goto cleanup;
  }
  options->server_id = strdup(server_id->as_string);
  options->channel_id = strdup(channel_id->as_string);
  struct JsonToken* include_nsfw =
      jsontok_get(options_object->as_object, "include_nsfw");
  if (include_nsfw && include_nsfw->type == JSON_BOOLEAN) {
    options->include_nsfw = include_nsfw->as_boolean;
  }
  struct JsonToken* content = jsontok_get(options_object->as_object, "content");
  if (content && content->type == JSON_STRING) {
    options->content = strdup(content->as_string);
  }
  struct JsonToken* mentions =
      jsontok_get(options_object->as_object, "mentions");
  if (mentions && mentions->type == JSON_STRING) {
    options->mentions = strdup(mentions->as_string);
  }
  struct JsonToken* pinned = jsontok_get(options_object->as_object, "pinned");
  if (pinned && pinned->type == JSON_BOOLEAN) {
    options->pinned = pinned->as_boolean;
  }
  jsontok_free(options_object);
  return options;
cleanup:
  free(options);
  jsontok_free(options_object);
  return NULL;
}
