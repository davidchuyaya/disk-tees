#include <string>
#include <unistd.h>
#include <dirent.h>
#include <sys/file.h>
#include "tee_fuse.hpp"

// If defined, will write files to the directory specified below. Used for testing locally.
#define CUSTOM_DIR
#ifdef CUSTOM_DIR
#define PREPEND_PATH(path) (("/home/davidchuyaya/hellotest" + std::string(path)).c_str())
#elif
#define PREPEND_PATH(path) (path)
#endif

void* TeeFuse::client_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    cfg->use_ino = 1;
	cfg->nullpath_ok = 1;
	cfg->entry_timeout = 0;
	cfg->attr_timeout = 0;
	cfg->negative_timeout = 0;
    return NULL;
}

int TeeFuse::client_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    int res;

	if(fi)
		res = fstat(fi->fh, stbuf);
	else
		res = lstat(PREPEND_PATH(path), stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

int TeeFuse::client_access(const char* path, int mask) {
    int res;

    res = access(PREPEND_PATH(path), mask);
    if (res == -1)
        return -errno;

    return 0;
}

struct xmp_dirp {
	DIR *dp;
	struct dirent *entry;
	off_t offset;
};

int TeeFuse::client_opendir(const char* path, struct fuse_file_info *fi) {
    int res;
	struct xmp_dirp *d = new xmp_dirp();
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

static inline struct xmp_dirp *get_dirp(struct fuse_file_info *fi) {
	return (struct xmp_dirp *) (uintptr_t) fi->fh;
}

int TeeFuse::client_releasedir(const char* path, struct fuse_file_info *fi) {
    struct xmp_dirp *d = get_dirp(fi);
	closedir(d->dp);
	delete d;
	return 0;
}

int TeeFuse::client_unlink(const char* path) {
    int res = unlink(PREPEND_PATH(path));
    if (res == -1)
        return -errno;

    return 0;
}

int TeeFuse::client_truncate(const char* path, off_t size, struct fuse_file_info *fi) {
    int res;

    if (fi)
        res = ftruncate(fi->fh, size);
    else
        res = truncate(PREPEND_PATH(path), size);
    if (res == -1)
        return -errno;

    return 0;
}

int TeeFuse::client_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
	int fd = open(PREPEND_PATH(path), fi->flags, mode);
	if (fd == -1)
		return -errno;

	fi->fh = fd;
	return 0;
}

int TeeFuse::client_write_buf(const char *path, struct fuse_bufvec *buf, off_t offset, struct fuse_file_info *fi) {
    struct fuse_bufvec dst = FUSE_BUFVEC_INIT(fuse_buf_size(buf));

	dst.buf[0].flags = (fuse_buf_flags) (FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK);
	dst.buf[0].fd = fi->fh;
	dst.buf[0].pos = offset;

	return fuse_buf_copy(&dst, buf, FUSE_BUF_SPLICE_NONBLOCK);
}

int TeeFuse::client_flush(const char *path, struct fuse_file_info *fi) {
    int res = close(dup(fi->fh));
    if (res == -1)
        return -errno;

    return 0;
}

int TeeFuse::client_release(const char *path, struct fuse_file_info *fi) {
    close(fi->fh);
    return 0;
}

int TeeFuse::client_fsync(const char *path, int isdatasync, struct fuse_file_info *fi) {
    int res;
    (void) path;
    if (isdatasync)
        res = fdatasync(fi->fh);
    else
        res = fsync(fi->fh);
    if (res == -1)
        return -errno;

    return 0;
}