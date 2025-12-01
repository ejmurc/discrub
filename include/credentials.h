#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN_32
#include <direct.h>
#include <io.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

char *credentials_filepath(const char *appname);
int save_credentials(const char *filepath, const char *credentials);
char *load_credentials(const char *filepath);

#endif
