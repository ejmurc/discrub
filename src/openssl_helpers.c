#include "openssl_helpers.h"

SSL_CTX* CreateSSL_CTX() {
  SSL_load_error_strings();
  SSL_library_init();
  OpenSSL_add_all_algorithms();
  SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
  if (!ctx) {
    ERR_print_errors_fp(stderr);
    return NULL;
  }
  if (!SSL_CTX_set_default_verify_paths(ctx)) {
    fprintf(stderr, "SSL_CTX_set_default_verify_paths failed");
    return NULL;
  }
  return ctx;
}

BIO* CreateBIO(SSL_CTX* ctx, const char* hostname) {
  if (ctx == NULL || hostname == NULL) {
    return NULL;
  }
  BIO* bio = BIO_new_ssl_connect(ctx);
  if (bio == NULL) {
    ERR_print_errors_fp(stderr);
    SSL_CTX_free(ctx);
    return NULL;
  }
  SSL* ssl = NULL;
  BIO_get_ssl(bio, &ssl);
  if (ssl == NULL) {
    fprintf(stderr, "Cannot locate SSL pointer");
    BIO_free_all(bio);
    SSL_CTX_free(ctx);
    return NULL;
  }
  SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
  BIO_set_conn_hostname(bio, hostname);
  if (BIO_do_connect(bio) != 1) {
    perror("Error connecting to host");
    BIO_free_all(bio);
    SSL_CTX_free(ctx);
    return NULL;
  }
  if (SSL_get_verify_result(ssl) != X509_V_OK) {
    fprintf(stderr, "SSL certificate verification failed: %s\n",
            X509_verify_cert_error_string(SSL_get_verify_result(ssl)));
    BIO_free_all(bio);
    SSL_CTX_free(ctx);
    return NULL;
  }
  return bio;
}
