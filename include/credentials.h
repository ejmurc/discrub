#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

char *credentials_filepath(const char *appname);
int save_credentials(const char *filepath, const char *credentials, const char *password);
char *load_credentials(const char *filepath, const char *password);

#endif
