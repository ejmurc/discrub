#include "openssl_helpers.h"

#define READ_BUFFER_SIZE 1024

static char* trim_whitespace(char* str) {
  char* end;
  while (*str == ' ')
    str++;
  if (*str == 0)
    return str;
  end = str + strlen(str) - 1;
  while (end > str && *end == ' ')
    end--;
  end[1] = '\0';
  return str;
}

SSL_CTX* create_ssl_ctx() {
  SSL_load_error_strings();
  SSL_library_init();
  OpenSSL_add_all_algorithms();
  SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
  if (ctx == NULL) {
    ERR_print_errors_fp(stderr);
    return NULL;
  }
  if (SSL_CTX_set_default_verify_paths(ctx) == 0) {
    fprintf(stderr, "SSL_CTX_set_default_verify_paths failed\n");
    return NULL;
  }
  return ctx;
}

BIO* create_bio(SSL_CTX* ctx, const char* hostname) {
  if (ctx == NULL || hostname == NULL) {
    return NULL;
  }
  BIO* bio = BIO_new_ssl_connect(ctx);
  if (bio == NULL) {
    fprintf(stderr, "BIO_new_ssl_connect failed:\n");
    ERR_print_errors_fp(stderr);
    return NULL;
  }
  SSL* ssl = NULL;
  BIO_get_ssl(bio, &ssl);
  if (ssl == NULL) {
    fprintf(stderr, "Cannot locate SSL pointer");
    BIO_free_all(bio);
    return NULL;
  }
  SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
  BIO_set_conn_hostname(bio, hostname);
  if (BIO_do_connect(bio) != 1) {
    fprintf(stderr, "BIO_do_connect failed:\n");
    ERR_print_errors_fp(stderr);
    BIO_free_all(bio);
    return NULL;
  }
  if (SSL_get_verify_result(ssl) != X509_V_OK) {
    fprintf(stderr, "SSL certificate verification failed: %s\n",
            X509_verify_cert_error_string(SSL_get_verify_result(ssl)));
    BIO_free_all(bio);
    return NULL;
  }
  return bio;
}

struct HTTPResponse* perform_http_request(BIO* bio, const char* request) {
  if (bio == NULL || request == NULL) {
    fprintf(stderr, "perform_http_request: Null argument(s)");
    return NULL;
  }
  /*BIO_reset(bio);*/
  const size_t request_length = strlen(request);
  size_t total_written = 0;
  while (total_written < request_length) {
    int written =
        BIO_write(bio, request + total_written, request_length - total_written);
    if (written > 0) {
      total_written += written;
    } else if (written == 0) {
      continue;
    } else {
      fprintf(stderr, "BIO_write error: %s\n",
              ERR_reason_error_string(ERR_get_error()));
      return NULL;
    }
  }
  char *raw = NULL, buffer[READ_BUFFER_SIZE];
  size_t total_size = 0;
  for (;;) {
    int size = BIO_read(bio, buffer, READ_BUFFER_SIZE - 1);
    if (size > 0) {
      buffer[size] = '\0';
      char* new_raw = realloc(raw, total_size + size + 1);
      if (new_raw == NULL) {
        perror("Failed to allocate memory");
        free(raw);
        return NULL;
      }
      raw = new_raw;
      memcpy(raw + total_size, buffer, size);
      total_size += size;
      raw[total_size] = '\0';
    } else if (size == 0) {
      break;
    } else if (BIO_should_retry(bio) == 0 ||
               (BIO_should_read(bio) == 0 && BIO_should_write(bio) == 0)) {
      fprintf(stderr, "BIO_read failed: %s\n",
              ERR_reason_error_string(ERR_get_error()));
      free(raw);
      return NULL;
    }
  }
  struct HTTPResponse* response = malloc(sizeof(struct HTTPResponse));
  if (response == NULL) {
    perror("Failed to allocate memory for HTTPResponse");
    free(raw);
    return NULL;
  }
  response->raw = raw;
  char* headers_end = strstr(raw, "\r\n\r\n");
  if (headers_end == NULL) {
    fprintf(stderr, "No headers found in response: %s\n", raw);
    free_http_response(response);
    return NULL;
  }
  *headers_end = '\0';
  char* status_line = raw;
  char* next_line = strstr(status_line, "\r\n");
  if (next_line == NULL) {
    fprintf(stderr, "Malformed response: no status line found.\n");
    free_http_response(response);
    return NULL;
  }
  *next_line = '\0';
  next_line += 2;
  char* http_version_end = strchr(status_line, ' ');
  if (http_version_end == NULL) {
    fprintf(stderr,
            "Malformed response: Missing HTTP version in status line: %s\n",
            status_line);
    free_http_response(response);
    return NULL;
  }
  char* response_code_end = strchr(http_version_end + 1, ' ');
  if (response_code_end == NULL) {
    fprintf(stderr,
            "Malformed response: Missing response code in status line: %s\n",
            status_line);
    free_http_response(response);
    return NULL;
  }
  *response_code_end = '\0';
  char* response_code_str = http_version_end + 1;
  char* endptr;
  errno = 0;
  long value = strtol(response_code_str, &endptr, 10);
  if (errno != 0 || endptr == response_code_str || *endptr != '\0' ||
      value < 0 || value > UINT16_MAX) {
    fprintf(stderr, "Malformed response: Invalid response code: %s\n",
            response_code_str);
    free_http_response(response);
    return NULL;
  }
  uint16_t response_code = value;
  char* header_line = next_line;
  while (header_line && *header_line != '\0') {
    next_line = strstr(header_line, "\r\n");
    if (next_line) {
      *next_line = '\0';
      next_line += 2;
    }
    char* key_end = strchr(header_line, ':');
    if (key_end == NULL) {
      fprintf(stderr, "Malformed response: key not found in header line: %s.\n",
              header_line);
      free_http_response(response);
      break;
    }
    *key_end = '\0';
    char* key = trim_whitespace(header_line);
    char* value = trim_whitespace(key_end + 1);
    if (key == NULL || value == NULL) {
      fprintf(
          stderr,
          "Malformed response: key or value not found in header line: %s.\n",
          header_line);
      free_http_response(response);
      break;
    }
    struct Header* header = malloc(sizeof(struct Header));
    if (header == NULL) {
      perror("Memory allocation failed");
      free_http_response(response);
      return NULL;
    }
    if (response->headers) {
      struct Header* current = response->headers;
      while (current->next) {
        current = current->next;
      }
      current->next = header;
    } else {
      response->headers = header;
    }
    header->key = key;
    header->value = value;
    header_line = next_line;
  }
  char* body = headers_end + 4;
  response->body = body;
  response->raw = raw;
  response->code = response_code;
  return response;
}

void free_http_response(struct HTTPResponse* response) {
  struct Header* header = response->headers;
  while (header) {
    struct Header* next = header->next;
    free(header);
    header = next;
  }
  free(response->raw);
  free(response);
}
