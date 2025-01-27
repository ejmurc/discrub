#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

char* read_file(const char* filepath);

int load_cache(char* filepath, char** uid, char** token);

int save_cache(char* filepath, const char* uid, const char* token);

int prompt_credentials(char** email, char** password);
