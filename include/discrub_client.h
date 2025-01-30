#ifndef DISCRUB_CLIENT_H
#define DISCRUB_CLIENT_H

#include <openssl/bio.h>
#include <stdbool.h>
#include "jsontok.h"
#include "openssl_helpers.h"

struct DiscordMessage {
  char* author_id;
  char* author_username;
  char* content;
  char* id;
  char* timestamp;
};

struct SearchResponse {
  struct DiscordMessage** messages;
  size_t length;
};

struct SearchOptions {
  char* author_id;
  char* channel_id;
  char* server_id;
  bool include_nsfw;
  size_t offset;
  char* content;
  char* mentions;
  bool pinned;
};

struct LoginResponse {
  char* token;
  char* uid;
};

struct LoginResponse* discrub_login(BIO* bio, const char* email,
                                    const char* password);

struct SearchResponse* discrub_search(BIO* bio, const char* token,
                                      struct SearchOptions* options);

struct SearchOptions* options_from_json(const char* json_string);

void discrub_search_response_free(struct SearchResponse* search_response);

#endif
