#pragma once
#include <arpa/inet.h>
#include <string>
#include <vector>

struct networkConfig {
    std::string ip;
    int port;
    std::string name;
};

/**
 * Expected JSON format:
 * [
 *   {
 *     "ip": "127.0.0.1",
 *     "port": 4433,
 *     "name": "disk_tee0", // Certificate should be disk_tee0.pem
 *     "id": 0 // 0-indexed
 *   }
 * ]
*/
std::vector<networkConfig> readNetworkConfig(const std::string& file);