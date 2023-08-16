#pragma once
#include <arpa/inet.h>
#include <string>
#include <vector>

struct peer {
    std::string ip;
    int port;
    std::string name;
};

struct networkConfig {
    std::vector<peer> ccfNodes;
    std::vector<peer> replicas;
    std::vector<peer> clients;
};

/**
 * Expected JSON format:
 * [
 *   {
 *     "ip": "127.0.0.1",
 *     "replicaPort": 4433,
 *     "clientPort": 5433,
 *     "ccfPort": 6433,
 *     "name": "disk_tee0", // Certificate should be disk_tee0.pem
 *     "id": 0 // 0-indexed
 *   }
 * ]
*/
networkConfig readNetworkConfig(const std::string& file);