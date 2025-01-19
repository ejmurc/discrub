#include "openssl_helpers.h"

#define READ_BUFFER_SIZE 1024

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
  char *response_string = NULL, buffer[READ_BUFFER_SIZE];
  size_t total_size = 0;
  for (;;) {
    int size = BIO_read(bio, buffer, READ_BUFFER_SIZE - 1);
    if (size > 0) {
      buffer[size] = '\0';
      char* new_response = realloc(response_string, total_size + size + 1);
      if (new_response == NULL) {
        perror("Failed to allocate memory");
        free(response_string);
        return NULL;
      }
      response_string = new_response;
      memcpy(response_string + total_size, buffer, size);
      total_size += size;
      response_string[total_size] = '\0';
    } else if (size == 0) {
      break;
    } else if (BIO_should_retry(bio) == 0 ||
               (BIO_should_read(bio) == 0 && BIO_should_write(bio) == 0)) {
      fprintf(stderr, "BIO_read failed: %s\n",
              ERR_reason_error_string(ERR_get_error()));
      free(response_string);
      return NULL;
    }
  }
  printf("%s\n", response_string);
  struct HTTPResponse* response = malloc(sizeof(struct HTTPResponse));
  if (response == NULL) {
    perror("Failed to allocate memory for HTTPResponse");
    free(response_string);
    return NULL;
  }
  return response;
}

void free_http_response(struct HTTPResponse* response) {
  struct Header* header = response->headers;
  while (header) {
    struct Header* next = header->next;
    free(header->key);
    free(header->value);
    free(header);
    header = next;
  }
  free(response->content);
  free(response);
}
