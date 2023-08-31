#pragma once
#include <sys/poll.h>
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
    static const int READ_BUFFER_SIZE = 8192;

    public:
        typedef std::function<void(RecvMsg, std::string, SSL*)> msgRecvFunc;
        TLS(const int id, const std::string name, const NodeType remoteType, const NodeType ownType,
             const networkConfig netConf, const std::string path, const std::function<void(const RecvMsg&, const std::string&, SSL*)> onRecv);
        void runEventLoopOnce(const int timeout); // 0 = return immediately, -1 = block until event. In milliseconds
        // WARN: Only use this broadcast if the set of destinations is not available through some config (such as the set of clients)
        void broadcast(const SendMsg& payload);
        void broadcast(const SendMsg& payload, const addresses& dests);
        void sendRoundRobin(const SendMsg& payload, const addresses& dests);
        void send(const SendMsg& payload, const std::string& addr);
        void newNetwork(const networkConfig& netConf);

    private:
        const int id;
        const std::string name;
        const NodeType remoteType;
        const NodeType ownType;
        const std::string path;
        const std::function<void(const RecvMsg&, const std::string&, SSL*)> onRecv;

        std::vector<pollfd> openSockets;
        int serverSocket;

        char readBuffer[READ_BUFFER_SIZE];
        
        std::map<std::string, SSL*> sslFromAddr;
        std::map<int, SSL*> sslFromSocket;
        std::map<int, std::string> addrFromSocket;
        std::map<std::string, int> socketFromAddr;
        SSL_CTX* serverCtx;
        int roundRobinIndex = 0;

        void startServer();
        int acceptConnection(); // returns socket of new connection, -1 if non exists
        void connectToServer(const int peerId, const std::string& peerIp);
        void closeSocket(const int sock);
        void loadOwnCertificates(SSL_CTX *ctx);
        void loadAcceptableCertificates(SSL_CTX *ctx);
        std::string readableAddr(const sockaddr_in& addr); // Format: ip:port
        RecvMsg recv(const int sock, SSL* src); // Will allocate a new buffer if provided buffer doesn't fit
        void blockingWrite(const int sock, SSL* ssl, const void* buffer, const int bytes);
        void blockingRead(const int sock, SSL* ssl, char* buffer, const int bytes);
        bool blockOnErr(const int sock, SSL* ssl, const int rc); // Returns false if there's an actual error
        std::string errorMessage(const std::errc errorCode);
        void disableNaglesAlgorithm(const int sock);
        void setNonBlocking(const int sock);
};