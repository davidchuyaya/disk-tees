/**
 * Open TLS connections between machines, start a server, listen to clients.
 * 
 * Mostly based on https://github.com/openssl/openssl/blob/master/demos/sslecho/main.c.
 * Assumes that certificates are self-signed.
*/

#include <stdio.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <thread>
#include "tls.hpp"

static const int CONNECTION_RETRY_TIMEOUT_SEC = 5;

TLS::TLS(const int myID, const networkConfig netConf,
            const std::function<void(clientInPayload)> onClientMsg, 
            const std::function<void(diskTeePayload)> onDiskTeeMsg) :
            myID(myID), netConf(netConf), onClientMsg(onClientMsg), onDiskTeeMsg(onDiskTeeMsg) {
    // Connect to peers asynchronously
    for (int i = 0; i < netConf.peers.size(); i++) {
        if (i == myID)
            continue;
        std::thread(&TLS::runClient, this, netConf.peers[i]).detach();
    }
    startServer();
}

void TLS::startServer()
{
    // Ignore broken pipe signals
    signal(SIGPIPE, SIG_IGN);

    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    loadAcceptableCertificates(ctx);
    loadOwnCertificates(ctx);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = netConf.peers[myID].sin_port;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Unable to bind" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (listen(sock, 1) < 0) {
        std::cerr << "Unable to listen" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Started server" << std::endl;

    while (true) {
        sockaddr_in addr;
        unsigned int len = sizeof(addr);

        int client = accept(sock, (struct sockaddr*)&addr, &len);
        if (client < 0) {
            std::cerr << "Unable to accept" << std::endl;
            continue;
        }

        std::cout << "Client TCP connection accepted" << std::endl;

        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            continue;
        }

        std::cout << "Client SSL connection accepted" << std::endl;

        // Listen to the client on an async thread
        std::thread connectionThread = std::thread(&TLS::threadListen, this, addr, ssl);
        connectionThread.detach();
    }

    // close server
    close(sock);
    SSL_CTX_free(ctx);
}

void TLS::runClient(const sockaddr_in serverAddr) {
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    loadAcceptableCertificates(ctx);
    loadOwnCertificates(ctx);
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    // Get IP address (string) out from sockaddr_in
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(serverAddr.sin_addr), ip, INET_ADDRSTRLEN);

    while (connect(sock, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) != 0) {
        std::cout << "Unable to TCP connect to server " << ip << ", retrying in " << CONNECTION_RETRY_TIMEOUT_SEC << "s..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(CONNECTION_RETRY_TIMEOUT_SEC));
    }
    std::cout << "TCP connection to server successful" << std::endl;

    
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);
    SSL_set_tlsext_host_name(ssl, ip);
    SSL_set1_host(ssl, ip);

    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        return;
    }

    std::cout << "SSL connection to server successful" << std::endl;

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
    close(SSL_get_fd(ssl));
    SSL_CTX_free(ctx);
}

void TLS::threadListen(const sockaddr_in senderAddr, SSL* sender) {
    while (true) {
        int readLen;
        char readBuffer[256];
        if ((readLen = SSL_read(sender, readBuffer, 256)) <= 0) {
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
        if (SSL_write(sender, readBuffer, readLen) <= 0) {
            ERR_print_errors_fp(stderr);
        }
    }
    
    // SSL shutdown, close socket
    SSL_shutdown(sender);
    SSL_free(sender);
    close(SSL_get_fd(sender));
}

void TLS::loadOwnCertificates(SSL_CTX *ctx) {
    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, (netConf.names[myID] + "_cert.pem").c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, (netConf.names[myID] + "_key.pem").c_str(), SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

void TLS::loadAcceptableCertificates(SSL_CTX *ctx) {
    for (const std::string& name : netConf.names) {
        SSL_CTX_load_verify_locations(ctx, (name + "_cert.pem").c_str(), NULL);
    }
}