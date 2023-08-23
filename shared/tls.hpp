#pragma once
#include <openssl/ssl.h>
#include <arpa/inet.h>
#include <shared_mutex>
#include <map>
#include <vector>
#include <functional>
#include <string>
#include <span>
#include "network_config.hpp"

template <class RecvMsg, class SendMsg>
class TLS {
    public:
        TLS(const int myID, const std::vector<peer> netConf, const std::string path, const std::function<void(RecvMsg, SSL*)> onRecv);
        void broadcast(const SendMsg& payload);
        void send(const SendMsg& payload, const std::string& dest);
        void send(const SendMsg& payload, SSL* dest);

    private:
        const int myID;
        const std::vector<peer> netConf;
        const std::string path;
        const std::function<void(RecvMsg, SSL*)> onRecv;
        
        std::shared_mutex connectionsMutex;
        std::map<std::string, SSL*> connections;

        void startServer();
        void connectToServer(const peer& addr);
        void threadListen(const std::string senderReadableAddr, const sockaddr_in senderAddr, SSL* sender);
        void loadOwnCertificates(SSL_CTX *ctx);
        void loadAcceptableCertificates(SSL_CTX *ctx);
        std::string readableAddr(const sockaddr_in& addr); // Format: ip::port
        RecvMsg recv(std::span<char> buffer, SSL* src); // Will allocate a new buffer if provided buffer doesn't fit
        std::string errorMessage(const std::errc errorCode);
        void disableNaglesAlgorithm(int sock);
};