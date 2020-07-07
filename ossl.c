#include "ossl.h"

void init_openssl(){
    SSL_load_error_strings();
}

SSL_CTX* create_tls_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_server_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("create tls ctx");
        exit(EXIT_FAILURE);
    }

    return ctx;
}
SSL_CTX* create_dtls_context(const char *CERT_PATH, const char *KEY_PATH) {

    const SSL_METHOD *method;
    SSL_CTX *ctx;
    
    method = DTLS_server_method();
    ctx = SSL_CTX_new(method);

    SSL_CTX_set_cipher_list(ctx, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");

    SSL_CTX_use_certificate_file(ctx, CERT_PATH, SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx, KEY_PATH, SSL_FILETYPE_PEM);

    return ctx;
}

void configure_context(SSL_CTX *ctx, const char* CERT_PATH, const char *KEY_PATH){
    SSL_CTX_set_ecdh_auto(ctx, 1);

    if(SSL_CTX_use_certificate_file(ctx, CERT_PATH, SSL_FILETYPE_PEM) <= 0){
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

     if(SSL_CTX_use_PrivateKey_file(ctx, KEY_PATH, SSL_FILETYPE_PEM) <= 0){
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}