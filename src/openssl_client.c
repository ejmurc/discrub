#include "openssl_client.h"

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
