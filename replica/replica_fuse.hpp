#pragma once
#include <string>
#include <map>
#include <set>
#include "../shared/fuse_messages.hpp"
#include "../shared/tls.hpp"
#include "../shared/tls.cpp"

class ReplicaFuse {
public:
    ReplicaFuse(const int id, const std::string& name, const std::string& trustedMode, const std::string& directory, const networkConfig& ccf, const std::string& path);
    void addClientTLS(TLS<clientMsg> *tls);
    // Visitor pattern: https://www.cppstories.com/2018/09/visit-variants/. One for every possible type in clientMsg
    void operator()(const mknodParams& params);
    void operator()(const mkdirParams& params);
    void operator()(const symlinkParams& params); 
    void operator()(const unlinkParams& params);
    void operator()(const rmdirParams& params);
    void operator()(const renameParams& params);
    void operator()(const linkParams& params);
    void operator()(const chmodParams& params);
    void operator()(const chownParams& params);
    void operator()(const truncateParams& params);
    void operator()(const utimensParams& params);
    void operator()(const createParams& params);
    void operator()(const openParams& params);
    void operator()(const writeBufParams& params);
    void operator()(const releaseParams& params);
    void operator()(const fsyncParams& params);
    void operator()(const fallocateParams& params);
    void operator()(const setxattrParams& params);
    void operator()(const removexattrParams& params);
    void operator()(const copyFileRangeParams& params);
    void operator()(const p1a& msg);
    void operator()(const diskReq& msg);
    void operator()(const p2a& msg);
    std::string sender; // Set before each visitor function is called

private:
    const int id;
    const std::string name;
    const std::string trustedMode;
    const std::string directory;
    const networkConfig ccf;
    const std::string path;
    std::string client; // the client with the highest ballot (reset in p1a)
    addresses replicas; // the replicas in the latest config (reset in p1a)
    networkConfig replicasNetConf;
    int written = -1;
    ballot highestBallot = {0,0,0};
    ballot normalBallot = {0,0,0};
    std::map<int, clientMsg> pendingWrites = {};
    std::map<int, fsyncParams> pendingFsyncs = {}; // Separate fsync from other writes, since fsync does not increment the write count
    std::map <int, std::chrono::steady_clock::time_point> fsyncRecvTime = {}; // TODO: Send fsyncMissing on timeout
    std::map<int, int> fileHandleConverter = {};
    TLS<clientMsg> *clientTLS;

    bool preWriteCheck(const int seq, const ballot& r, const clientMsg& msg);
    void postWriteCheck(const int seq);
    std::chrono::steady_clock::time_point now();
};