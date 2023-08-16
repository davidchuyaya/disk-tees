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

struct truncateParams {
    off_t size;
    fuse_file_info_lite fi;
};

struct createParams {
    std::string path;
    mode_t mode;
    fuse_file_info_lite fi;
};

struct writeBufParams {
    off_t offset;
    std::vector<char> buf; // Use a vector so the receiver can resize its buffer based on how much is sent.
    fuse_file_info_lite fi;
};

struct releaseParams {
    fuse_file_info_lite fi;
};

struct fsyncParams {
    fuse_file_info_lite fi;
};

// Add to variant as the number of variants increase
typedef std::variant<truncateParams,
                    createParams, 
                    writeBufParams, 
                    releaseParams, 
                    fsyncParams> fuseMethodParams;

struct clientMsg {
    int seq;
    fuseMethodParams params;
};