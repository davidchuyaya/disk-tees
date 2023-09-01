#pragma once
#include <string>
#include <map>
#include <vector>
#include <variant>
#include <span>
#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 31
#endif
#include <fuse.h>
#include "network_config.hpp"

struct ballot {
    int ballotNum = 0;
    int clientId = 0;
    int configNum = 0;

    bool operator ==(const ballot& rhs) const {
        return ballotNum == rhs.ballotNum && clientId == rhs.clientId && configNum == rhs.configNum;
    }

    bool operator <(const ballot& rhs) const {
        bool ballotNumLess = ballotNum < rhs.ballotNum;
        bool idLess = ballotNum == rhs.ballotNum && clientId < rhs.clientId;
        bool configLess = ballotNum == rhs.ballotNum && clientId == rhs.clientId && configNum < rhs.configNum;
        return ballotNumLess || idLess || configLess;
    }
};

// inline keyword: See https://stackoverflow.com/a/12802613/4028758
inline std::ostream& operator <<(std::ostream& out, const ballot& r) {
    return out << "(" << r.ballotNum << "," << r.clientId << "," << r.configNum << ")";
}

/**
 * Modified structs from fuse_common.h to simplify data, remove incompatibilities, and remove pointers
*/
// Removed unnecessary fields from fuse_file_info in fuse_common.h, no bitfields (not supported by ZPP)
struct fuse_file_info_lite {
	/** Open flags.	 Available in open() and release() */
	int flags;

	/** File handle id.  May be filled in by filesystem in create,
	 * open, and opendir().  Available in most other file operations on the
	 * same file handle. */
	uint64_t fh;
};

struct mknodParams {
    int seq;
    ballot r;
    std::string path;
    mode_t mode;
    dev_t rdev;
};

struct mkdirParams {
    int seq;
    ballot r;
    std::string path;
    mode_t mode;
};

struct symlinkParams {
    int seq;
    ballot r;
    std::string target;
    std::string linkpath;
};

struct unlinkParams {
    int seq;
    ballot r;
    std::string path;
};

struct rmdirParams {
    int seq;
    ballot r;
    std::string path;
};

struct renameParams {
    int seq;
    ballot r;
    std::string oldpath;
    std::string newpath;
};

struct linkParams {
    int seq;
    ballot r;
    std::string oldpath;
    std::string newpath;
};

// Use fi or path depending on whether hasFi is true
struct chmodParams {
    int seq;
    ballot r;
    std::string path;
    mode_t mode;
    fuse_file_info_lite fi;
    bool hasFi;
};

// Use fi or path depending on whether hasFi is true
struct chownParams {
    int seq;
    ballot r;
    std::string path;
    uid_t uid;
    gid_t gid;
    fuse_file_info_lite fi;
    bool hasFi;
};

struct truncateParams {
    int seq;
    ballot r;
    std::string path;
    off_t size;
    fuse_file_info_lite fi;
    bool hasFi;
};

struct utimensParams {
    int seq;
    ballot r;
    std::string path;
    timespec tv0; // split tv array into 2 fields because ZPP likes it better
    timespec tv1;
    fuse_file_info_lite fi;
    bool hasFi;
};

struct createParams {
    int seq;
    ballot r;
    std::string path;
    mode_t mode;
    fuse_file_info_lite fi;
};

struct openParams {
    int seq;
    ballot r;
    std::string path;
    fuse_file_info_lite fi;
};

struct writeBufParams {
    int seq;
    ballot r;
    off_t offset;
    std::vector<char> buf; // Use a vector so the receiver can resize its buffer based on how much is sent.
    fuse_file_info_lite fi;
};

struct releaseParams {
    int seq;
    ballot r;
    fuse_file_info_lite fi;
};

struct fsyncParams {
    int seq;
    ballot r;
    fuse_file_info_lite fi;
};

struct fallocateParams {
    int seq;
    ballot r;
    off_t offset;
    off_t length;
    fuse_file_info_lite fi;
};

struct setxattrParams {
    int seq;
    ballot r;
    std::string path;
    std::string name;
    std::vector<char> value;
    int flags;
};

struct removexattrParams {
    int seq;
    ballot r;
    std::string path;
    std::string name;
};

struct copyFileRangeParams {
    int seq;
    ballot r;
    fuse_file_info_lite fi_in;
    off_t off_in;
    fuse_file_info_lite fi_out;
    off_t off_out;
    size_t len;
    int flags;
};

struct p1a {
    ballot r;
    networkConfig netConf;
};

struct diskReq {
    int id;
    ballot r;
};

struct p2a {
    ballot r;
    int written;
    std::string diskReplicaName;
    std::string diskChecksum;
};

// Add to variant as the number of variants increase
typedef std::variant<mknodParams, 
                    mkdirParams, 
                    symlinkParams,
                    unlinkParams,
                    rmdirParams,
                    renameParams,
                    linkParams,
                    chmodParams,
                    chownParams,
                    truncateParams,
                    utimensParams,
                    createParams,
                    openParams,
                    writeBufParams,
                    releaseParams,
                    fsyncParams,
                    fallocateParams,
                    setxattrParams,
                    removexattrParams,
                    copyFileRangeParams,
                    p1a,
                    diskReq,
                    p2a> clientMsg;

struct p1b {
    int id;
    ballot clientBallot;
    int written;
    ballot normalBallot;
    ballot highestBallot;

    bool operator <(const p1b& rhs) const { 
        return id < rhs.id;
    }
};

struct disk {
    int id;
    ballot clientBallot;
    int written;
    std::string diskChecksum;

    bool operator <(const disk& rhs) const { return id < rhs.id; }
};

struct p2b {
    int id;
    ballot clientBallot;
    ballot highestBallot;

    bool operator<(const p2b& rhs) const { return id < rhs.id; }
};

struct fsyncMissing {
    int id;
    ballot r;
    int fsyncSeq;
    std::vector<int> holes;
};

struct fsyncAck {
    int id;
    int written;
    ballot r;
};

typedef std::variant<p1b,
                    disk,
                    p2b,
                    fsyncMissing,
                    fsyncAck> replicaMsg;

struct helloMsg {
    std::string text;
};