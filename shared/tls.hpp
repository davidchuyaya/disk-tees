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

struct clientInPayload {
    // TODO: Writes from the client
};

const int MSG_TYPE_WRITE_ACK = 0;
const int MSG_TYPE_FSYNC_ACK = 1;
const int MSG_TYPE_RECOVERY = 2;
struct clientOutPayload {
    int messageType;
    int seq;
    int maxContSeq;
    // TODO: Add field to store file system nodes
};

struct diskTeePayload {
    // TODO: Message types for recovery
    std::string data;
};

class TLS {
    public:
        TLS(const int myID, const std::vector<networkConfig> netConf,
            const std::function<void(clientInPayload)> onClientMsg, 
            const std::function<void(diskTeePayload)> onDiskTeeMsg);
        void startServer();
        void connectToServer(const networkConfig& peer);
        template <class T>
        void broadcastToPeers(const T& payload); 
        template <class T>
        void sendToClient(const T& payload); 

    private:
        const int myID;
        const std::vector<networkConfig> netConf;
        const std::function<void(clientInPayload)> onClientMsg;
        const std::function<void(diskTeePayload)> onDiskTeeMsg;
        
        std::shared_mutex connectionsMutex;
        SSL* client;
        std::map<std::string, SSL*> connections;

        void threadListen(const std::string senderReadableAddr, const sockaddr_in senderAddr, SSL* sender);
        void loadOwnCertificates(SSL_CTX *ctx);
        void loadAcceptableCertificates(SSL_CTX *ctx);
        std::string readableAddr(const sockaddr_in& addr); // Format: ip::port
        template <class T>
        void send(const T& payload, SSL* dest);
        template <class T>
        T recv(std::span<char> buffer, SSL* src); // Will allocate a new buffer if provided buffer doesn't fit
        std::string errorMessage(const std::errc errorCode);
};