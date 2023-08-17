#include <string>
#include <unistd.h>
#include <dirent.h>
#include <sys/file.h>
#include "client_fuse.hpp"

// If defined, will write files to the directory specified below. Used for testing locally.
#define CUSTOM_DIR
#ifdef CUSTOM_DIR
#define PREPEND_PATH(path) (("/home/davidchuyaya/hellotest" + std::string(path)).c_str())
#elif
#define PREPEND_PATH(path) (path)
#endif

ClientFuse::ClientFuse(TLS<diskTeePayload, clientMsg> *replicaTLSParam) {
	replicaTLS = replicaTLSParam;
	written = 0;
}

void* ClientFuse::client_init(fuse_conn_info *conn, fuse_config *cfg) {
    cfg->use_ino = 1;
	cfg->nullpath_ok = 1;
	cfg->entry_timeout = 0;
	cfg->attr_timeout = 0;
	cfg->negative_timeout = 0;
    return NULL;
}

int ClientFuse::client_getattr(const char *path, struct stat *stbuf, fuse_file_info *fi) {
    int res;

	if(fi)
		res = fstat(fi->fh, stbuf);
	else
		res = lstat(PREPEND_PATH(path), stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

int ClientFuse::client_access(const char* path, int mask) {
    int res;

    res = access(PREPEND_PATH(path), mask);
    if (res == -1)
        return -errno;

    return 0;
}

struct xmp_dirp {
	DIR *dp;
	dirent *entry;
	off_t offset;
};

int ClientFuse::client_opendir(const char* path, fuse_file_info *fi) {
    int res;
	xmp_dirp *d = new xmp_dirp();
	if (d == NULL)
		return -ENOMEM;

	d->dp = opendir(PREPEND_PATH(path));
	if (d->dp == NULL) {
		res = -errno;
		free(d);
		return res;
	}
	d->offset = 0;
	d->entry = NULL;

	fi->fh = (unsigned long) d;
	return 0;
}

static inline xmp_dirp *get_dirp(fuse_file_info *fi) {
	return (xmp_dirp *) (uintptr_t) fi->fh;
}

int ClientFuse::client_releasedir(const char* path, fuse_file_info *fi) {
    xmp_dirp *d = get_dirp(fi);
	closedir(d->dp);
	delete d;
	return 0;
}

int ClientFuse::client_unlink(const char* path) {
    int res = unlink(PREPEND_PATH(path));
    if (res == -1)
        return -errno;

	// Start networking
	unlinkParams msg {
		.seq = written++,
		.r = r,
		.path = std::string(path)
	};
	replicaTLS->broadcast(msg);
	// Finish networking
    return 0;
}

int ClientFuse::client_truncate(const char* path, off_t size, fuse_file_info *fi) {
    if (fi == NULL) {
		std::cerr << "Truncate called on path " << path << " without file info" << std::endl;
		exit(EXIT_FAILURE);
	}

  	int res = ftruncate(fi->fh, size);      
    if (res == -1)
        return -errno;

	// Start networking
	truncateParams msg {
		.seq = written++,
		.r = r,
		.size = size,
		.fi = make_lite(fi)
	};
	replicaTLS->broadcast(msg);
	// Finish networking

    return 0;
}

int ClientFuse::client_create(const char *path, mode_t mode, fuse_file_info *fi) {
	int fd = open(PREPEND_PATH(path), fi->flags, mode);
	if (fd == -1)
		return -errno;

	fi->fh = fd;

	// Start networking
	createParams msg {
		.seq = written++,
		.r = r,
		.path = std::string(path),
		.mode = mode,
		.fi = make_lite(fi)
	};
	replicaTLS->broadcast(msg);
	// Finish networking

	return 0;
}

int ClientFuse::client_write_buf(const char *path, fuse_bufvec *buf, off_t offset, fuse_file_info *fi) {
	size_t bufSize = fuse_buf_size(buf);
    fuse_bufvec dst = FUSE_BUFVEC_INIT(bufSize);

	dst.buf[0].flags = (fuse_buf_flags) (FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK);
	dst.buf[0].fd = fi->fh;
	dst.buf[0].pos = offset;

	// Start networking
	// 1. Copy memory from buffer into mem
	char* bufStart = (char*) buf->buf[buf->idx].mem + buf->off;
	// TODO: This is probably the critical point of the code, and we're doing 2 copies (1 into the vector, 1 after serializing).
	// How do we do this with zero copy? sendFile?
	std::vector<char> mem;
	mem.insert(mem.end(), bufStart, bufStart + bufSize - buf->off);
	// 2. Send mem
	writeBufParams msg {
		.seq = written++,
		.r = r,
		.offset = offset,
		.buf = mem,
		.fi = make_lite(fi)
	};
	replicaTLS->broadcast(msg);
	// Finish networking

	return fuse_buf_copy(&dst, buf, FUSE_BUF_SPLICE_NONBLOCK);
}

int ClientFuse::client_flush(const char *path, fuse_file_info *fi) {
    int res = close(dup(fi->fh));
    if (res == -1)
        return -errno;

    return 0;
}

int ClientFuse::client_release(const char *path, fuse_file_info *fi) {
    close(fi->fh);

	// Start networking
	releaseParams msg {
		.seq = written++,
		.r = r,
		.fi = make_lite(fi)
	};
	replicaTLS->broadcast(msg);
	// End networking

    return 0;
}

int ClientFuse::client_fsync(const char *path, int isdatasync, fuse_file_info *fi) {
    int res;
	
    if (isdatasync)
        res = fdatasync(fi->fh); //TODO: See if there are any fdatasyncs and send the int to replica_fuse.cpp if so
    else
        res = fsync(fi->fh);
    if (res == -1)
        return -errno;

	// Start networking
	fsyncParams msg {
		.seq = written, // Note: Fsync does not increase the written count
		.r = r,
		.fi = make_lite(fi)
	};
	replicaTLS->broadcast(msg);
	// End networking

    return 0;
}

fuse_file_info_lite ClientFuse::make_lite(fuse_file_info *fi) {
	return fuse_file_info_lite {
		.flags = fi->flags,
		.fh = fi->fh	
	};
}