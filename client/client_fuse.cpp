#include <string>
#include <chrono>
#include <unistd.h>
#include <dirent.h>
#include <sys/file.h>
#include <sys/xattr.h>
#include "client_fuse.hpp"

// If defined, will write files to the directory specified below. Used for testing locally.
#define CUSTOM_DIR
#ifdef CUSTOM_DIR
#define PREPEND_PATH(path) ((redirectPoint + std::string(path)).c_str())
#elif
#define PREPEND_PATH(path) (path)
#endif

// If defined, will send messages to the replicas.
#define NETWORK

ClientFuse::ClientFuse(const bool networkParam, const ballot ballotParam, const std::string& redirectPointParam, const int quorumParam,
 const addresses& configParam) {
	network = networkParam;
	r = ballotParam;
	redirectPoint = redirectPointParam;
	quorum = quorumParam;
	config = configParam;

	written = -1;
	replicasCommitted = 0;
}

void ClientFuse::addTLS(TLS<replicaMsg, clientMsg> *tls) {
	replicaTLS = tls;
}

void ClientFuse::operator()(const p1b& msg) {
	std::unique_lock<std::mutex> lock(replicaRecvMutex);
}

void ClientFuse::operator()(const disk& msg) {
	// TODO: Implement
}

void ClientFuse::operator()(const p2b& msg) {
	// TODO: Implement
}

void ClientFuse::operator()(const fsyncMissing& msg) {
	// TODO: Implement
}

void ClientFuse::operator()(const fsyncAck& msg) {
	// Obtain lock, since connections to replicas are multithreaded
	std::unique_lock<std::mutex> lock(replicaRecvMutex);

	replicaWritten[msg.id] = msg.written;
	// we know the seq has already been committed, so there's nothing to do
	if (replicasCommitted >= msg.written)
		return;

	int numWritten = 0;
	for (auto& [id, written] : replicaWritten) {
		if (written == msg.written)
			numWritten++;
	}

	if (numWritten < quorum)
		return;

	replicasCommitted = msg.written;
	uncommittedWrites.clear();
	// Wake up waiting fsyncs
	lock.unlock();
	fsyncCommitted.notify_all();
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

int ClientFuse::client_readlink(const char *path, char *buf, size_t size) {
	int res = readlink(PREPEND_PATH(path), buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
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

int ClientFuse::client_readdir(const char* path, void *buf, fuse_fill_dir_t filler, off_t offset, fuse_file_info *fi, enum fuse_readdir_flags flags) {
	xmp_dirp *d = get_dirp(fi);

	if (offset != d->offset) {
		seekdir(d->dp, offset);
		d->entry = NULL;
		d->offset = offset;
	}
	while (true) {
		struct stat st;
		off_t nextoff;
		enum fuse_fill_dir_flags fill_flags = (fuse_fill_dir_flags) 0;

		if (!d->entry) {
			d->entry = readdir(d->dp);
			if (!d->entry)
				break;
		}

		if (!(fill_flags & FUSE_FILL_DIR_PLUS)) {
			memset(&st, 0, sizeof(st));
			st.st_ino = d->entry->d_ino;
			st.st_mode = d->entry->d_type << 12;
		}
		nextoff = telldir(d->dp);

		if (filler(buf, d->entry->d_name, &st, nextoff, fill_flags))
			break;

		d->entry = NULL;
		d->offset = nextoff;
	}

	return 0;
}

int ClientFuse::client_releasedir(const char* path, fuse_file_info *fi) {
    xmp_dirp *d = get_dirp(fi);
	closedir(d->dp);
	delete d;
	return 0;
}

int ClientFuse::client_read_buf(const char *path, fuse_bufvec **bufp, size_t size, off_t offset, fuse_file_info *fi) {
	fuse_bufvec *src = new fuse_bufvec();
	*src = FUSE_BUFVEC_INIT(size);

	src->buf[0].flags = (fuse_buf_flags) (FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK);
	src->buf[0].fd = fi->fh;
	src->buf[0].pos = offset;

	*bufp = src;

	return 0;
}

int ClientFuse::client_statfs(const char *path, struct statvfs *stbuf) {
	int res = statvfs(PREPEND_PATH(path), stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

int ClientFuse::client_flush(const char *path, fuse_file_info *fi) {
    int res = close(dup(fi->fh));
    if (res == -1)
        return -errno;

    return 0;
}

int ClientFuse::client_getxattr(const char *path, const char *name, char *value, size_t size) {
	int res = lgetxattr(PREPEND_PATH(path), name, value, size);
	if (res == -1)
		return -errno;

	return res;
}

int ClientFuse::client_listxattr(const char *path, char *list, size_t size) {
	int res = llistxattr(PREPEND_PATH(path), list, size);
	if (res == -1)
		return -errno;

	return res;
}

int ClientFuse::client_flock(const char *path, fuse_file_info *fi, int op) {
	int res = flock(fi->fh, op);
	if (res == -1)
		return -errno;
	
	return 0;
}

int ClientFuse::client_lseek(const char *path, off_t off, int whence, fuse_file_info *fi) {
	int res = lseek(fi->fh, off, whence);
	if (res == -1)
		return -errno;

	return res;
}


/**
 * The following functions require communication with the replicas.
*/

int ClientFuse::client_mknod(const char *path, mode_t mode, dev_t rdev) {
	int res;
	
	if (S_ISFIFO(mode))
		res = mkfifo(PREPEND_PATH(path), mode);
	else
		res = mknod(PREPEND_PATH(path), mode, rdev);
	if (res == -1)
		return -errno;

	if (network) {
		std::unique_lock<std::mutex> lock(reconfigMutex);
		reconfigComplete.wait(lock, [&] { return !isReconfiguring; });

		written += 1;
		auto& msg = uncommittedWrites.emplace_back(mknodParams {
			.seq = written,
			.r = r,
			.path = std::string(path),
			.mode = mode,
			.rdev = rdev
		});
		replicaTLS->sendRoundRobin(msg, config);
	}

	return 0;
}

int ClientFuse::client_mkdir(const char *path, mode_t mode) {
	int res = mkdir(PREPEND_PATH(path), mode);
	if (res == -1)
		return -errno;

	if (network) {
		std::unique_lock<std::mutex> lock(reconfigMutex);
		reconfigComplete.wait(lock, [&] { return !isReconfiguring; });

		written += 1;
		auto& msg = uncommittedWrites.emplace_back(mkdirParams {
			.seq = written,
			.r = r,
			.path = std::string(path),
			.mode = mode
		});
		replicaTLS->sendRoundRobin(msg, config);
	}
	
	return 0;
}

int ClientFuse::client_symlink(const char *target, const char *linkpath) {
	/* symlink only requires "to" to be remapped, as seen here: https://github.com/libfuse/python-fuse/blob/master/example/xmp.py#L83 */
	int res = symlink(target, PREPEND_PATH(linkpath));
	if (res == -1)
		return -errno;
	
	if (network) {
		std::unique_lock<std::mutex> lock(reconfigMutex);
		reconfigComplete.wait(lock, [&] { return !isReconfiguring; });

		written += 1;
		auto& msg = uncommittedWrites.emplace_back(symlinkParams {
			.seq = written,
			.r = r,
			.target = std::string(target),
			.linkpath = std::string(linkpath)
		});
		replicaTLS->sendRoundRobin(msg, config);
	}

	return 0;
}

int ClientFuse::client_unlink(const char *path) {
    int res = unlink(PREPEND_PATH(path));
    if (res == -1)
        return -errno;

	if (network) {
		std::unique_lock<std::mutex> lock(reconfigMutex);
		reconfigComplete.wait(lock, [&] { return !isReconfiguring; });

		written += 1;
		auto& msg = uncommittedWrites.emplace_back(unlinkParams {
			.seq = written,
			.r = r,
			.path = std::string(path)
		});
		replicaTLS->sendRoundRobin(msg, config);
	}
	   
    return 0;
}

int ClientFuse::client_rmdir(const char *path) {
	int res = rmdir(PREPEND_PATH(path));
	if (res == -1)
		return -errno;

	if (network) {
		std::unique_lock<std::mutex> lock(reconfigMutex);
		reconfigComplete.wait(lock, [&] { return !isReconfiguring; });

		written += 1;
		auto& msg = uncommittedWrites.emplace_back(rmdirParams {
			.seq = written,
			.r = r,
			.path = std::string(path)
		});
		replicaTLS->sendRoundRobin(msg, config);
	}

	return 0;
}

int ClientFuse::client_rename(const char *oldpath, const char *newpath, unsigned int flags) {
	int res = rename(PREPEND_PATH(oldpath), PREPEND_PATH(newpath));
	if (res == -1)
		return -errno;

	if (network) {
		std::unique_lock<std::mutex> lock(reconfigMutex);
		reconfigComplete.wait(lock, [&] { return !isReconfiguring; });

		written += 1;
		auto& msg = uncommittedWrites.emplace_back(renameParams {
			.seq = written,
			.r = r,
			.oldpath = std::string(oldpath),
			.newpath = std::string(newpath)
		});
		replicaTLS->sendRoundRobin(msg, config);
	}

	return 0;
}

int ClientFuse::client_link(const char *oldpath, const char *newpath) {
	int res = link(PREPEND_PATH(oldpath), PREPEND_PATH(newpath));
	if (res == -1)
		return -errno;
	
	if (network) {
		std::unique_lock<std::mutex> lock(reconfigMutex);
		reconfigComplete.wait(lock, [&] { return !isReconfiguring; });

		written += 1;
		auto& msg = uncommittedWrites.emplace_back(linkParams {
			.seq = written,
			.r = r,
			.oldpath = std::string(oldpath),
			.newpath = std::string(newpath)
		});
		replicaTLS->sendRoundRobin(msg, config);
	}

	return 0;
}

int ClientFuse::client_chmod(const char *path, mode_t mode, fuse_file_info *fi) {
	int res;
	bool hasFi = fi != NULL;
	if (hasFi)
		res = fchmod(fi->fh, mode);
	else
		res = chmod(PREPEND_PATH(path), mode);
	if (res == -1)
		return -errno;

	if (network) {
		std::unique_lock<std::mutex> lock(reconfigMutex);
		reconfigComplete.wait(lock, [&] { return !isReconfiguring; });

		written += 1;
		auto& msg = uncommittedWrites.emplace_back(chmodParams {
			.seq = written,
			.r = r,
			.path = hasFi ? "" : std::string(path),
			.mode = mode,
			.fi = hasFi ? make_lite(fi) : fuse_file_info_lite(),
			.hasFi = hasFi
		});
		replicaTLS->sendRoundRobin(msg, config);
	}

	return 0;
}

int ClientFuse::client_chown(const char *path, uid_t uid, gid_t gid, fuse_file_info *fi) {
	int res;
	bool hasFi = fi != NULL;
	if (hasFi)
		res = fchown(fi->fh, uid, gid);
	else
		res = lchown(PREPEND_PATH(path), uid, gid);
	if (res == -1)
		return -errno;

	if (network) {
		std::unique_lock<std::mutex> lock(reconfigMutex);
		reconfigComplete.wait(lock, [&] { return !isReconfiguring; });

		written += 1;
		auto& msg = uncommittedWrites.emplace_back(chownParams {
			.seq = written,
			.r = r,
			.path = hasFi ? "" : std::string(path),
			.uid = uid,
			.gid = gid,
			.fi = hasFi ? make_lite(fi) : fuse_file_info_lite(),
			.hasFi = hasFi
		});
		replicaTLS->sendRoundRobin(msg, config);
	}

	return 0;
}

int ClientFuse::client_truncate(const char* path, off_t size, fuse_file_info *fi) {
  	int res;
	bool hasFi = fi != NULL;
	if (hasFi)
		res = ftruncate(fi->fh, size);
	else
		res = truncate(PREPEND_PATH(path), size);
    if (res == -1)
        return -errno;

	if (network) {
		std::unique_lock<std::mutex> lock(reconfigMutex);
		reconfigComplete.wait(lock, [&] { return !isReconfiguring; });

		written += 1;
		auto& msg = uncommittedWrites.emplace_back(truncateParams {
			.seq = written,
			.r = r,
			.path = hasFi ? "" : std::string(path),
			.size = size,
			.fi = hasFi ? make_lite(fi) : fuse_file_info_lite(),
			.hasFi = hasFi
		});
		replicaTLS->sendRoundRobin(msg, config);
	}

    return 0;
}

int ClientFuse::client_utimens(const char *path, const timespec tv[2], fuse_file_info *fi) {
	int res;
	bool hasFi = fi != NULL;
	if (hasFi)
		res = futimens(fi->fh, tv);
	else
		res = utimensat(0, PREPEND_PATH(path), tv, AT_SYMLINK_NOFOLLOW);
	if (res == -1)
		return -errno;

	if (network) {
		std::unique_lock<std::mutex> lock(reconfigMutex);
		reconfigComplete.wait(lock, [&] { return !isReconfiguring; });

		written += 1;
		auto& msg = uncommittedWrites.emplace_back(utimensParams {
			.seq = written,
			.r = r,
			.path = hasFi ? "" : std::string(path),
			.tv0 = tv[0],
			.tv1 = tv[1],
			.fi = hasFi ? make_lite(fi) : fuse_file_info_lite(),
			.hasFi = hasFi
		});
		replicaTLS->sendRoundRobin(msg, config);
	}

	return 0;
}

int ClientFuse::client_create(const char *path, mode_t mode, fuse_file_info *fi) {
	int fd = open(PREPEND_PATH(path), fi->flags, mode);
	if (fd == -1)
		return -errno;

	fi->fh = fd;
	// std::cout << "Fh " << fd << " at path " << path << std::endl;

	if (network) {
		std::unique_lock<std::mutex> lock(reconfigMutex);
		reconfigComplete.wait(lock, [&] { return !isReconfiguring; });

		written += 1;
		auto& msg = uncommittedWrites.emplace_back(createParams {
			.seq = written,
			.r = r,
			.path = std::string(path),
			.mode = mode,
			.fi = make_lite(fi)
		});
		replicaTLS->sendRoundRobin(msg, config);
	}

	return 0;
}


int ClientFuse::client_open(const char *path, fuse_file_info *fi) {
	int fd = open(PREPEND_PATH(path), fi->flags);
	if (fd == -1)
		return -errno;

	fi->fh = fd;
	// std::cout << "Fh " << fd << " at path " << path << std::endl;

	if (network) {
		std::unique_lock<std::mutex> lock(reconfigMutex);
		reconfigComplete.wait(lock, [&] { return !isReconfiguring; });

		written += 1;
		auto& msg = uncommittedWrites.emplace_back(openParams {
			.seq = written,
			.r = r,
			.path = std::string(path),
			.fi = make_lite(fi)
		});
		replicaTLS->sendRoundRobin(msg, config);
	}

	return 0;
}

int ClientFuse::client_write_buf(const char *path, fuse_bufvec *buf, off_t offset, fuse_file_info *fi) {
	size_t bufSize = fuse_buf_size(buf);
    fuse_bufvec dst = FUSE_BUFVEC_INIT(bufSize);

	dst.buf[0].flags = (fuse_buf_flags) (FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK);
	dst.buf[0].fd = fi->fh;
	dst.buf[0].pos = offset;

	if (network) {
		std::unique_lock<std::mutex> lock(reconfigMutex);
		reconfigComplete.wait(lock, [&] { return !isReconfiguring; });

		// 1. Copy memory from buffer into mem
		char* bufStart = (char*) buf->buf[buf->idx].mem + buf->off;
		// TODO: This is probably the critical point of the code, and we're doing 2 copies (1 into the vector, 1 after serializing).
		// How do we do this with zero copy? sendFile?
		std::vector<char> mem;
		mem.insert(mem.end(), bufStart, bufStart + bufSize - buf->off);
		// 2. Send mem
		written += 1;
		auto& msg = uncommittedWrites.emplace_back(writeBufParams {
			.seq = written,
			.r = r,
			.offset = offset,
			.buf = mem,
			.fi = make_lite(fi)
		});
		replicaTLS->sendRoundRobin(msg, config);
	}

	return fuse_buf_copy(&dst, buf, FUSE_BUF_SPLICE_NONBLOCK);
}

int ClientFuse::client_release(const char *path, fuse_file_info *fi) {
    close(fi->fh);
	// std::cout << "Fh " << fi->fh << " released at path " << path << std::endl;

	if (network) {
		std::unique_lock<std::mutex> lock(reconfigMutex);
		reconfigComplete.wait(lock, [&] { return !isReconfiguring; });

		written += 1;
		auto& msg = uncommittedWrites.emplace_back(releaseParams {
			.seq = written,
			.r = r,
			.fi = make_lite(fi)
		});
		replicaTLS->sendRoundRobin(msg, config);
	}

    return 0;
}

int ClientFuse::client_fsync(const char *path, int isdatasync, fuse_file_info *fi) {
    int res;
	
    if (isdatasync)
        res = fdatasync(fi->fh);
    else
        res = fsync(fi->fh);
    if (res == -1)
        return -errno;

	if (network) {
		{
			std::unique_lock<std::mutex> lock(reconfigMutex);
			reconfigComplete.wait(lock, [&] { return !isReconfiguring; });
		}

		replicaTLS->broadcast(fsyncParams {
			.seq = written, // Note: Fsync does not increase the written count
			.r = r,
			.fi = make_lite(fi)
		}, config);

		// block until fsync is acknowledged by quorum
		std::unique_lock<std::mutex> lock(replicaRecvMutex);
		fsyncCommitted.wait(lock, [&] { return replicasCommitted >= written; });
	}

    return 0;
}

int ClientFuse::client_fallocate(const char *path, int mode, off_t offset, off_t length, fuse_file_info *fi) {
	int res = posix_fallocate(fi->fh, offset, length);

	if (network) {
		std::unique_lock<std::mutex> lock(reconfigMutex);
		reconfigComplete.wait(lock, [&] { return !isReconfiguring; });

		written += 1;
		auto& msg = uncommittedWrites.emplace_back(fallocateParams {
			.seq = written,
			.r = r,
			.offset = offset,
			.length = length,
			.fi = make_lite(fi)
		});
		replicaTLS->sendRoundRobin(msg, config);
	}

	return -res;
}

int ClientFuse::client_setxattr(const char *path, const char *name, const char *value, size_t size, int flags) {
	int res = lsetxattr(PREPEND_PATH(path), name, value, size, flags);
	if (res == -1)
		return -errno;

	if (network) {
		std::unique_lock<std::mutex> lock(reconfigMutex);
		reconfigComplete.wait(lock, [&] { return !isReconfiguring; });

		// Value is not necessarily a string. See https://linux.die.net/man/2/setxattr.
		std::vector<char> valueVec;
		valueVec.insert(valueVec.end(), value, value + size);
		written += 1;
		auto& msg = uncommittedWrites.emplace_back(setxattrParams {
			.seq = written,
			.r = r,
			.path = std::string(path),
			.name = std::string(name),
			.value = valueVec,
			.flags = flags
		});
		replicaTLS->sendRoundRobin(msg, config);
	}

	return 0;
}

int ClientFuse::client_removexattr(const char *path, const char *name) {
	int res = lremovexattr(PREPEND_PATH(path), name);
	if (res == -1)
		return -errno;

	if (network) {
		std::unique_lock<std::mutex> lock(reconfigMutex);
		reconfigComplete.wait(lock, [&] { return !isReconfiguring; });

	written += 1;
		auto& msg = uncommittedWrites.emplace_back(removexattrParams {
			.seq = written,
			.r = r,
			.path = std::string(path),
			.name = std::string(name)
		});
		replicaTLS->sendRoundRobin(msg, config);
	}
	
	return 0;
}

int ClientFuse::client_copy_file_range(const char *path_in, fuse_file_info *fi_in, off_t off_in, const char *path_out, fuse_file_info *fi_out, off_t off_out, size_t len, int flags) {
	int res = copy_file_range(fi_in->fh, &off_in, fi_out->fh, &off_out, len, flags);
	if (res == -1)
		return -errno;
	
	if (network) {
		std::unique_lock<std::mutex> lock(reconfigMutex);
		reconfigComplete.wait(lock, [&] { return !isReconfiguring; });

		written += 1;
		auto& msg = uncommittedWrites.emplace_back(copyFileRangeParams {
			.seq = written,
			.r = r,
			.fi_in = make_lite(fi_in),
			.off_in = off_in,
			.fi_out = make_lite(fi_out),
			.off_out = off_out,
			.len = len,
			.flags = flags
		});
		replicaTLS->sendRoundRobin(msg, config);
	}

	return 0;
}

fuse_file_info_lite ClientFuse::make_lite(fuse_file_info *fi) {
	return fuse_file_info_lite {
		.flags = fi->flags,
		.fh = fi->fh	
	};
}