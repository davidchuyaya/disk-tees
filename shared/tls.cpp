/**
 * Open TLS connections between machines, start a server, listen to clients.
 * 
 * Mostly based on https://github.com/openssl/openssl/blob/master/demos/sslecho/main.c.
 * Assumes that certificates are self-signed.
*/

#ifndef __TLS_CPP__
#define __TLS_CPP__

#include <stdio.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <thread>
#include <mutex>
#include <memory>
#include <stdexcept>
#include <filesystem>
#include "zpp_bits.h"
#include "tls.hpp"

static const int CONNECTION_RETRY_TIMEOUT_SEC = 5;
static const int READ_BUFFER_SIZE = 8192;

template <class RecvMsg, class SendMsg>
TLS<RecvMsg, SendMsg>::TLS(const int myID, const std::vector<peer> netConf, const std::string path, const std::function<void(RecvMsg, SSL*)> onRecv) :
            myID(myID), netConf(netConf), path(path), onRecv(onRecv) {
    // Connect to peers asynchronously, with lower-numbered peers connecting to higher-numbered peers
    for (int i = myID + 1; i < netConf.size(); i++) {
        const peer& addr = netConf[i];
        std::thread(&TLS::connectToServer, this, addr).detach();
    }
    // Start server asynchronously
    std::thread(&TLS::startServer, this).detach();
}

template <class RecvMsg, class SendMsg>
void TLS<RecvMsg, SendMsg>::startServer()
{
    // Ignore broken pipe signals
    signal(SIGPIPE, SIG_IGN);

    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    loadAcceptableCertificates(ctx);
    loadOwnCertificates(ctx);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(netConf[myID].port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    disableNaglesAlgorithm(sock);

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

        // Add connection to map
        // TODO: If connection already exists, something's going on (TLS impersonation)
        const std::string key = readableAddr(addr);
        {
            std::unique_lock lock(connectionsMutex);
            connections.emplace(key, ssl);
        }

        // Listen to the client on an async thread
        std::thread connectionThread = std::thread(&TLS::threadListen, this, key, addr, ssl);
        connectionThread.detach();
    }

    // close server
    close(sock);
    SSL_CTX_free(ctx);
}

template <class RecvMsg, class SendMsg>
void TLS<RecvMsg, SendMsg>::connectToServer(const peer& addr) {
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    loadAcceptableCertificates(ctx);
    loadOwnCertificates(ctx);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    disableNaglesAlgorithm(sock);
    sockaddr_in sockAddr;
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(addr.port);   
    inet_pton(AF_INET, addr.ip.c_str(), &(sockAddr.sin_addr.s_addr));
    
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);
    SSL_set_tlsext_host_name(ssl, addr.ip.c_str());
    SSL_set1_host(ssl, addr.ip.c_str());

    std::string readableAddrStr = addr.ip + ":" + std::to_string(addr.port);

    while (true) {
        // code for connection is in a loop so we automatically attempt to reconnect
        while (connect(sock, (struct sockaddr*) &sockAddr, sizeof(sockAddr)) != 0) {
            std::cout << "Unable to TCP connect to server " << addr.ip << ", retrying in " << CONNECTION_RETRY_TIMEOUT_SEC << "s..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(CONNECTION_RETRY_TIMEOUT_SEC));
        }
        std::cout << "TCP connection to server successful" << std::endl;

        if (SSL_connect(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            break;
        }
        std::cout << "SSL connection to server successful" << std::endl;

        // Add connection to map
        {
            std::unique_lock lock(connectionsMutex);
            connections.emplace(readableAddrStr, ssl);
        }

        /* Loop to send input from keyboard */
        char readBuffer[READ_BUFFER_SIZE];
        try {
            while (true) {
                RecvMsg recvPayload = recv(readBuffer, ssl);
                onRecv(recvPayload, ssl);
            }
        }
        catch (const std::exception& e) { // on exception, close connection
            std::cerr << e.what() << std::endl;
        }
    }

    // Remove the connection from the map
    {
        std::unique_lock lock(connectionsMutex);
        connections.erase(readableAddrStr);
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(SSL_get_fd(ssl));
    SSL_CTX_free(ctx);
}

template <class RecvMsg, class SendMsg>
void TLS<RecvMsg, SendMsg>::threadListen(const std::string senderReadableAddr, const sockaddr_in senderAddr, SSL* sender) {
    char readBuffer[READ_BUFFER_SIZE];
    try {
        while (true) {
            RecvMsg recvPayload = recv(readBuffer, sender);
            onRecv(recvPayload, sender);
        }
    }
    catch (const std::exception& e) { // on exception, close connection
        std::cerr << e.what() << std::endl;
    }

    // Remove the connection from the map
    {
        std::unique_lock lock(connectionsMutex);
        connections.erase(senderReadableAddr);
    }
    
    // SSL shutdown, close socket
    SSL_shutdown(sender);
    SSL_free(sender);
    close(SSL_get_fd(sender));
}

template <class RecvMsg, class SendMsg>
void TLS<RecvMsg, SendMsg>::broadcast(const SendMsg& payload) {
    std::shared_lock lock(connectionsMutex);
    for (const auto& [name, ssl] : connections) {
        try {
            send(payload, ssl);
        }
        catch (const std::exception& e) {} //TODO: What to do when broadcast fails?
    }
}

template <class RecvMsg, class SendMsg>
void TLS<RecvMsg, SendMsg>::loadOwnCertificates(SSL_CTX *ctx) {
    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, (path + "/" + netConf[myID].name + "_cert.pem").c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, (path + "/" + netConf[myID].name + "_key.pem").c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

template <class RecvMsg, class SendMsg>
void TLS<RecvMsg, SendMsg>::loadAcceptableCertificates(SSL_CTX *ctx) {
    for (const peer& addr : netConf) {
        SSL_CTX_load_verify_locations(ctx, (path + "/" + addr.name + "_cert.pem").c_str(), NULL);
    }
}

template <class RecvMsg, class SendMsg>
std::string TLS<RecvMsg, SendMsg>::readableAddr(const sockaddr_in& addr) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN);
    return std::string(ip) + ":" + std::to_string(ntohs(addr.sin_port));
}

template <class RecvMsg, class SendMsg>
void TLS<RecvMsg, SendMsg>::send(const SendMsg& payload, SSL* dest) {
    auto [sendData, out] = zpp::bits::data_out();
    auto serializeResult = out(payload);
    if (zpp::bits::failure(serializeResult)) {
        std::cerr << "Unable to serialize payload, error: " << errorMessage(serializeResult) << std::endl;
        return;
    }

    // 1. Send the size of the data
    int dataSize = sendData.size();
    if (SSL_write(dest, &dataSize, sizeof(dataSize)) <= 0)
        throw std::runtime_error("Connection closed on SSL_write");
    // 2. Send the data
    if (SSL_write(dest, sendData.data(), dataSize) <= 0)
        throw std::runtime_error("Connection closed on SSL_write");
}

template <class RecvMsg, class SendMsg>
RecvMsg TLS<RecvMsg, SendMsg>::recv(std::span<char> buffer, SSL* src) {
    int readLen;
    int dataSize;

    // 1. Read the size of the data
    readLen = SSL_read(src, &dataSize, sizeof(dataSize));
    if (readLen <= 0)
        throw std::runtime_error("Connection closed on SSL_read");
    // 2. Read the data.
    // Check whether we can use the existing buffer to read
    bool needNewBuffer = dataSize > buffer.size();
    std::span<char> actualBuffer;
    std::unique_ptr<char[]> biggerBuffer; // use a unique pointer to store larger buffer so it gets freed if we exit by throwing an exception
    if (needNewBuffer) {
        biggerBuffer = std::make_unique<char[]>(dataSize);
        actualBuffer = {biggerBuffer.get(), (size_t) dataSize};
    }   
    else
        actualBuffer = buffer;
    // Keep reading until everything has been received.
    int readSoFar = 0;
    while (readSoFar < dataSize) {
        readLen = SSL_read(src, actualBuffer.data() + readSoFar, dataSize - readSoFar);
        if (readLen <= 0)
            throw std::runtime_error("Connection closed on SSL_read");
        readSoFar += readLen;
    }
    
    zpp::bits::in in(actualBuffer);
    RecvMsg recvPayload;
    auto deserializeResult = in(recvPayload);
    if (zpp::bits::failure(deserializeResult))
        throw std::runtime_error("Unable to deserialize payload, error: " + errorMessage(deserializeResult));

    // Free memory or reset bits in buffer
    if (!needNewBuffer)
        memset(buffer.data(), 0, dataSize);

    return recvPayload;
}

/**
 * From https://github.com/eyalz800/zpp_bits#error-codes.
*/
template <class RecvMsg, class SendMsg>
std::string TLS<RecvMsg, SendMsg>::errorMessage(const std::errc errorCode) {
    switch (errorCode) {
        case std::errc::result_out_of_range:
            return "attempting to write or read from a too short buffer";
        case std::errc::no_buffer_space:
            return "growing buffer would grow beyond the allocation limits or overflow";
        case std::errc::value_too_large:
            return "varint (variable length integer) encoding is beyond the representation limits";
        case std::errc::message_size:
            return "message size is beyond the user defined allocation limits";
        case std::errc::not_supported:
            return "attempt to call an RPC that is not listed as supported";
        case std::errc::bad_message:
            return "attempt to read a variant of unrecognized type";
        case std::errc::invalid_argument:
            return "attempting to serialize null pointer or a value-less variant";
        case std::errc::protocol_error:
            return "attempt to deserialize an invalid protocol message";
        default:
            return "unknown error";
    }
}

template <class RecvMsg, class SendMsg>
void TLS<RecvMsg, SendMsg>::disableNaglesAlgorithm(int sock) {
    int flag = 1;
    int result = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
    if (result < 0) {
        std::cerr << "Unable to disable Nagle's algorithm" << std::endl;
        exit(EXIT_FAILURE);
    }
}

#endif