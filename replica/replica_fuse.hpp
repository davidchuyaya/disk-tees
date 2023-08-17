#pragma once
#include <string>
#include <map>
#include <set>
#include "../shared/fuse_messages.hpp"

class ReplicaFuse {
public:
    // Visitor pattern: https://www.cppstories.com/2018/09/visit-variants/. One for every possible type in fuseMethodParams
    void operator()(const unlinkParams& params);
    void operator()(const truncateParams& params);
    void operator()(const createParams& params);
    void operator()(const writeBufParams& params);
    void operator()(const fsyncParams& params);
    void operator()(const releaseParams& params);
private:
    int written = -1; // TODO: Clear state on reconnect
    round highestRound;
    round normalRound;
    std::map<int, clientMsg> pendingWrites;
    std::map<int, fsyncParams> pendingFsyncs; // Separate fsync from other writes, since fsync does not increment the write count
    std::map<int, int> fileHandleConverter;

    bool preWriteCheck(const int seq, const round& r, const clientMsg& msg);
    void postWriteCheck(const int seq);
};