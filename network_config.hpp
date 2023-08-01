#pragma once
#include <arpa/inet.h>
#include <string>
#include <vector>

struct networkConfig {
    std::vector<sockaddr_in> peers;
    std::vector<std::string> names;
};

networkConfig readNetworkConfig(const std::string file);