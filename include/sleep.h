#ifndef SLEEP_H
#define SLEEP_H

#ifdef _WIN32
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>
#else
#include <unistd.h>
#endif

void sleep_ms(int ms);

#endif
