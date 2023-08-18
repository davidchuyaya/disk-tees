/**
 * Logic mostly copied from https://github.com/davidchuyaya/TEE-fuse/blob/master/example/passthrough_fh.c
*/
#include <unistd.h>
#include <sys/file.h>
#include <sys/xattr.h>
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

void ReplicaFuse::operator()(const mknodParams &params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;
    
    int res;
    if (S_ISFIFO(params.mode))
        res = mkfifo(REPLICA_PREPEND_PATH(params.path), params.mode);
    else
        res = mknod(REPLICA_PREPEND_PATH(params.path), params.mode, params.rdev);
    if (res == -1) {
        std::cerr << "Unexpected failure to mknod at path: " << params.path << ", errno: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    postWriteCheck(params.seq);
}

void ReplicaFuse::operator()(const mkdirParams &params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;

    int res = mkdir(REPLICA_PREPEND_PATH(params.path), params.mode);
    if (res == -1) {
        std::cerr << "Unexpected failure to mkdir at path: " << params.path << ", errno: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    postWriteCheck(params.seq);
}

void ReplicaFuse::operator()(const symlinkParams &params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;
    
    int res = symlink(params.target.c_str(), REPLICA_PREPEND_PATH(params.linkpath));
    if (res == -1) {
        std::cerr << "Unexpected failure to symlink from path " << params.target << " to path " << params.linkpath << ", errno: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void ReplicaFuse::operator()(const unlinkParams &params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;

    int res = unlink(REPLICA_PREPEND_PATH(params.path));
    if (res == -1) {
        std::cerr << "Unexpected failure to unlink at path: " << params.path << ", errno: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    postWriteCheck(params.seq);
}

void ReplicaFuse::operator()(const rmdirParams &params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;

    int res = rmdir(REPLICA_PREPEND_PATH(params.path));
    if (res == -1) {
        std::cerr << "Unexpected failure to rmdir at path: " << params.path << ", errno: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    postWriteCheck(params.seq);
}

void ReplicaFuse::operator()(const renameParams &params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;

    int res = rename(REPLICA_PREPEND_PATH(params.oldpath), REPLICA_PREPEND_PATH(params.newpath));
    if (res == -1) {
        std::cerr << "Unexpected failure to rename from path " << params.oldpath << " to path " << params.newpath << ", errno: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    postWriteCheck(params.seq);
}

void ReplicaFuse::operator()(const linkParams &params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;

    int res = link(REPLICA_PREPEND_PATH(params.oldpath), REPLICA_PREPEND_PATH(params.newpath));
    if (res == -1) {
        std::cerr << "Unexpected failure to link from path " << params.oldpath << " to path " << params.newpath << ", errno: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    postWriteCheck(params.seq);
}

void ReplicaFuse::operator()(const chmodParams &params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;
    
    int res;
    if (params.hasFi)
        res = fchmod(fileHandleConverter.at(params.fi.fh), params.mode);
    else   
        res = chmod(REPLICA_PREPEND_PATH(params.path), params.mode);
    if (res == -1) {
        std::cerr << "Unexpected failure to chmod at path: " << params.path << ", fh: " << params.fi.fh <<
            ", localFh: " << fileHandleConverter.at(params.fi.fh) << ", errno: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    postWriteCheck(params.seq);
}

void ReplicaFuse::operator()(const chownParams &params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;
    
    int res;
    if (params.hasFi)
        res = fchown(fileHandleConverter.at(params.fi.fh), params.uid, params.gid);
    else   
        res = lchown(REPLICA_PREPEND_PATH(params.path), params.uid, params.gid);
    if (res == -1) {
        std::cerr << "Unexpected failure to chown at path: " << params.path << ", fh: " << params.fi.fh <<
            ", localFh: " << fileHandleConverter.at(params.fi.fh) << ", errno: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    postWriteCheck(params.seq);
}

void ReplicaFuse::operator()(const truncateParams& params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;

    int res;
    if (params.hasFi)
        res = ftruncate(fileHandleConverter.at(params.fi.fh), params.size);
    else
        res = truncate(REPLICA_PREPEND_PATH(params.path), params.size);
    if (res == -1) {
        std::cerr << "Unexpected failure to truncate at path: " << params.path << ", fh: " << params.fi.fh <<
            ", localFh: " << fileHandleConverter.at(params.fi.fh) << ", errno: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    postWriteCheck(params.seq);
}

void ReplicaFuse::operator()(const utimensParams &params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;
    
    int res;
    const timespec tv[]{params.tv0, params.tv1};
    if (params.hasFi)
        res = futimens(fileHandleConverter.at(params.fi.fh), tv);
    else   
        res = utimensat(0, REPLICA_PREPEND_PATH(params.path), tv, AT_SYMLINK_NOFOLLOW);
    if (res == -1) {
        std::cerr << "Unexpected failure to utimens at path: " << params.path << ", fh: " << params.fi.fh <<
            ", localFh: " << fileHandleConverter.at(params.fi.fh) << ", errno: " << std::strerror(errno) << std::endl;
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
    // std::cout << "Fh " << params.fi.fh << " points to " << localFd << " at path " << params.path << std::endl;

    postWriteCheck(params.seq);
}

void ReplicaFuse::operator()(const openParams& params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;

    int localFd = open(REPLICA_PREPEND_PATH(params.path), params.fi.flags);
    if (localFd == -1) {
        std::cerr << "Unexpected failure to open at path: " << params.path << ", errno: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    fileHandleConverter[params.fi.fh] = localFd;
    // std::cout << "Fh " << params.fi.fh << " points to " << localFd << " at path " << params.path << std::endl;

    postWriteCheck(params.seq);
}

void ReplicaFuse::operator()(const writeBufParams& params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;

    int res = pwrite(fileHandleConverter.at(params.fi.fh), params.buf.data(), params.buf.size(), params.offset);
    if (res == -1) {
        std::cerr << "Unexpected failure to write at fh: " << params.fi.fh <<
            ", localFh: " << fileHandleConverter.at(params.fi.fh) << ", errno: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    postWriteCheck(params.seq); 
}

void ReplicaFuse::operator()(const releaseParams& params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;

    int res = close(fileHandleConverter.at(params.fi.fh));
    if (res == -1) {
        std::cerr << "Unexpected failure to release at fh: " << params.fi.fh <<
            ", localFh: " << fileHandleConverter.at(params.fi.fh) << ", errno: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    // std::cout << "Fh " << params.fi.fh << " no longer points to " << fileHandleConverter.at(params.fi.fh) << std::endl;
    fileHandleConverter.erase(params.fi.fh);

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

void ReplicaFuse::operator()(const fallocateParams &params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;

    posix_fallocate(fileHandleConverter.at(params.fi.fh), params.offset, params.length);

    postWriteCheck(params.seq);
}

void ReplicaFuse::operator()(const setxattrParams &params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;
    
    // std::cout << "setxattr at path: " << params.path << ", name: " << params.name << ", value: " << params.value << ", flags: " << params.flags << std::endl;
    
    int res = lsetxattr(REPLICA_PREPEND_PATH(params.path), params.name.c_str(), params.value.data(), params.value.size(), params.flags);
    if (res == -1) {
        std::cerr << "Unexpected failure to setxattr at path: " << params.path << ", errno: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    postWriteCheck(params.seq);
}

void ReplicaFuse::operator()(const removexattrParams &params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;
    
    int res = lremovexattr(REPLICA_PREPEND_PATH(params.path), params.name.c_str());
    if (res == -1) {
        std::cerr << "Unexpected failure to removexattr at path: " << params.path << ", errno: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    postWriteCheck(params.seq);
}

void ReplicaFuse::operator()(const copyFileRangeParams &params) {
    if (!preWriteCheck(params.seq, params.r, params))
        return;
    
    off_t off_in = params.off_in;
    off_t off_out = params.off_out;
    int res = copy_file_range(fileHandleConverter.at(params.fi_in.fh), &off_in, fileHandleConverter.at(params.fi_out.fh), &off_out, params.len, params.flags);
    if (res == -1) {
        std::cerr << "Unexpected failure to copy_file_range, errno: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

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