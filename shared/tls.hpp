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
        typedef std::function<void(RecvMsg, std::string, SSL*)> msgRecvFunc;
        TLS(const int id, const std::string name, const NodeType remoteType, const NodeType ownType,
             const networkConfig netConf, const std::string path, const std::function<void(const RecvMsg&, const std::string&, SSL*)> onRecv);
        // WARN: Only use this broadcast if the set of destinations is not available through some config (such as the set of clients)
        void broadcast(const SendMsg& payload);
        void broadcast(const SendMsg& payload, const addresses& dests);
        void sendRoundRobin(const SendMsg& payload, const addresses& dests);
        void send(const SendMsg& payload, SSL* dest);
        void newNetwork(const networkConfig& netConf);

    private:
        const int id;
        const std::string name;
        const NodeType remoteType;
        const NodeType ownType;
        const std::string path;
        const std::function<void(const RecvMsg&, const std::string&, SSL*)> onRecv;
        
        std::shared_mutex connectionsMutex;
        std::map<std::string, SSL*> connections;
        int roundRobinIndex = 0;

        void startServer();
        void connectToServer(const int peerId, const std::string& peerIp);
        void threadListen(const std::string& addr, SSL* sender);
        void loadOwnCertificates(SSL_CTX *ctx);
        void loadAcceptableCertificates(SSL_CTX *ctx);
        std::string readableAddr(const sockaddr_in& addr); // Format: ip:port
        RecvMsg recv(std::span<char> buffer, SSL* src); // Will allocate a new buffer if provided buffer doesn't fit
        std::string errorMessage(const std::errc errorCode);
        void disableNaglesAlgorithm(const int sock);
};