#include <openssl/ssl.h>
#include <stdio.h>

#include "discrub_client.h"
#include "openssl_helpers.h"
#include "setup.h"

#define CREDENTIALS_FILEPATH "~/.cache/discrub/credentials"

int main() {
  SSL_CTX* ctx = create_ssl_ctx();
  if (ctx == NULL) {
    goto cleanup_none;
  }
  BIO* bio = create_bio(ctx, "discord.com:443");
  if (bio == NULL) {
    goto cleanup_ctx;
  }
  char *uid = NULL, *token = NULL;
  if (load_cache(CREDENTIALS_FILEPATH, &uid, &token)) {
    char *email = NULL, *password = NULL;
    if (prompt_credentials(&email, &password)) {
      goto cleanup_bio;
    }
    struct LoginResponse* response = discrub_login(bio, email, password);
    free(password);
    if (response == NULL) {
      fprintf(stderr, "Failed to get response from discrub_login.\n");
      goto cleanup_bio;
    }
    uid = response->uid;
    token = response->token;
    if (save_cache(CREDENTIALS_FILEPATH, uid, token)) {
      goto cleanup_bio;
    }
    free(response);
  }
  printf("%s %s\n", uid, token);

cleanup_bio:
  BIO_free_all(bio);
cleanup_ctx:
  SSL_CTX_free(ctx);
cleanup_none:
  EVP_cleanup();
  ERR_free_strings();
}
