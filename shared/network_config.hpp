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
 *     "ip": "127.0.0.1", // Private IP of the node
 *     "replicaPort": 4433, // Port for listening to other replicas
 *     "clientPort": 5433, // Port for listening to clients
 *     "ccfPort": 6433, // Port for listening to CCF
 *     "name": "disk_tee0", // Certificate should be disk_tee0_cert.pem, key should be disk_tee0_key.pem (if this replica owns the key),
 *                             and they must be in the main directory (disk-tees)
 *     "id": 0 // 0-indexed
 *   }
 * ]
*/
networkConfig readNetworkConfig(const std::string& file);