#pragma once
#include <string>
#include <map>
#include <set>
#include "../shared/fuse_messages.hpp"
#include "../shared/tls.hpp"
#include "../shared/tls.cpp"

class ReplicaFuse {
public:
    ReplicaFuse(const int id, const std::string& directory);
    void addTLS(TLS<clientMsg, replicaMsg> *tls);
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

private:
    int id;
    int written = -1; // TODO: Clear state on reconnect
    round highestRound;
    round normalRound;
    std::string directory;
    std::map<int, clientMsg> pendingWrites;
    std::map<int, fsyncParams> pendingFsyncs; // Separate fsync from other writes, since fsync does not increment the write count
    std::map<int, int> fileHandleConverter;
    TLS<clientMsg, replicaMsg> *clientTLS;

    bool preWriteCheck(const int seq, const round& r, const clientMsg& msg);
    void postWriteCheck(const int seq);
};