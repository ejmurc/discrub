#include <openssl/ssl.h>
#include <stdio.h>

#include "discrub_client.h"
#include "logging.h"
#include "openssl_helpers.h"
#include "setup.h"

#define CREDENTIALS_FILEPATH "~/.cache/discrub/credentials"

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

cleanup:
  BIO_free_all(bio);
  SSL_CTX_free(ctx);
  EVP_cleanup();
  ERR_free_strings();
}
