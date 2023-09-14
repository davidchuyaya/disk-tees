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
#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <thread>
#include <memory>
#include <stdexcept>
#include <filesystem>
#include "zpp_bits.h"
#include "tls.hpp"

static const int CONNECTION_RETRY_TIMEOUT_SEC = 1;

template <class RecvMsg>
TLS<RecvMsg>::TLS(const int id, const std::string name,  const NodeType ownType, const networkConfig netConf, const std::string path, const std::function<void(const RecvMsg&, const std::string&)> onRecv) : id(id), name(name), ownType(ownType), path(path), onRecv(onRecv) {
    if (ownType == Replica)
        startServer();

    newNetwork(netConf);
}

template <class RecvMsg>
void TLS<RecvMsg>::startServer()
{
    // Ignore broken pipe signals
    signal(SIGPIPE, SIG_IGN);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(NetworkConfig::getPort(ownType, id));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    disableNaglesAlgorithm(sock);
    setNonBlocking(sock);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Unable to bind" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (listen(sock, 1) < 0) {
        std::cerr << "Unable to listen" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Started server" << std::endl;

    serverSocket = sock;
    openSockets.emplace_back(pollfd{sock, POLLIN, 0});
}

template <class RecvMsg>
int TLS<RecvMsg>::acceptConnection() {
    sockaddr_in sockAddr;
    unsigned int len = sizeof(sockAddr);

    int client = accept(serverSocket, (struct sockaddr*)&sockAddr, &len);
    if (client < 0) {
        if (errno != EWOULDBLOCK && errno != EAGAIN)
            std::cerr << "Unable to accept" << std::endl;
        return -1;
    }
    disableNaglesAlgorithm(client);

    std::cout << "Client TCP connection accepted" << std::endl;

    SSL_CTX* serverCtx = SSL_CTX_new(TLS_server_method());
    SSL_CTX_set_verify(serverCtx, SSL_VERIFY_PEER, NULL);
    loadAcceptableCertificates(serverCtx);
    loadOwnCertificates(serverCtx);

    SSL *ssl = SSL_new(serverCtx);
    SSL_set_fd(ssl, client);

    // Block until connection is complete
    int rc = 0;
    while (rc <= 0) {
        rc = SSL_accept(ssl);
        bool success = blockOnErr(client, ssl, rc);
        if (!success) {
            ERR_print_errors_fp(stderr);
            return -1;
        }
    }

    std::cout << "Client SSL connection accepted" << std::endl;

    // Add connection to maps
    std::string addr = readableAddr(sockAddr);
    sslFromAddr[addr] = ssl;
    sslFromSocket[client] = ssl;
    addrFromSocket[client] = addr;
    socketFromAddr[addr] = client;

    return client;
}

template <class RecvMsg>
void TLS<RecvMsg>::connectToServer(const int peerId, const std::string& peerIp) {
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    loadAcceptableCertificates(ctx);
    loadOwnCertificates(ctx);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    disableNaglesAlgorithm(sock);
    setNonBlocking(sock);

    sockaddr_in sockAddr;
    sockAddr.sin_family = AF_INET;
    int port = NetworkConfig::getPort(Replica, peerId);
    sockAddr.sin_port = htons(port);
    inet_pton(AF_INET, peerIp.c_str(), &(sockAddr.sin_addr.s_addr));
    
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);
    SSL_set_tlsext_host_name(ssl, peerIp.c_str());
    SSL_set1_host(ssl, peerIp.c_str());

    std::string addr = peerIp + ":" + std::to_string(port);

    // Attempt to connect forever
    while (connect(sock, (struct sockaddr*) &sockAddr, sizeof(sockAddr)) != 0) {
        std::cout << "Unable to TCP connect to server " << addr << ", retrying in " << CONNECTION_RETRY_TIMEOUT_SEC << "s..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(CONNECTION_RETRY_TIMEOUT_SEC));
    }
    std::cout << "TCP connection to server successful" << std::endl;

    // Block until connection is complete
    int rc = 0;
    while (rc <= 0) {
        rc = SSL_connect(ssl);
        bool success = blockOnErr(sock, ssl, rc);
        if (!success) {
            std::cout << "Failed to connect to server" << std::endl;
            ERR_print_errors_fp(stderr);
            return;
        }
    }
    
    std::cout << "SSL connection to server successful" << std::endl;

    // Add connection to maps
    sslFromAddr[addr] = ssl;
    sslFromSocket[sock] = ssl;
    addrFromSocket[sock] = addr;
    socketFromAddr[addr] = sock;

    openSockets.emplace_back(pollfd{sock, POLLIN, 0});
}


template <class RecvMsg>
void TLS<RecvMsg>::runEventLoopOnce(const int timeout) {
    int rc = poll(openSockets.data(), openSockets.size(), timeout);
    if (rc == 0) // Timed out
        return;

    std::vector<int> newSockets;
    std::vector<int> deletedSocketIndices;
    for (int i = 0; i < openSockets.size(); i++) {
        pollfd& sock = openSockets[i];
        // std::cout << "Checking addr " << addrFromSocket[sock.fd] << " for messages" << std::endl;

        if (sock.revents == 0)
            continue;

        // std::cout << "Found message from addr " << addrFromSocket[sock.fd] << std::endl;
        
        // Server listens for connections
        if (sock.fd == serverSocket) {
            int client = acceptConnection();
            if (client != -1)
                newSockets.emplace_back(client);
        }
        else {
            // Listen to socket
            try {
                std::optional<RecvMsg> recvPayload = recv(sock.fd, sslFromSocket.at(sock.fd));
                if (recvPayload.has_value())
                    onRecv(recvPayload.value(), addrFromSocket.at(sock.fd));
            } catch (const std::exception& e) {           // on exception, close connection
                memset(readBuffer, 0, READ_BUFFER_SIZE);  // Clear read buffer in case it's corrupted
                std::cout << e.what() << std::endl;
                closeSocket(sock.fd);
                deletedSocketIndices.emplace_back(i);
            }
            // std::cout << "Completed message receive" << std::endl;
        }
    }

    // Modify openSockets
    for (int i = deletedSocketIndices.size() - 1; i >= 0; i--) {
        int index = deletedSocketIndices[i];
        openSockets.erase(openSockets.begin() + index);
    }
    for (int sock : newSockets)
        openSockets.emplace_back(pollfd{sock, POLLIN, 0});

}

template <class RecvMsg>
void TLS<RecvMsg>::closeSocket(const int sock) {
    std::string& addr = addrFromSocket[sock];
    sslFromAddr.erase(addr);
    addrFromSocket.erase(sock);
    socketFromAddr.erase(addr);
    SSL* ssl = sslFromSocket[sock];
    sslFromSocket.erase(sock);

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sock);
}

template <class RecvMsg>
template <class SendMsg>
void TLS<RecvMsg>::broadcastExcept(const SendMsg& payload, const std::string& doNotBroadcastToAddr) {
    for (const auto& [addr, ssl] : sslFromAddr) {
        // send to address if the connection exists
        if (addr != doNotBroadcastToAddr) {
            try {
                send<SendMsg>(payload, addr);
            } catch (const std::exception& e) {
                std::cout << "Failed to broadcast to addr: " << addr << std::endl;
            }  // If it's an actual connection failure, the recv will eventually fail
        }
    }
}

template <class RecvMsg>
template <class SendMsg>
void TLS<RecvMsg>::broadcast(const SendMsg& payload, const addresses& dests) {
    for (const std::string& addr : dests) {
        // send to address if the connection exists
        if (sslFromAddr.contains(addr)) {
            try {
                send<SendMsg>(payload, addr);
            }
            catch (const std::exception& e) {
                std::cout << "Failed to broadcast to addr: " << addr << std::endl;
            } // If it's an actual connection failure, the recv will eventually fail
        }
        else
            std::cout << "Could not broadcast to address: " << addr << std::endl;
    }
}

template <class RecvMsg>
template <class SendMsg>
void TLS<RecvMsg>::sendRoundRobin(const SendMsg& payload, const addresses& dests) {
    while (true) {
        roundRobinIndex += 1;
        if (roundRobinIndex >= dests.size())
            roundRobinIndex = 0;
        // send to address if the connection exists
        std::string addr = dests[roundRobinIndex];
        if (sslFromAddr.contains(addr)) {
            try {
                send<SendMsg>(payload, addr);
                return;
            }
            catch (const std::exception& e) {
                std::cout << "Failed to send round robin to addr: " << addr << std::endl;
            } // If it's an actual connection failure, the recv will eventually fail
        }
    }
}

template <class RecvMsg>
void TLS<RecvMsg>::newNetwork(const networkConfig& netConf) {
    // Connect to new nodes. Don't need to shut down old ones (they should die naturally).
    // Lower-numbered peers connect to higher-numbered peers
    for (auto& [peerId, peerIp] : netConf) {
        if (ownType == Client || (ownType == Replica && peerId > this->id))
            connectToServer(peerId, peerIp);
    }
}

template <class RecvMsg>
void TLS<RecvMsg>::loadOwnCertificates(SSL_CTX *ctx) {
    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, (path + "/" + name + "_cert.pem").c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, (path + "/" + name + "_key.pem").c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

template <class RecvMsg>
void TLS<RecvMsg>::loadAcceptableCertificates(SSL_CTX *ctx) {
    // All certificates in the right directory will be used
    SSL_CTX_load_verify_locations(ctx, NULL, path.c_str());
}

template <class RecvMsg>
std::string TLS<RecvMsg>::readableAddr(const sockaddr_in& addr) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN);
    return std::string(ip) + ":" + std::to_string(ntohs(addr.sin_port));
}

template <class RecvMsg>
template <class SendMsg>
void TLS<RecvMsg>::send(const SendMsg& payload, const std::string& addr) {
    auto [sendData, out] = zpp::bits::data_out();
    auto serializeResult = out(payload);
    if (zpp::bits::failure(serializeResult)) {
        std::cerr << "Unable to serialize payload, error: " << errorMessage(serializeResult) << std::endl;
        return;
    }

    int sock = socketFromAddr.at(addr);
    SSL* dest = sslFromAddr.at(addr);
    // 1. Send the size of the data
    int dataSize = sendData.size();
    blockingWrite(sock, dest, &dataSize, sizeof(dataSize));
    // 2. Send the data
    blockingWrite(sock, dest, sendData.data(), dataSize);
}

template <class RecvMsg>
std::optional<RecvMsg> TLS<RecvMsg>::recv(const int sock, SSL* src) {
    int readLen;
    int dataSize;
    // 1. Read the size of the data (non-blocking, because sometimes there is nothing to read)
    // std::cout << "Begin reading size" << std::endl;
    int rc = SSL_read(src, &dataSize, sizeof(dataSize));
    int err = SSL_get_error(src, rc);
    if (err == SSL_ERROR_WANT_READ) // Nothing to read
        return std::nullopt;
    // std::cout << "Read size: " << dataSize << std::endl;
    // 2. Read the data.
    // Check whether we can use the existing buffer to read
    bool needNewBuffer = dataSize > READ_BUFFER_SIZE;
    std::span<char> actualBuffer;
    std::unique_ptr<char[]> biggerBuffer; // use a unique pointer to store larger buffer so it gets freed if we exit by throwing an exception
    if (needNewBuffer) {
        biggerBuffer = std::make_unique<char[]>(dataSize);
        actualBuffer = {biggerBuffer.get(), (size_t) dataSize};
    }   
    else
        actualBuffer = readBuffer;
    // Keep reading until everything has been received.
    // std::cout << "Begin reading packet of size: " << dataSize << std::endl;
    blockingRead(sock, src, actualBuffer.data(), dataSize);
    // std::cout << "Read packet" << std::endl;
    
    zpp::bits::in in(actualBuffer);
    RecvMsg recvPayload;
    auto deserializeResult = in(recvPayload);
    if (zpp::bits::failure(deserializeResult))
        throw std::runtime_error("Unable to deserialize payload, error: " + errorMessage(deserializeResult));

    // Reset bits in buffer
    if (!needNewBuffer)
        memset(readBuffer, 0, dataSize);

    return recvPayload;
}

template <class RecvMsg>
void TLS<RecvMsg>::blockingWrite(const int sock, SSL* ssl, const void* buffer, const int bytes) {
    int rc = 0;
    while (rc <= 0) {
        rc = SSL_write(ssl, buffer, bytes);
        if (rc <= 0) {
            // block and retry
            bool success = blockOnErr(sock, ssl, rc);
            if (!success)
                throw std::runtime_error("Connection closed on SSL_write");
        }
    }
}

template <class RecvMsg>
void TLS<RecvMsg>::blockingRead(const int sock, SSL* ssl, char* buffer, const int bytes) {
    int readSoFar = 0;
    while (readSoFar < bytes) {
        // std::cout << "Attempting blocking read for " << bytes << " bytes, readSoFar: " << readSoFar << std::endl;
        int readLen = SSL_read(ssl, buffer + readSoFar, bytes - readSoFar);
        if (readLen <= 0) {
            // block and retry
            bool success = blockOnErr(sock, ssl, readLen);
            if (!success)
                throw std::runtime_error("Connection closed on SSL_read");
        }
        else
            readSoFar += readLen;
    }
}

template <class RecvMsg>
bool TLS<RecvMsg>::blockOnErr(const int sock, SSL* ssl, const int rc) {
    int err = SSL_get_error(ssl, rc);
    // std::cout << "SSL error: " << err << std::endl;
    if (err == SSL_ERROR_NONE)
        return true;
    else if (err == SSL_ERROR_WANT_READ) {
        pollfd fds[1] {pollfd{sock, POLLIN, 0}};
        // std::cout << "SSL blocking on WANT_READ" << std::endl;
        poll(fds, 1, -1);
        // std::cout << "SSL WANT_READ unblocked" << std::endl;
        return true;
    }
    else if (err == SSL_ERROR_WANT_WRITE) {
        pollfd fds[1] {pollfd{sock, POLLOUT, 0}};
        // std::cout << "SSL blocking on WANT_WRITE" << std::endl;
        poll(fds, 1, -1);
        // std::cout << "SSL WANT_WRITE unblocked" << std::endl;
        return true;
    }
    else {
        std::cerr << "SSL error: " << err << std::endl;
        return false;
    }
}

/**
 * From https://github.com/eyalz800/zpp_bits#error-codes.
*/
template <class RecvMsg>
std::string TLS<RecvMsg>::errorMessage(const std::errc errorCode) {
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

template <class RecvMsg>
void TLS<RecvMsg>::disableNaglesAlgorithm(const int sock) {
    int flag = 1;
    int result = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
    if (result < 0) {
        std::cerr << "Unable to disable Nagle's algorithm" << std::endl;
        exit(EXIT_FAILURE);
    }
}

template <class RecvMsg>
void TLS<RecvMsg>::setNonBlocking(const int sock) {
    if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0) {
        std::cerr << "Unable to set socket to non-blocking" << std::endl;
        exit(EXIT_FAILURE);
    }
}

#endif