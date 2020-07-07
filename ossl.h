#ifndef OPENSSL_H
#define OPENSSL_H

#include <openssl/ssl.h>
#include <openssl/err.h>

#define CERT_PEM_PATH "./ssl-cert/cert.pem"
#define PKEY_PEM_PATH "./ssl-cert/key.pem"


void init_openssl();

SSL_CTX *create_tls_context();
SSL_CTX *create_dtls_context();

void configure_context(SSL_CTX *ctx, const char* CERT_PATH, const char *KEY_PATH);

#endif