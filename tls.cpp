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
#include <mutex>
#include "zpp_bits.h"
#include "tls.hpp"

static const int CONNECTION_RETRY_TIMEOUT_SEC = 5;
static const int READ_BUFFER_SIZE = 256;

TLS::TLS(const int myID, const networkConfig netConf,
            const std::function<void(clientInPayload)> onClientMsg, 
            const std::function<void(diskTeePayload)> onDiskTeeMsg) :
            myID(myID), netConf(netConf), onClientMsg(onClientMsg), onDiskTeeMsg(onDiskTeeMsg) {
    // Connect to peers asynchronously
    for (int i = 0; i < netConf.peers.size(); i++) {
        if (i == myID)
            continue;
        const sockaddr_in& peer = netConf.peers[i];
        std::thread(&TLS::connectToServer, this, ipFromSockAddr(peer), peer).detach();
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

        // Add connection to map
        // TODO: If connection already exists, something's going on (TLS impersonation)
        const std::string ip = ipFromSockAddr(addr);
        {
            std::unique_lock lock(connectionsMutex);
            connections.emplace(ip, ssl);
        }

        // Listen to the client on an async thread
        std::thread connectionThread = std::thread(&TLS::threadListen, this, ip, addr, ssl);
        connectionThread.detach();
    }

    // close server
    close(sock);
    SSL_CTX_free(ctx);
}

void TLS::connectToServer(const std::string serverIp, const sockaddr_in serverAddr) {
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

    // Add connection to map
    {
        std::unique_lock lock(connectionsMutex);
        connections.emplace(serverIp, ssl);
    }

    /* Loop to send input from keyboard */
    while (true) {
        auto [sendData, out] = zpp::bits::data_out();
        std::string input;
        std::getline(std::cin, input);
        diskTeePayload sendPayload = { .data = input };
        auto serializeResult = out(sendPayload);
        if (failure(serializeResult)) {
            std::cerr << "Unable to serialize payload, error: " << errorMessage(serializeResult) << std::endl;
            continue;
        }
        
        /* Send it to the server */
        int result;
        if ((result = SSL_write(ssl, sendData.data(), sendData.size())) <= 0) {
            printf("Server closed connection on SSL_write\n");
            ERR_print_errors_fp(stderr);
            break;
        }

        /* Wait for the echo */
        char rxbuf[READ_BUFFER_SIZE];
        int rxlen = SSL_read(ssl, rxbuf, READ_BUFFER_SIZE);
        if (rxlen <= 0) {
            printf("Server closed connection on SSL_read\n");
            ERR_print_errors_fp(stderr);
            break;
        } else {
            /* Show it */
            zpp::bits::in in(rxbuf);
            diskTeePayload recvPayload;
            auto deserializeResult = in(recvPayload);
            if (failure(deserializeResult)) {
                std::cerr << "Unable to deserialize payload, error: " << errorMessage(deserializeResult) << std::endl;
                continue;
            }
            
            std::cout << "Received: " << recvPayload.data << std::endl;
        }
    }

    // TODO: Instead of ever releasing the socket, the client should always try to reconnect
    // Remove the connection from the map
    {
        std::unique_lock lock(connectionsMutex);
        connections.erase(serverIp);
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(SSL_get_fd(ssl));
    SSL_CTX_free(ctx);
}

void TLS::threadListen(const std::string senderIp, const sockaddr_in senderAddr, SSL* sender) {
    while (true) {
        int readLen;
        char readBuffer[READ_BUFFER_SIZE];

        if ((readLen = SSL_read(sender, readBuffer, READ_BUFFER_SIZE)) <= 0) {
            if (readLen == 0) {
                printf("Client closed connection on SSL_read\n");
            } else {
                printf("SSL_read returned %d\n", readLen);
            }
            ERR_print_errors_fp(stderr);
            break;
        }
        
        zpp::bits::in in(readBuffer);
        diskTeePayload recvPayload;
        auto deserializeResult = in(recvPayload);
        if (failure(deserializeResult)) {
            std::cerr << "Unable to deserialize payload, error: " << errorMessage(deserializeResult) << std::endl;
            continue;
        }

        std::cout << "Received: " << recvPayload.data << std::endl;

        /* Echo it back */
        auto [sendData, out] = zpp::bits::data_out();
        auto serializeResult = out(recvPayload);
        if (failure(serializeResult)) {
            std::cerr << "Unable to serialize payload, error: " << errorMessage(serializeResult) << std::endl;
            continue;
        }
        
        if (SSL_write(sender, sendData.data(), sendData.size()) <= 0) {
            printf("Client closed connection on SSL_write\n");
            ERR_print_errors_fp(stderr);
            break;
        }
    }

    // Remove the connection from the map
    {
        std::unique_lock lock(connectionsMutex);
        connections.erase(senderIp);
    }
    
    // SSL shutdown, close socket
    SSL_shutdown(sender);
    SSL_free(sender);
    close(SSL_get_fd(sender));
}

template <class T>
void TLS::broadcastToPeers(const T& payload) {
    auto sendData = serialize(payload);

    std::shared_lock lock(connectionsMutex);
    for (const auto& [name, ssl] : connections) {
        if (SSL_write(ssl, sendData.data(), sendData.size()) <= 0) {
            std::cerr << "Tried to write to broken peer" << std::endl;
            ERR_print_errors_fp(stderr);
            continue;
        }
    }
}

template <class T>
void TLS::sendToClient(const T& payload) {
    auto sendData = serialize(payload);

    std::shared_lock lock(connectionsMutex);
    if (SSL_write(client, sendData.data(), sendData.size()) <= 0) {
        std::cerr << "Tried to write to broken client" << std::endl;
        ERR_print_errors_fp(stderr);
    }
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

std::string TLS::ipFromSockAddr(const sockaddr_in& addr) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN);
    return std::string(ip) + ":" + std::to_string(ntohs(addr.sin_port));
}

template <class T>
std::vector<std::byte> TLS::serialize(const T& payload) {
    auto [sendData, out] = zpp::bits::data_out();
    auto serializeResult = out(payload);
    if (failure(serializeResult)) {
        std::cerr << "Unable to serialize payload, error: " << errorMessage(serializeResult) << std::endl;
        return {};
    }
    return sendData;
}

/**
 * From https://github.com/eyalz800/zpp_bits#error-codes.
*/
std::string TLS::errorMessage(const std::errc errorCode) {
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