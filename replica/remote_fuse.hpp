#pragma once
#include <string>
#include <map>

typedef unsigned int mode_t;

// Copied from fuse_common.h in libfuse
struct fuse_file_info {
	/** Open flags.	 Available in open() and release() */
	int flags;

	/** In case of a write operation indicates if this was caused
	    by a delayed write from the page cache. If so, then the
	    context's pid, uid, and gid fields will not be valid, and
	    the *fh* value may not match the *fh* value that would
	    have been sent with the corresponding individual write
	    requests if write caching had been disabled. */
	unsigned int writepage : 1;

	/** Can be filled in by open/create, to use direct I/O on this file. */
	unsigned int direct_io : 1;

	/** Can be filled in by open and opendir. It signals the kernel that any
	    currently cached data (ie., data that the filesystem provided the
	    last time the file/directory was open) need not be invalidated when
	    the file/directory is closed. */
	unsigned int keep_cache : 1;

	/** Can be filled by open/create, to allow parallel direct writes on this
         *  file */
    unsigned int parallel_direct_writes : 1;

	/** Indicates a flush operation.  Set in flush operation, also
	    maybe set in highlevel lock operation and lowlevel release
	    operation. */
	unsigned int flush : 1;

	/** Can be filled in by open, to indicate that the file is not
	    seekable. */
	unsigned int nonseekable : 1;

	/* Indicates that flock locks for this file should be
	   released.  If set, lock_owner shall contain a valid value.
	   May only be set in ->release(). */
	unsigned int flock_release : 1;

	/** Can be filled in by opendir. It signals the kernel to
	    enable caching of entries returned by readdir().  Has no
	    effect when set in other contexts (in particular it does
	    nothing when set by open()). */
	unsigned int cache_readdir : 1;

	/** Can be filled in by open, to indicate that flush is not needed
	    on close. */
	unsigned int noflush : 1;

	/** Padding.  Reserved for future use*/
	unsigned int padding : 23;
	unsigned int padding2 : 32;

	/** File handle id.  May be filled in by filesystem in create,
	 * open, and opendir().  Available in most other file operations on the
	 * same file handle. */
	uint64_t fh;

	/** Lock owner id.  Available in locking operations and flush */
	uint64_t lock_owner;

	/** Requested poll events.  Available in ->poll.  Only set on kernels
	    which support it.  If unsupported, this field is set to zero. */
	uint32_t poll_events;
};

struct createParams {
    std::string path;
    mode_t mode;
    fuse_file_info fi;
};

class Fuse {
private:
    std::map<int, int> fileHandleConverter;

    void remote_create(const createParams& params);
};