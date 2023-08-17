/**
 * Logic mostly copied from https://github.com/davidchuyaya/TEE-fuse/blob/master/example/passthrough_fh.c
*/
#include <unistd.h>
#include <sys/file.h>
#include <iostream>
#include <variant>
#include <cerrno>
#include <cstring>
#include "replica_fuse.hpp"

// If defined, will write files to the directory specified below. Used for testing locally.
#define REPLICA_CUSTOM_DIR
#ifdef REPLICA_CUSTOM_DIR
#define REPLICA_PREPEND_PATH(path) (("/home/davidchuyaya/disk-tees-test" + std::string(path)).c_str())
#elif
#define REPLICA_PREPEND_PATH(path) (path)
#endif

void ReplicaFuse::operator()(const unlinkParams& params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;

    int res = unlink(REPLICA_PREPEND_PATH(params.path));
    if (res == -1) {
        std::cerr << "Unexpected failure to unlink at path: " << params.path << ", errno: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    postWriteCheck(params.seq);
}

void ReplicaFuse::operator()(const truncateParams& params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;

    int localFd = fileHandleConverter.at(params.fi.fh);
    int res = ftruncate(localFd, params.size);
    if (res == -1) {
        std::cerr << "Unexpected failure to truncate, errno: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    postWriteCheck(params.seq);
}

void ReplicaFuse::operator()(const createParams& params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;

    int localFd = open(REPLICA_PREPEND_PATH(params.path), params.fi.flags, params.mode);
    if (localFd == -1) {
        std::cerr << "Unexpected failure to create at path: " << params.path << ", errno: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    fileHandleConverter[params.fi.fh] = localFd;

    postWriteCheck(params.seq);
}

void ReplicaFuse::operator()(const writeBufParams& params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;

    int localFd = fileHandleConverter.at(params.fi.fh);
    int res = pwrite(localFd, params.buf.data(), params.buf.size(), params.offset);
    if (res == -1) {
        std::cerr << "Unexpected failure to write, errno: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    postWriteCheck(params.seq); 
}

void ReplicaFuse::operator()(const fsyncParams& params) {
    // Can't use standard preWriteCheck, since we want to add this to pendingFsyncs instead of pendingWrites
    // Code is mostly copy pasted from pendingWrites though
    if (params.r != normalRound || params.r < highestRound)
        return;
    // TODO: If r > normalRound and r >= highestRound, might want to cache the write and wait for p2a with that ballot to conclude
    if (params.seq > written) {
        pendingFsyncs[params.seq] = params;
        return;
    }
    // TODO: Reply to client to confirm commit
    // Don't use postWriteCheck, since written was not incremented
}

void ReplicaFuse::operator()(const releaseParams& params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;

    int localFd = fileHandleConverter.at(params.fi.fh);
    int res = close(localFd);
    if (res == -1) {
        std::cerr << "Unexpected failure to release, errno: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    fileHandleConverter.erase(params.fi.fh);

    postWriteCheck(params.seq); 
}

bool ReplicaFuse::preWriteCheck(const int seq, const round& r, const clientMsg& msg) {
    if (r != normalRound || r < highestRound) {
        std::cout << "Attempted write with round " << r << " when normalRound is " << normalRound << " and highestRound is " << highestRound << std::endl;
        return false;
    }
    // TODO: If r > normalRound and r >= highestRound, might want to cache the write and wait for p2a with that ballot to conclude
    if (seq != written + 1) {
        pendingWrites[seq] = msg;
        std::cout << "Attempted write at seq " << seq << " when written is " << written << std::endl;
        return false;
    }
    // TODO: Broadcast to other replicas
    return true;
}

void ReplicaFuse::postWriteCheck(const int seq) {
    pendingWrites.erase(seq);

    // check fsync
    const auto& f = pendingFsyncs.find(seq);
    if (f != pendingFsyncs.end())
        (*this)(f->second);

    written++;

    // check pending writes
    const auto& w = pendingWrites.find(seq);
    if (w != pendingWrites.end())
        std::visit(*this, w->second);
}