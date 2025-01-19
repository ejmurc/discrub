#ifndef DISCRUB_CLIENT_H
#define DISCRUB_CLIENT_H

#include <openssl/bio.h>
#include "jsontok.h"
#include "openssl_helpers.h"

struct LoginResponse {
  char* token;
  char* uid;
};

struct LoginResponse* discrub_login(BIO* bio, const char* username,
                                    const char* password);

#endif
