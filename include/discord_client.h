#ifndef DISCORD_CLIENT_H
#define DISCORD_CLIENT_H

#include "openssl_client.h"
#include <openssl/ssl.h>
#include <yyjson.h>

int discord_login(SSL *ssl, const char *username, const char *password, char **uid, char **token);

#endif
