#ifndef DISCORD_CLIENT_H
#define DISCORD_CLIENT_H

#include <openssl/ssl.h>

struct DiscordAuth {
  char *uid;
  char *token;
};

struct DiscordAuth *discord_login(SSL *ssl, const char *username, const char *password);

#endif
