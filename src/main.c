#include <stdio.h>
#include <yyjson.h>

#include "openssl_client.h"

int main(void) {
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
}
