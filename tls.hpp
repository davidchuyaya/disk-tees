#include <openssl/ssl.h>
#include <arpa/inet.h>
#include <shared_mutex>
#include <map>
#include <vector>
#include <functional>

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
};

class TLS {
    public:
        TLS(const std::vector<sockaddr_in> peers, const int port, 
            const std::function<void(clientInPayload)> onClientMsg, 
            const std::function<void(diskTeePayload)> onDiskTeeMsg);
        void broadcastToPeers(); // TODO: Add payload parameter
        void sendToClient(); // TODO: Add payload parameter

    private:
        const std::vector<sockaddr_in> peers;
        const std::function<void(clientInPayload)> onClientMsg;
        const std::function<void(diskTeePayload)> onDiskTeeMsg;
        
        SSL* client;
        std::shared_mutex connectionsMutex;
        std::map<sockaddr_in, SSL*> connections;

        void startServer(const int port);
        void connectToServer(const sockaddr_in server);
        void threadListen(const sockaddr_in senderAddr, SSL* sender);
        void configure_context(SSL_CTX *ctx, const std::string cert, const std::string key);
};

void runClient(const int port);