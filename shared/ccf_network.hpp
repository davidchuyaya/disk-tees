#pragma once
#include <string>
#include <vector>
#include "fuse_messages.hpp"
#include "network_config.hpp"

struct matchB {
    ballot r;
    std::vector<networkConfig> configs;
};

class CCFNetwork {
public:
    // Note: Addresses should be formatted as "ip:port"
    CCFNetwork(const std::string& path, const networkConfig ccf);
    void getCert(const int id, const std::string& name, const std::string& type);
    matchB sendMatchA(const ballot r, const networkConfig config);
    void sendGarbageA(const ballot r);
private:
    const std::string path;
    std::vector<std::string> ccfAddresses;
    std::vector<std::string> liveAddresses;

    std::string ballotToString(const ballot& r);
    ballot ballotFromString(const std::string& s);
    void removeOutputFile(const std::string& outputFile);
    // Side effect: Sleeps if false
    bool curlSuccessful(const int errorCode);
    // Side effect: Sleeps if false
    bool requestSuccessful(const std::string& outputFile);
    void markLastAddressDead();
};