#include <openssl/ssl.h>
#include <stdio.h>

#include "discrub_client.h"
#include "logging.h"
#include "openssl_helpers.h"
#include "setup.h"

#define CREDENTIALS_FILEPATH "~/.cache/discrub/credentials"
#define OPTIONS_HELP_MESSAGE                                                   \
  "Usage: The program requires a valid 'options.json' file to run.\n"          \
  "Please ensure that the file exists and contains the necessary "             \
  "configuration.\n"                                                           \
  "The configuration should include the following required options:\n"         \
  "  - server_id<string>: The ID of the server to search within.\n"            \
  "  - channel_id<string>: The ID of the channel to search within.\n"          \
  "With the following non-required options:\n"                                 \
  "  - include_nsfw<boolean>: A boolean flag to include or exclude NSFW "      \
  "content.\n"                                                                 \
  "  - content<string>: A string to search for within the message content.\n"  \
  "  - mentions<string>: A string to filter messages that mention a specific " \
  "user or role.\n"                                                            \
  "  - pinned<boolean>: A boolean flag to filter only pinned messages.\n"      \
  "Example 'options.json':\n"                                                  \
  "{\n"                                                                        \
  "  \"server_id\": \"123456789012345678\",\n"                                 \
  "  \"channel_id\": \"987654321098765432\",\n"                                \
  "  \"include_nsfw\": false,\n"                                               \
  "  \"content\": \"search keyword\",\n"                                       \
  "  \"mentions\": \"username.asdf\",\n"                                       \
  "  \"pinned\": true\n"                                                       \
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
  printf_verbose("Checking cache for authentication credentials at '%s'...",
                 CREDENTIALS_FILEPATH);
  if (load_cache(CREDENTIALS_FILEPATH, &uid, &token)) {
    printf_verbose("Could not find authentication credentials in cache.");
    char *email = NULL, *password = NULL;
    if (prompt_credentials(&email, &password)) {
      printf_verbose("Failed to retrieve email or password from user prompt.");
      goto cleanup;
    }
    struct LoginResponse* response = discrub_login(bio, email, password);
    free(password);
    if (response == NULL) {
      printf_verbose(
          "Authentication failed: No response received from 'discrub_login'.");
      goto cleanup;
    }
    uid = response->uid;
    token = response->token;
    if (save_cache(CREDENTIALS_FILEPATH, uid, token)) {
      printf_verbose("Failed to save authentication credentials to '%s'.",
                     CREDENTIALS_FILEPATH);
      goto cleanup;
    }
    free(response);
  }
  printf_verbose("User <%s> successfully authenticated.", uid);

  char* options_string = read_file("options.json");
  if (options_string == NULL) {
    printf_verbose(OPTIONS_HELP_MESSAGE);
    goto cleanup;
  }
  struct SearchOptions* options = options_from_json(options_string);
  if (options == NULL) {
    printf(OPTIONS_HELP_MESSAGE);
    goto cleanup;
  }
  options->author_id = uid;
  struct SearchResponse* response = discrub_search(bio, options);

cleanup:
  BIO_free_all(bio);
  SSL_CTX_free(ctx);
  EVP_cleanup();
  ERR_free_strings();
}
