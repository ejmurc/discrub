#ifndef LOG_H
#define LOG_H

#define CLR_GRN "\033[32m"
#define CLR_YLW "\033[33m"
#define CLR_RED "\033[31m"
#define CLR_RST "\033[0m"

#define LOG_OK(...) (fprintf(stdout, CLR_GRN "[+]" CLR_RST " " __VA_ARGS__), fputc('\n', stdout))
#define LOG_INFO(...) (fprintf(stdout, "[-] " __VA_ARGS__), fputc('\n', stdout))
#define LOG_WARN(...) (fprintf(stdout, CLR_YLW "[?]" CLR_RST " " __VA_ARGS__), fputc('\n', stdout))
#define LOG_ERR(...) (fprintf(stderr, CLR_RED "[!]" CLR_RST " " __VA_ARGS__), fputc('\n', stderr))

#endif
