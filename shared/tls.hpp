#pragma once
#include <sys/poll.h>
#include <openssl/ssl.h>
#include <arpa/inet.h>
#include <map>
#include <vector>
#include <functional>
#include <string>
#include <span>
#include <system_error>
#include <optional>
#include "network_config.hpp"

template <class RecvMsg>
class TLS {
    static const int READ_BUFFER_SIZE = 8192;

    public:
        typedef std::function<void(RecvMsg, std::string, SSL*)> msgRecvFunc;
        TLS(const int id, const std::string name, const NodeType ownType,
             const networkConfig netConf, const std::string path, const std::function<void(const RecvMsg&, const std::string&)> onRecv);
        void runEventLoopOnce(const int timeout); // 0 = return immediately, -1 = block until event. In milliseconds
        template <class SendMsg>
        void broadcastExcept(const SendMsg& payload, const std::string& doNotBroadcastToAddr); // send to all connected except specified addr. Useful if trying to send to client connections with unknown addresses
        template <class SendMsg>
        void broadcast(const SendMsg& payload, const addresses& dests);
        template <class SendMsg>
        void sendRoundRobin(const SendMsg& payload, const addresses& dests);
        template <class SendMsg>
        void send(const SendMsg& payload, const std::string& addr);
        void newNetwork(const networkConfig& netConf);

    private:
        const int id;
        const std::string name;
        const NodeType ownType;
        const std::string path;
        const std::function<void(const RecvMsg&, const std::string&)> onRecv;

        char readBuffer[READ_BUFFER_SIZE];
        
        std::vector<pollfd> openSockets = {};
        int serverSocket;
        std::map<std::string, SSL*> sslFromAddr = {};
        std::map<int, SSL*> sslFromSocket = {};
        std::map<int, std::string> addrFromSocket = {};
        std::map<std::string, int> socketFromAddr = {};
        int roundRobinIndex = 0;

        void startServer();
        int acceptConnection(); // returns socket of new connection, -1 if non exists
        void connectToServer(const int peerId, const std::string& peerIp);
        void closeSocket(const int sock);
        void loadOwnCertificates(SSL_CTX *ctx);
        void loadAcceptableCertificates(SSL_CTX *ctx);
        std::string readableAddr(const sockaddr_in& addr); // Format: ip:port
        std::optional<RecvMsg> recv(const int sock, SSL* src); // Will allocate a new buffer if provided buffer doesn't fit
        void blockingWrite(const int sock, SSL* ssl, const void* buffer, const int bytes);
        void blockingRead(const int sock, SSL* ssl, char* buffer, const int bytes);
        bool blockOnErr(const int sock, SSL* ssl, const int rc); // Returns false if there's an actual error
        std::string errorMessage(const std::errc errorCode);
        void disableNaglesAlgorithm(const int sock);
        void setNonBlocking(const int sock);
};