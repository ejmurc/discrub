#include "openssl_client.h"

static char *trim_whitespace(char *str) {
  char *end;
  while (*str == ' ') {
    str++;
  }
  if (*str == 0) {
    return str;
  }
  end = str + strlen(str) - 1;
  while (end > str && *end == ' ') {
    end--;
  }
  end[1] = '\0';
  return str;
}

static void string_tolower(char *p) {
  for (; *p; p++) {
    *p = (char)tolower(*p);
  }
}

SSL_CTX *ssl_ctx_new(void) {
  SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
  if (!ctx) {
    ERR_print_errors_fp(stderr);
    return NULL;
  }
  SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
  if (SSL_CTX_set_default_verify_paths(ctx) != 1) {
    ERR_print_errors_fp(stderr);
    SSL_CTX_free(ctx);
    return NULL;
  }
  return ctx;
}

SSL *ssl_new(SSL_CTX *ctx, const char *hostname, const char *port) {
  if (!ctx || !hostname || !port) {
    fprintf(stderr, "ssl_connect: invalid argument\n");
    return NULL;
  }
  struct addrinfo hints = {0}, *res = NULL, *rp;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  int err = getaddrinfo(hostname, port, &hints, &res);
  if (err != 0) {
    fprintf(stderr, "getaddrinfo(%s:%s): %s\n", hostname, port, gai_strerror(err));
    return NULL;
  }
  int sock = -1;
  for (rp = res; rp != NULL; rp = rp->ai_next) {
#ifdef _WIN32
    sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sock == INVALID_SOCKET)
      continue;
#else
    sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sock < 0)
      continue;
#endif
    if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0)
      break;
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    sock = -1;
  }
  freeaddrinfo(res);
  if (sock < 0) {
    fprintf(stderr, "ssl_connect: unable to connect to %s:%s\n", hostname, port);
    return NULL;
  }

  SSL *ssl = SSL_new(ctx);
  if (!ssl) {
    fprintf(stderr, "SSL_new failed:\n");
    ERR_print_errors_fp(stderr);
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    return NULL;
  }
  if (!SSL_set_tlsext_host_name(ssl, hostname)) {
    fprintf(stderr, "SSL_set_tlsext_host_name failed:\n");
    ERR_print_errors_fp(stderr);
    SSL_free(ssl);
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    return NULL;
  }
  SSL_set_fd(ssl, sock);
  if (SSL_connect(ssl) <= 0) {
    fprintf(stderr, "SSL_connect failed:\n");
    ERR_print_errors_fp(stderr);
    SSL_free(ssl);
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    return NULL;
  }
  long verify = SSL_get_verify_result(ssl);
  if (verify != X509_V_OK) {
    fprintf(stderr, "SSL certificate verification failed: %s\n",
            X509_verify_cert_error_string(verify));
    SSL_shutdown(ssl);
    SSL_free(ssl);
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    return NULL;
  }
  return ssl;
}

char *ssl_request(SSL *ssl, const char *method, const char *path, const char *hostname,
                  const char *body) {
  if (!ssl || !method || !path || !hostname) {
    return NULL;
  }
  char *request = NULL;
  int len = 0;
  if (body) {
    const char *request_fmt = "%s %s HTTP/1.1\r\n"
                              "Host: %s\r\n"
                              "Content-Length: %zu\r\n"
                              "Content-Type: application/json\r\n"
                              "Connection: close\r\n"
                              "\r\n"
                              "%s";
    len = snprintf(NULL, 0, request_fmt, method, path, hostname, strlen(body), body);
    request = malloc((size_t)len + 1);
    if (!request) {
      return NULL;
    }
    if (snprintf(request, len + 1, request_fmt, method, path, hostname, strlen(body), body) !=
        len) {
      return NULL;
    }
  } else {
    const char *request_fmt = "%s %s HTTP/1.1\r\n"
                              "Host: %s\r\n"
                              "Content-Type: application/json\r\n"
                              "Connection: close\r\n"
                              "\r\n";
    len = snprintf(NULL, 0, request_fmt, method, path, hostname);
    request = malloc((size_t)len + 1);
    if (!request) {
      return NULL;
    }
    if (snprintf(request, len + 1, request_fmt, method, path, hostname) != len) {
      return NULL;
    }
  }
  int total_written = 0;
  while (total_written < len) {
    int written = SSL_write(ssl, request + total_written, len - total_written);
    if (written > 0) {
      total_written += written;
    } else {
      int err = SSL_get_error(ssl, written);
      if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ) {
        continue;
      } else {
        fprintf(stderr, "SSL_write failed with error %d:\n", err);
        ERR_print_errors_fp(stderr);
        return NULL;
      }
    }
  }
  free(request);
  int response_size = 0;
  int response_capacity = 1024;
  char *response = malloc((size_t)response_capacity);
  while (1) {
    int bytes = SSL_read(ssl, response + response_size, response_capacity - response_size - 1);
    if (bytes > 0) {
      response_size += bytes;
      if (response_size + 1024 > response_capacity) {
        response_capacity *= 2;
        char *new_response = realloc(response, (size_t)response_capacity);
        if (!new_response) {
          fprintf(stderr, "ssl_request: realloc failed\n");
          free(response);
          return NULL;
        }
        response = new_response;
      }
    } else {
      int err = SSL_get_error(ssl, bytes);
      if (err == SSL_ERROR_ZERO_RETURN) {
        break;
      } else if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
        continue;
      } else {
        fprintf(stderr, "SSL_read failed with error %d:\n", err);
        ERR_print_errors_fp(stderr);
        free(response);
        return NULL;
      }
    }
  }
  response[response_size] = '\0';
  return response;
}

struct HTTPResponse *parse_raw_response(char *raw) {
  if (!raw) {
    return NULL;
  }
  struct HTTPResponse *response = malloc(sizeof(struct HTTPResponse));
  if (!response) {
    return NULL;
  }
  response->raw = raw;
  response->headers = NULL;
  char *headers_end = strstr(raw, "\r\n\r\n");
  if (!headers_end) {
    free(response);
    return NULL;
  }
  *headers_end = '\0';
  char *status_line = raw;
  char *next_line = strstr(status_line, "\r\n");
  if (!next_line) {
    free(response);
    return NULL;
  }
  *next_line = '\0';
  next_line += 2;
  char *http_version_end = strchr(status_line, ' ');
  if (!http_version_end) {
    free(response);
    return NULL;
  }
  char *response_code_end = strchr(http_version_end + 1, ' ');
  if (!response_code_end) {
    free(response);
    return NULL;
  }
  *response_code_end = '\0';
  char *response_code_str = http_version_end + 1;
  char *endptr;
  errno = 0;
  long code_value = strtol(response_code_str, &endptr, 10);
  if (errno != 0 || endptr == response_code_str || *endptr != '\0' || code_value < 0 ||
      code_value > UINT16_MAX) {
    free(response);
    return NULL;
  }
  response->code = (uint16_t)code_value;
  char *header_line = next_line;
  unsigned char chunked = 0;
  while (header_line && *header_line != '\0') {
    next_line = strstr(header_line, "\r\n");
    if (next_line) {
      *next_line = '\0';
      next_line += 2;
    }
    char *key_end = strchr(header_line, ':');
    if (!key_end) {
      break;
    }
    *key_end = '\0';
    char *header_key = trim_whitespace(header_line);
    char *header_value = trim_whitespace(key_end + 1);
    if (!header_key || !header_value) {
      break;
    }
    string_tolower(header_key);
    struct Header *header = malloc(sizeof(struct Header));
    if (!header) {
      free_http_response(response);
      return NULL;
    }
    header->key = header_key;
    header->value = header_value;
    header->next = NULL;
    if (response->headers) {
      struct Header *current = response->headers;
      while (current->next) {
        current = current->next;
      }
      current->next = header;
    } else {
      response->headers = header;
    }
    if (chunked == 0 && strcmp(header_key, "transfer-encoding") == 0 &&
        strcmp(header_value, "chunked") == 0) {
      chunked = 1;
    }
    header_line = next_line;
  }
  char *body = headers_end + 4;
  if (strlen(body) > 0 && chunked == 1) {
    char *read_ptr = body;
    char *write_ptr = body;
    for (;;) {
      char *chunk_size_end = strstr(read_ptr, "\r\n");
      if (!chunk_size_end)
        break;
      *chunk_size_end = '\0';
      size_t chunk_size = (size_t)strtol(read_ptr, NULL, 16);
      if (chunk_size == 0)
        break;
      read_ptr = chunk_size_end + 2;
      memmove(write_ptr, read_ptr, chunk_size);
      write_ptr += chunk_size;
      read_ptr += chunk_size + 2;
    }
    *write_ptr = '\0';
  }
  response->body = body;
  return response;
}

void free_http_response(struct HTTPResponse *response) {
  struct Header *header = response->headers;
  while (header) {
    struct Header *next = header->next;
    free(header);
    header = next;
  }
  free(response->raw);
  free(response);
}

void ssl_free(SSL *ssl) {
#ifdef _WIN32
  SOCKET sock = SSL_get_fd(ssl);
#else
  int sock = SSL_get_fd(ssl);
#endif
  SSL_shutdown(ssl);
  SSL_free(ssl);
#ifdef _WIN32
  closesocket(sock);
#else
  close(sock);
#endif
}
