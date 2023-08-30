#pragma once
#include <arpa/inet.h>
#include <string>
#include <vector>
#include <map>

typedef std::map<int, std::string> networkConfig;
typedef std::vector<std::string> addresses;
enum NodeType {
    Client,
    Replica
};

namespace NetworkConfig {
    static const int CLIENT_START_PORT = 10000;
    static const int REPLICA_START_PORT = 10100;

    /**
     * Expected JSON format:
     * [
     *   {
     *     "ip": "127.0.0.1", // Private IP of the node
     *     "id": 0 
     *   }
     * ]
    */
    networkConfig readNetworkConfig(const std::string& file);
    addresses configToAddrs(const NodeType type, const networkConfig& conf);
    int getPort(const NodeType type, const int id);
}