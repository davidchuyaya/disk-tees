/**
 * Mostly based on https://github.com/openssl/openssl/blob/master/demos/sslecho/main.c.
 * Certificates are self-signed.
*/

#include <stdio.h>
#include <unistd.h>
#include <string>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "tls.hpp"

static const char IP[] = "127.0.0.1";
static const int PORT = 4433;

int create_socket(int port)
{
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("Unable to create socket");
        exit(EXIT_FAILURE);
    }

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Unable to bind");
        exit(EXIT_FAILURE);
    }

    if (listen(s, 1) < 0) {
        perror("Unable to listen");
        exit(EXIT_FAILURE);
    }

    return s;
}

SSL_CTX *create_context(bool isServer)
{
    const SSL_METHOD* method = isServer ? TLS_server_method() : TLS_client_method();
    SSL_CTX *ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

/**
 * From https://forum.nim-lang.org/t/7484 on how to verify self-signed certificates.
 * In code here: https://github.com/benob/gemini/blob/master/src/gemini.nim#L268
 * and here: https://github.com/benob/gemini/blob/master/src/gemini/patched_asyncnet.nim#L17.
*/
int verifyCallback(int preverify, X509_STORE_CTX *store)
{
    int error = X509_STORE_CTX_get_error(store);
    switch (error) {
        case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
        case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
            return 1;
    }
    return preverify;
}

void configure_context(SSL_CTX *ctx, const std::string cert, const std::string key)
{
    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, cert.c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key.c_str(), SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    // SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, verifyCallback);
}

void runServer()
{
    /* Ignore broken pipe signals */
    signal(SIGPIPE, SIG_IGN);

    SSL_CTX *ctx = create_context(true);
    configure_context(ctx, "server_cert.pem", "server_key.pem");
    int sock = create_socket(PORT);

    /* Handle connections */
    while (true) {
        struct sockaddr_in addr;
        unsigned int len = sizeof(addr);

        int client = accept(sock, (struct sockaddr*)&addr, &len);
        if (client < 0) {
            perror("Unable to accept");
            return;
        }

        printf("Client TCP connection accepted\n");

        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            return;
        }
        
        while (true) {
            int readLen;
            char readBuffer[256];
            if ((readLen = SSL_read(ssl, readBuffer, 256)) <= 0) {
                if (readLen == 0) {
                    printf("Client closed connection\n");
                } else {
                    printf("SSL_read returned %d\n", readLen);
                }
                ERR_print_errors_fp(stderr);
                break;
            }
            /* Insure null terminated input */
            readBuffer[readLen] = 0;
            printf("Received: %s", readBuffer);
            /* Echo it back */
            if (SSL_write(ssl, readBuffer, readLen) <= 0) {
                ERR_print_errors_fp(stderr);
            }
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client);
    }

    close(sock);
    SSL_CTX_free(ctx);
}

void runClient() {
    /* Ignore broken pipe signals */
    signal(SIGPIPE, SIG_IGN);

    SSL_CTX *ctx = create_context(false);
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    SSL_CTX_load_verify_locations(ctx, "server_cert.pem", NULL);
    // configure_context(ctx, "client_cert.pem", "client_key.pem");
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, IP, &(addr.sin_addr.s_addr));
    addr.sin_port = htons(4433);

    if (connect(sock, (struct sockaddr*) &addr, sizeof(addr)) != 0) {
        perror("Unable to TCP connect to server");
        return;
    }
    printf("TCP connection to server successful\n");

    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);
    SSL_set_tlsext_host_name(ssl, IP);
    SSL_set1_host(ssl, IP);

    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        return;
    }

    printf("SSL connection to server successful\n\n");

    /* Loop to send input from keyboard */
    while (true) {
        char* txbuf = NULL;
        size_t txcap = sizeof(txbuf);
        /* Get a line of input */
        int txlen = getline(&txbuf, &txcap, stdin);
        /* Exit loop on error */
        if (txlen < 0 || txbuf == NULL) {
            break;
        }
        /* Exit loop if just a carriage return */
        if (txbuf[0] == '\n') {
            break;
        }
        /* Send it to the server */
        int result;
        if ((result = SSL_write(ssl, txbuf, txlen)) <= 0) {
            printf("Server closed connection\n");
            ERR_print_errors_fp(stderr);
            break;
        }

        /* Wait for the echo */
        char rxbuf[256];
        size_t rxcap = sizeof(rxbuf);
        int rxlen = SSL_read(ssl, rxbuf, rxcap);
        if (rxlen <= 0) {
            printf("Server closed connection\n");
            ERR_print_errors_fp(stderr);
            break;
        } else {
            /* Show it */
            rxbuf[rxlen] = 0;
            printf("Received: %s", rxbuf);
        }
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sock);
    SSL_CTX_free(ctx);
}