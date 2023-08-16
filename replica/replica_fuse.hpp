#pragma once
#include <string>
#include <map>
#include "../shared/fuse_messages.hpp"

class ReplicaFuse {
public:
    // Visitor pattern: https://www.cppstories.com/2018/09/visit-variants/. One for every possible type in fuseMethodParams
    void operator()(const truncateParams& params);
    void operator()(const createParams& params);
    void operator()(const writeBufParams& params);
    void operator()(const fsyncParams& params);
    void operator()(const releaseParams& params);
private:
    std::map<int, int> fileHandleConverter;
};