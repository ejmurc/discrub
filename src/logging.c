#include "logging.h"

void printf_verbose(const char* format, ...) {
  time_t now;
  time(&now);
  char timestamp[20];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
  printf("[%s] ", timestamp);
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("\n");
}
