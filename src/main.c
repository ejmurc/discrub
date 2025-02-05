#include <openssl/ssl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "discrub_client.h"
#include "logging.h"
#include "openssl_helpers.h"
#include "setup.h"
#include "sleep.h"

#define CREDENTIALS_FILEPATH "~/.cache/discrub/credentials"
#define OPTIONS_HELP_MESSAGE                                                   \
  "Usage: The program requires a valid 'options.json' file to run.\n"          \
  "Please ensure that the file exists and contains the necessary "             \
  "configuration.\n"                                                           \
  "The configuration should include the following required options:\n"         \
  "  - server_id<string>: The ID of the server to search within.\n"            \
  "With the following non-required options:\n"                                 \
  "  - channel_id<string>: The ID of the channel to search within.\n"          \
  "  - include_nsfw<boolean>: A boolean flag to include or exclude NSFW "      \
  "content.\n"                                                                 \
  "  - content<string>: A string to search for within the message content.\n"  \
  "  - mentions<string>: A string to filter messages that mention a specific " \
  "user or role.\n"                                                            \
  "  - pinned<boolean>: A boolean flag to filter only pinned messages.\n"      \
  "  - delay_ms<size_t>: An unsigned integer representing the initial delay "  \
  "between delete requests. Keep in mind this delay increases exponentially "  \
  "upon deletion ratelimiting.\n"                                              \
  "Example 'options.json':\n"                                                  \
  "{\n"                                                                        \
  "  \"server_id\": \"123456789012345678\",\n"                                 \
  "  \"channel_id\": \"987654321098765432\",\n"                                \
  "  \"include_nsfw\": false,\n"                                               \
  "  \"content\": \"search keyword\",\n"                                       \
  "  \"mentions\": \"username.asdf\",\n"                                       \
  "  \"pinned\": true\n"                                                       \
  "  \"delay_ms\": 500\n"                                                      \
  "  \"max_id\": \"123456789012345678\",\n"                                    \
  "}\n"

int main() {
  BIO* bio = NULL;
  SSL_CTX* ctx = create_ssl_ctx();
  if (ctx == NULL) {
    goto cleanup;
  }
  bio = create_bio(ctx, "discord.com:443");
  if (bio == NULL) {
    goto cleanup;
  }
  char *uid = NULL, *token = NULL;
  printf_verbose("Checking cache for authentication credentials at '%s'...\n",
                 CREDENTIALS_FILEPATH);
  if (load_cache(CREDENTIALS_FILEPATH, &uid, &token)) {
    printf_verbose("Could not find authentication credentials in cache.\n");
    char *email = NULL, *password = NULL;
    if (prompt_credentials(&email, &password)) {
      printf_verbose(
          "Failed to retrieve email or password from user prompt.\n");
      goto cleanup;
    }
    struct LoginResponse* response = discrub_login(bio, email, password);
    free(password);
    if (response == NULL) {
      printf_verbose(
          "Authentication failed: No response received from "
          "'discrub_login'.\n");
      goto cleanup;
    }
    uid = response->uid;
    token = response->token;
    if (save_cache(CREDENTIALS_FILEPATH, uid, token)) {
      printf_verbose("Failed to save authentication credentials to '%s'.\n",
                     CREDENTIALS_FILEPATH);
      goto cleanup;
    }
    free(response);
  }
  printf_verbose("User <%s> successfully authenticated.\n", uid);

  char* options_string = read_file("options.json");
  if (options_string == NULL) {
    printf_verbose(OPTIONS_HELP_MESSAGE);
    goto cleanup;
  }
  struct SearchOptions* options = options_from_json(options_string);
  free(options_string);
  if (options == NULL) {
    printf(OPTIONS_HELP_MESSAGE);
    goto cleanup;
  }
  options->author_id = uid;
  srand(time(NULL));
  struct SearchResponse* response = discrub_search(bio, token, options);
  if (response == NULL) {
    fprintf(stderr, "Failed to get response from discrub_search\n");
    goto cleanup;
  }
  const size_t total_messages = response->total_messages;
  size_t remaining_messages = total_messages;
  size_t delay_ms = options->delay_ms;
  if (delay_ms == 0) {
    delay_ms = 1500;
  }
  while (remaining_messages > 0) {
    printf_verbose(
        "Remaining: %zu/%zu (%.2f%%), Fetched: %zu\n", remaining_messages,
        total_messages,
        (total_messages - remaining_messages) * 100.0 / total_messages,
        response->length);
    size_t i = 0;
    for (; i < response->length; i++) {
      struct DiscordMessage* message = response->messages[i];
      if (message == NULL)
        continue;
      if (message->type == 19) {
        printf_verbose("Unarchiving thread for message <%s>\n", message->id,
                       message->channel_id);
        if (discrub_unarchive_thread(bio, token, message->channel_id)) {
          printf_verbose(
              "Failed to unarchive thread. Continuing with iterations in "
              "batch.\n");
          continue;
        }
      }
      printf_verbose("Deleting message <%s> {%s} [%s] %.40s\n", message->id,
                     message->timestamp, message->author_username,
                     message->content);
      int retry = discrub_delete_message(bio, token, message->channel_id,
                                         message->id),
          attempts = 1;
      if (retry > 0) {
        delay_ms *= 2;
      }
      while (retry > 0 && attempts <= 10) {
        const int offset = rand() % 1000;
        printf_verbose("Rate limited. Retrying in %dms.\n",
                       retry * attempts + offset);
        sleep_ms(retry * attempts + offset);
        retry = discrub_delete_message(bio, token, message->channel_id,
                                       message->id);
        attempts++;
      }
      if (retry == -1) {
        printf_verbose(
            "Failed to delete message. Continuing with iterations in batch.\n");
        continue;
      }
      remaining_messages--;
      sleep_ms(delay_ms + rand() % 200);
    }
    if (response->length > 0) {
      free(options->max_id);
      options->max_id = strdup(response->messages[response->length - 1]->id);
    } else {
      printf_verbose("No more messages found.\n");
      break;
    }
    discrub_search_response_free(response);
    sleep_ms(1500);
    response = discrub_search(bio, token, options);
    if (response == NULL) {
      printf_verbose(
          "Failed to fetch the next batch of messages. Exiting loop.\n");
      break;
    }
  }
  discrub_search_options_free(options);
  discrub_search_response_free(response);
  free(token);

cleanup:
  BIO_free_all(bio);
  SSL_CTX_free(ctx);
  EVP_cleanup();
  ERR_free_strings();
}
