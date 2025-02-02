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
  add_param(&params, "max_id", options->max_id);
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
  struct JsonToken* uid = jsontok_get(body_json->as_object, "user_id");
  struct JsonToken* token = jsontok_get(body_json->as_object, "token");
  if (uid == NULL || uid->type != JSON_STRING || token == NULL ||
      token->type != JSON_STRING) {
    free(login_response);
    free_http_response(response);
    return NULL;
  }
  login_response->uid = strdup(uid->as_string);
  login_response->token = strdup(token->as_string);
  jsontok_free(body_json);
  return login_response;
}

struct SearchResponse* discrub_search(BIO* bio, const char* token,
                                      struct SearchOptions* options) {
  if (bio == NULL || token == NULL || options == NULL) {
    fprintf(stderr, "discrub_search: Null argument(s)\n");
    return NULL;
  }
  if (options->server_id == NULL) {
    fprintf(stderr, "discrub_search: Options must include server_id\n");
    return NULL;
  }
  char* params = get_params(options);
  if (params == NULL) {
    fprintf(stderr, "discrub_search: Failed to get parameters from options\n");
    return NULL;
  }
  const char* request_fmt =
      "GET /api/v9/guilds/%s/messages/search?%s HTTP/1.1\r\n"
      "Host: discord.com\r\n"
      "Authorization: %s\r\n"
      "Connection: close\r\n"
      "\r\n";
  size_t request_size =
      snprintf(NULL, 0, request_fmt, options->server_id, params, token) + 1;
  char* request_string = malloc(request_size);
  snprintf(request_string, request_size, request_fmt, options->server_id,
           params, token);
  free(params);
  struct HTTPResponse* response = perform_http_request(bio, request_string);
  free(request_string);
  if (response == NULL) {
    return NULL;
  }
  if (response->code != 200) {
    fprintf(stderr, "discrub_search: Response code was %hu\n", response->code);
    free_http_response(response);
    return NULL;
  }
  struct SearchResponse* search_response =
      malloc(sizeof(struct SearchResponse));
  if (search_response == NULL) {
    fprintf(stderr,
            "discrub_search: Failed to allocate memory for search_response\n");
    free_http_response(response);
    return NULL;
  }
  search_response->length = 0;
  search_response->total_messages = 0;
  search_response->messages = NULL;
  enum JsonError err = JSON_ENOERR;
  struct JsonToken* body_json = jsontok_parse(response->body, &err);
  if (err != JSON_ENOERR) {
    fprintf(stderr, "Error parsing response JSON (%s): %s\n",
            jsontok_strerror(err), response->body);
    free(search_response);
    free_http_response(response);
    return NULL;
  }
  struct JsonToken* total_results =
      jsontok_get(body_json->as_object, "total_results");
  if (total_results == NULL) {
    fprintf(stderr, "total_results is null in response JSON\n%s\n",
            response->body);
    free(search_response);
    free_http_response(response);
    return NULL;
  }
  search_response->total_messages = total_results->as_number;
  struct JsonToken* messages_wrapped =
      jsontok_get(body_json->as_object, "messages");
  if (messages_wrapped == NULL) {
    fprintf(stderr, "messages_wrapped is null in response JSON\n%s\n",
            response->body);
    discrub_search_response_free(search_response);
    free_http_response(response);
    return NULL;
  }
  if (messages_wrapped->type != JSON_WRAPPED_ARRAY) {
    fprintf(stderr, "messages is not of type JSON_WRAPPED_ARRAY\n%s\n",
            response->body);
    discrub_search_response_free(search_response);
    free_http_response(response);
    return NULL;
  }
  struct JsonToken* messages = jsontok_parse(messages_wrapped->as_string, &err);
  search_response->length = messages->as_array->length;
  search_response->messages =
      malloc(search_response->length * sizeof(struct DiscordMessage*));
  if (search_response->messages == NULL) {
    fprintf(stderr,
            "Failed to allocate memory for search_response->messages\n");
    jsontok_free(body_json);
    jsontok_free(messages);
    free(search_response);
    free_http_response(response);
  }
  size_t i;
  for (i = 0; i < messages->as_array->length; i++) {
    struct JsonToken* message_container =
        jsontok_parse(messages->as_array->elements[i]->as_string, &err);
    if (err) {
      fprintf(stderr, "Failed to parse message at index %zu: %s\n%s\n", i,
              jsontok_strerror(err), response->body);
      discrub_search_response_free(search_response);
      free_http_response(response);
      return NULL;
    }
    struct JsonToken* message = jsontok_parse(
        message_container->as_array->elements[0]->as_string, &err);
    if (err) {
      fprintf(stderr, "Failed to parse message at index %zu: %s\n%s\n", i,
              jsontok_strerror(err), response->body);
      jsontok_free(body_json);
      jsontok_free(messages);
      jsontok_free(message_container);
      discrub_search_response_free(search_response);
      free_http_response(response);
      return NULL;
    }
    struct JsonToken* wrapped_author =
        jsontok_get(message->as_object, "author");
    if (wrapped_author == NULL || wrapped_author->type != JSON_WRAPPED_OBJECT) {
      fprintf(
          stderr,
          "Failed to parse message at index %zu: Missing author object\n%s\n",
          i, response->body);
      jsontok_free(body_json);
      jsontok_free(messages);
      jsontok_free(message_container);
      jsontok_free(message);
      discrub_search_response_free(search_response);
      free_http_response(response);
      return NULL;
    }
    struct JsonToken* author = jsontok_parse(wrapped_author->as_string, &err);
    struct JsonToken* author_id = jsontok_get(author->as_object, "id");
    struct JsonToken* author_username =
        jsontok_get(author->as_object, "username");
    struct JsonToken* content = jsontok_get(message->as_object, "content");
    struct JsonToken* id = jsontok_get(message->as_object, "id");
    struct JsonToken* timestamp = jsontok_get(message->as_object, "timestamp");
    if (author == NULL || author_id == NULL || author_username == NULL ||
        content == NULL || id == NULL || timestamp == NULL) {
      fprintf(stderr,
              "Failed to parse message at index %zu: Missing one or more of "
              "{author.id,author.username,content,id,timestamp\n%s\n",
              i, response->body);
      jsontok_free(body_json);
      jsontok_free(messages);
      jsontok_free(message_container);
      jsontok_free(message);
      jsontok_free(author);
      discrub_search_response_free(search_response);
      free_http_response(response);
      return NULL;
    }
    struct DiscordMessage* new_message = malloc(sizeof(struct DiscordMessage));
    if (new_message == NULL) {
      fprintf(stderr, "Failed to allocate memory for new message\n");
      jsontok_free(body_json);
      jsontok_free(messages);
      jsontok_free(message_container);
      jsontok_free(message);
      jsontok_free(author);
      discrub_search_response_free(search_response);
      free_http_response(response);
    }
    new_message->author_id = strdup(author_id->as_string);
    new_message->author_username = strdup(author_username->as_string);
    new_message->content = strdup(content->as_string);
    new_message->id = strdup(id->as_string);
    new_message->timestamp = strdup(timestamp->as_string);
    search_response->messages[i] = new_message;
    jsontok_free(message_container);
    jsontok_free(message);
    jsontok_free(author);
  }
  jsontok_free(body_json);
  jsontok_free(messages);
  free_http_response(response);
  return search_response;
}

int discrub_delete_message(BIO* bio, const char* token, const char* channel_id,
                           const char* message_id) {
  if (bio == NULL || token == NULL || channel_id == NULL ||
      message_id == NULL) {
    fprintf(stderr, "discrub_delete_message: Null argument(s)\n");
    return -1;
  }
  const char* request_fmt =
      "DELETE /api/v9/channels/%s/messages/%s HTTP/1.1\r\n"
      "Host: discord.com\r\n"
      "Authorization: %s\r\n"
      "Connection: close\r\n"
      "\r\n";
  size_t request_size =
      snprintf(NULL, 0, request_fmt, channel_id, message_id, token) + 1;
  char* request_string = malloc(request_size);
  snprintf(request_string, request_size, request_fmt, channel_id, message_id,
           token);
  struct HTTPResponse* response = perform_http_request(bio, request_string);
  free(request_string);
  if (response == NULL) {
    return -1;
  }
  if (response->code != 204) {
    struct Header* header = response->headers;
    while (header) {
      if (strcmp(header->key, "retry-after") == 0) {
        char* endptr;
        long value = strtol(header->value, &endptr, 10);
        if (*endptr == '\0' && value > 0) {
          free_http_response(response);
          return (int)value;
        }
      }
      header = header->next;
    }
    fprintf(stderr, "discrub_delete_message: Response code was %hu\n",
            response->code);
    free_http_response(response);
    return -1;
  }
  free_http_response(response);
  return 0;
}

void discrub_search_response_free(struct SearchResponse* search_response) {
  if (search_response == NULL) {
    return;
  }
  size_t i;
  for (i = 0; i < search_response->length; i++) {
    struct DiscordMessage* message = search_response->messages[i];
    free(message->author_id);
    free(message->author_username);
    free(message->content);
    free(message->id);
    free(message->timestamp);
    free(message);
  }
  free(search_response->messages);
  free(search_response);
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
  struct JsonToken* max_id = jsontok_get(options_object->as_object, "max_id");
  if (max_id && max_id->type == JSON_STRING) {
    options->max_id = strdup(max_id->as_string);
  }
  struct JsonToken* pinned = jsontok_get(options_object->as_object, "pinned");
  if (pinned && pinned->type == JSON_BOOLEAN) {
    options->pinned = pinned->as_boolean;
  }
  struct JsonToken* delay_ms =
      jsontok_get(options_object->as_object, "delay_ms");
  if (delay_ms && delay_ms->type == JSON_NUMBER) {
    double delay_value = delay_ms->as_number;
    if (delay_value >= 0 && delay_value <= (double)SIZE_MAX) {
      options->delay_ms = (size_t)delay_value;
    } else {
      goto cleanup;
    }
  }
  jsontok_free(options_object);
  return options;
cleanup:
  free(options);
  jsontok_free(options_object);
  return NULL;
}

void discrub_search_options_free(struct SearchOptions* options) {
  if (options == NULL) {
    return;
  }
  if (options->author_id) {
    free(options->author_id);
  }
  if (options->channel_id) {
    free(options->channel_id);
  }
  if (options->server_id) {
    free(options->server_id);
  }
  if (options->content) {
    free(options->content);
  }
  if (options->mentions) {
    free(options->mentions);
  }
  if (options->max_id) {
    free(options->max_id);
  }
  free(options);
}
