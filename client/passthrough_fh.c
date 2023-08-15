/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2011       Sebastian Pipping <sebastian@pipping.org>

  This program can be distributed under the terms of the GNU GPLv2.
  See the file COPYING.
*/

/** @file
 *
 * This file system mirrors the existing file system hierarchy of the
 * system, starting at the root file system. This is implemented by
 * just "passing through" all requests to the corresponding user-space
 * libc functions. This implementation is a little more sophisticated
 * than the one in passthrough.c, so performance is not quite as bad.
 *
 * Compile with:
 *
 *     gcc -Wall passthrough_fh.c `pkg-config fuse3 --cflags --libs` -lulockmgr -o passthrough_fh
 *
 * ## Source code ##
 * \include passthrough_fh.c
 */

#define FUSE_USE_VERSION 31

#define _GNU_SOURCE

#include <fuse.h>

#ifdef HAVE_LIBULOCKMGR
#include <ulockmgr.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include <sys/file.h> /* flock(2) */
#include <signal.h>

/* Operations that would've been passed through to Linux root instead occur in the specified directory. */
const char* mount_point = "/home/davidchuyaya/hellotest/";
const size_t mount_point_len = 29; // Remember to keep in sync with mount_point.

// # define TRACK_WRITE_COUNT
int write_count = 0;

#define TRACK_OP_COUNT
#ifdef TRACK_OP_COUNT
typedef struct {
	int init;
	int getattr;
	int access;
	int readlink;
	int opendir;
	int readdir;
	int releasedir;
	int mknod;
	int mkdir;
	int symlink;
	int unlink;
	int rmdir;
	int rename;
	int link;
	int chmod;
	int chown;
	int truncate;
	int utimens;
	int create;
	int open;
	int read;
	int read_buf;
	int write;
	int write_buf;
	int statfs;
	int flush;
	int release;
	int fsync;
	int fallocate;
	int setxattr;
	int getxattr;
	int listxattr;
	int removexattr;
	int lock;
	int flock;
	int copy_file_range;
	int lseek;
} operationCount;
static operationCount allCount;
#endif

/* Deletes the first character of "path" (assumed to be "/") and prepends mount_point. */
static char* prepend_path(const char* path)
{
	size_t path_len = strlen(path);
	// if (path_len <= 0)
	// 	return mount_point;
	char* new_path = malloc(mount_point_len + path_len);
	strcpy(new_path, mount_point);
	strcat(new_path, path+1);
	// printf("New path: %s\n", new_path);
	return new_path;
}

static void *xmp_init(struct fuse_conn_info *conn,
		      struct fuse_config *cfg)
{
	#ifdef TRACK_OP_COUNT
	allCount.init++;
	#endif

	(void) conn;
	cfg->use_ino = 1;
	cfg->nullpath_ok = 1;

	/* Pick up changes from lower filesystem right away. This is
	   also necessary for better hardlink support. When the kernel
	   calls the unlink() handler, it does not know the inode of
	   the to-be-removed entry and can therefore not invalidate
	   the cache of the associated inode - resulting in an
	   incorrect st_nlink value being reported for any remaining
	   hardlinks to this inode. */
	cfg->entry_timeout = 0;
	cfg->attr_timeout = 0;
	cfg->negative_timeout = 0;

	return NULL;
}

static int xmp_getattr(const char *path, struct stat *stbuf,
			struct fuse_file_info *fi)
{
	#ifdef TRACK_OP_COUNT
	allCount.getattr++;
	#endif

	int res;

	(void) path;

	if(fi)
		res = fstat(fi->fh, stbuf);
	else {
		char* new_path = prepend_path(path);
		res = lstat(new_path, stbuf);
		free(new_path);
	}
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_access(const char *path, int mask)
{
	#ifdef TRACK_OP_COUNT
	allCount.access++;
	#endif

	int res;

	char* new_path = prepend_path(path);
	res = access(new_path, mask);
	free(new_path);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	#ifdef TRACK_OP_COUNT
	allCount.readlink++;
	#endif

	int res;

	char* new_path = prepend_path(path);
	res = readlink(new_path, buf, size - 1);
	free(new_path);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}

struct xmp_dirp {
	DIR *dp;
	struct dirent *entry;
	off_t offset;
};

static int xmp_opendir(const char *path, struct fuse_file_info *fi)
{
	#ifdef TRACK_OP_COUNT
	allCount.opendir++;
	#endif

	int res;
	struct xmp_dirp *d = malloc(sizeof(struct xmp_dirp));
	if (d == NULL)
		return -ENOMEM;

	char* new_path = prepend_path(path);
	d->dp = opendir(new_path);
	free(new_path);
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

static inline struct xmp_dirp *get_dirp(struct fuse_file_info *fi)
{
	return (struct xmp_dirp *) (uintptr_t) fi->fh;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi,
		       enum fuse_readdir_flags flags)
{
	#ifdef TRACK_OP_COUNT
	allCount.readdir++;
	#endif

	struct xmp_dirp *d = get_dirp(fi);

	(void) path;
	if (offset != d->offset) {
#ifndef __FreeBSD__
		seekdir(d->dp, offset);
#else
		/* Subtract the one that we add when calling
		   telldir() below */
		seekdir(d->dp, offset-1);
#endif
		d->entry = NULL;
		d->offset = offset;
	}
	while (1) {
		struct stat st;
		off_t nextoff;
		enum fuse_fill_dir_flags fill_flags = 0;

		if (!d->entry) {
			d->entry = readdir(d->dp);
			if (!d->entry)
				break;
		}
#ifdef HAVE_FSTATAT
		if (flags & FUSE_READDIR_PLUS) {
			int res;

			res = fstatat(dirfd(d->dp), d->entry->d_name, &st,
				      AT_SYMLINK_NOFOLLOW);
			if (res != -1)
				fill_flags |= FUSE_FILL_DIR_PLUS;
		}
#endif
		if (!(fill_flags & FUSE_FILL_DIR_PLUS)) {
			memset(&st, 0, sizeof(st));
			st.st_ino = d->entry->d_ino;
			st.st_mode = d->entry->d_type << 12;
		}
		nextoff = telldir(d->dp);
#ifdef __FreeBSD__		
		/* Under FreeBSD, telldir() may return 0 the first time
		   it is called. But for libfuse, an offset of zero
		   means that offsets are not supported, so we shift
		   everything by one. */
		nextoff++;
#endif
		if (filler(buf, d->entry->d_name, &st, nextoff, fill_flags))
			break;

		d->entry = NULL;
		d->offset = nextoff;
	}

	return 0;
}

static int xmp_releasedir(const char *path, struct fuse_file_info *fi)
{
	#ifdef TRACK_OP_COUNT
	allCount.releasedir++;
	#endif

	struct xmp_dirp *d = get_dirp(fi);
	(void) path;
	closedir(d->dp);
	free(d);
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	#ifdef TRACK_OP_COUNT
	allCount.mknod++;
	#endif

	int res;

	char* new_path = prepend_path(path);
	if (S_ISFIFO(mode))
		res = mkfifo(new_path, mode);
	else
		res = mknod(new_path, mode, rdev);
	free(new_path);
	if (res == -1)
		return -errno;

	#ifdef TRACK_WRITE_COUNT
	write_count++;
	printf("Write count mknod: %d\n", write_count);
	#endif

	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	#ifdef TRACK_OP_COUNT
	allCount.mkdir++;
	#endif

	int res;

	char* new_path = prepend_path(path);
	res = mkdir(new_path, mode);
	free(new_path);
	if (res == -1)
		return -errno;
	
	#ifdef TRACK_WRITE_COUNT
	write_count++;
	printf("Write count mkdir: %d\n", write_count);
	#endif

	return 0;
}

static int xmp_unlink(const char *path)
{
	#ifdef TRACK_OP_COUNT
	allCount.unlink++;
	#endif

	int res;

	char* new_path = prepend_path(path);
	res = unlink(new_path);
	free(new_path);
	if (res == -1)
		return -errno;

	#ifdef TRACK_WRITE_COUNT
	write_count++;
	printf("Write count unlink: %d\n", write_count);
	#endif

	return 0;
}

static int xmp_rmdir(const char *path)
{
	#ifdef TRACK_OP_COUNT
	allCount.rmdir++;
	#endif

	int res;

	char* new_path = prepend_path(path);
	res = rmdir(new_path);
	free(new_path);
	if (res == -1)
		return -errno;

	#ifdef TRACK_WRITE_COUNT
	write_count++;
	printf("Write count rmdir: %d\n", write_count);
	#endif

	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	#ifdef TRACK_OP_COUNT
	allCount.symlink++;
	#endif

	int res;

	/* symlink only requires "to" to be remapped, as seen here: https://github.com/libfuse/python-fuse/blob/master/example/xmp.py#L83 */
	char* new_to = prepend_path(to);
	res = symlink(from, new_to);
	free(new_to);
	if (res == -1)
		return -errno;

	#ifdef TRACK_WRITE_COUNT
	write_count++;
	printf("Write count symlink: %d\n", write_count);
	#endif

	return 0;
}

static int xmp_rename(const char *from, const char *to, unsigned int flags)
{
	#ifdef TRACK_OP_COUNT
	allCount.rename++;
	#endif

	int res;

	/* When we have renameat2() in libc, then we can implement flags */
	if (flags)
		return -EINVAL;

	char* new_from = prepend_path(from);
	char* new_to = prepend_path(to);
	res = rename(new_from, new_to);
	free(new_from);
	free(new_to);
	if (res == -1)
		return -errno;

	#ifdef TRACK_WRITE_COUNT
	write_count++;
	printf("Write count rename: %d\n", write_count);
	#endif

	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	#ifdef TRACK_OP_COUNT
	allCount.link++;
	#endif

	int res;

	char* new_from = prepend_path(from);
	char* new_to = prepend_path(to);
	res = link(new_from, new_to);
	free(new_from);
	free(new_to);
	if (res == -1)
		return -errno;

	#ifdef TRACK_WRITE_COUNT
	write_count++;
	printf("Write count link: %d\n", write_count);
	#endif

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode,
		     struct fuse_file_info *fi)
{
	#ifdef TRACK_OP_COUNT
	allCount.chmod++;
	#endif

	int res;

	if(fi)
		res = fchmod(fi->fh, mode);
	else {
		char* new_path = prepend_path(path);
		res = chmod(new_path, mode);
		free(new_path);
	}
	if (res == -1)
		return -errno;

	#ifdef TRACK_WRITE_COUNT
	write_count++;
	printf("Write count chmod: %d\n", write_count);
	#endif

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid,
		     struct fuse_file_info *fi)
{
	#ifdef TRACK_OP_COUNT
	allCount.chown++;
	#endif

	int res;

	if (fi)
		res = fchown(fi->fh, uid, gid);
	else {
		char* new_path = prepend_path(path);
		res = lchown(new_path, uid, gid);
		free(new_path);
	}
	if (res == -1)
		return -errno;

	#ifdef TRACK_WRITE_COUNT
	write_count++;
	printf("Write count chown: %d\n", write_count);
	#endif

	return 0;
}

static int xmp_truncate(const char *path, off_t size,
			 struct fuse_file_info *fi)
{
	#ifdef TRACK_OP_COUNT
	allCount.truncate++;
	#endif

	int res;

	if(fi)
		res = ftruncate(fi->fh, size);
	else {
		char* new_path = prepend_path(path);
		res = truncate(new_path, size);
		free(new_path);
	}

	if (res == -1)
		return -errno;

	#ifdef TRACK_WRITE_COUNT
	write_count++;
	printf("Write count truncate: %d\n", write_count);
	#endif

	return 0;
}

#ifdef HAVE_UTIMENSAT
static int xmp_utimens(const char *path, const struct timespec ts[2],
		       struct fuse_file_info *fi)
{
	#ifdef TRACK_OP_COUNT
	allCount.utimens++;
	#endif

	int res;

	/* don't use utime/utimes since they follow symlinks */
	if (fi)
		res = futimens(fi->fh, ts);
	else {
		char* new_path = prepend_path(path);
		res = utimensat(0, new_path, ts, AT_SYMLINK_NOFOLLOW);
		free(new_path);
	}
	if (res == -1)
		return -errno;

	#ifdef TRACK_WRITE_COUNT
	write_count++;
	printf("Write count utimens: %d\n", write_count);
	#endif

	return 0;
}
#endif

static int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	#ifdef TRACK_OP_COUNT
	allCount.create++;
	#endif

	int fd;

	char* new_path = prepend_path(path);
	fd = open(new_path, fi->flags, mode);
	free(new_path);
	if (fd == -1)
		return -errno;

	#ifdef TRACK_WRITE_COUNT
	write_count++;
	printf("Write count create: %d\n", write_count);
	#endif

	fi->fh = fd;
	return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	#ifdef TRACK_OP_COUNT
	allCount.open++;
	#endif

	int fd;

	char* new_path = prepend_path(path);
	fd = open(new_path, fi->flags);
	free(new_path);
	if (fd == -1)
		return -errno;

	fi->fh = fd;
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	#ifdef TRACK_OP_COUNT
	allCount.read++;
	#endif

	int res;

	(void) path;
	res = pread(fi->fh, buf, size, offset);
	if (res == -1)
		res = -errno;

	return res;
}

static int xmp_read_buf(const char *path, struct fuse_bufvec **bufp,
			size_t size, off_t offset, struct fuse_file_info *fi)
{
	#ifdef TRACK_OP_COUNT
	allCount.read_buf++;
	#endif

	struct fuse_bufvec *src;

	(void) path;

	src = malloc(sizeof(struct fuse_bufvec));
	if (src == NULL)
		return -ENOMEM;

	*src = FUSE_BUFVEC_INIT(size);

	src->buf[0].flags = FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK;
	src->buf[0].fd = fi->fh;
	src->buf[0].pos = offset;

	*bufp = src;

	return 0;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	#ifdef TRACK_OP_COUNT
	allCount.write++;
	#endif

	int res;

	(void) path;
	res = pwrite(fi->fh, buf, size, offset);
	if (res == -1)
		res = -errno;

	#ifdef TRACK_WRITE_COUNT
	write_count++;
	printf("Write count write: %d\n", write_count);
	#endif

	return res;
}

static int xmp_write_buf(const char *path, struct fuse_bufvec *buf,
		     off_t offset, struct fuse_file_info *fi)
{
	#ifdef TRACK_OP_COUNT
	allCount.write_buf++;
	#endif

	struct fuse_bufvec dst = FUSE_BUFVEC_INIT(fuse_buf_size(buf));

	(void) path;

	dst.buf[0].flags = FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK;
	dst.buf[0].fd = fi->fh;
	dst.buf[0].pos = offset;

	#ifdef TRACK_WRITE_COUNT
	write_count++;
	printf("Write count write_buf: %d\n", write_count);
	#endif

	return fuse_buf_copy(&dst, buf, FUSE_BUF_SPLICE_NONBLOCK);
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	#ifdef TRACK_OP_COUNT
	allCount.statfs++;
	#endif

	int res;

	char* new_path = prepend_path(path);
	res = statvfs(path, stbuf);
	free(new_path);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_flush(const char *path, struct fuse_file_info *fi)
{
	#ifdef TRACK_OP_COUNT
	allCount.flush++;
	#endif

	int res;

	(void) path;
	/* This is called from every close on an open file, so call the
	   close on the underlying filesystem.	But since flush may be
	   called multiple times for an open file, this must not really
	   close the file.  This is important if used on a network
	   filesystem like NFS which flush the data/metadata on close() */
	res = close(dup(fi->fh));
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	#ifdef TRACK_OP_COUNT
	allCount.release++;
	#endif

	(void) path;
	close(fi->fh);

	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	#ifdef TRACK_OP_COUNT
	allCount.fsync++;
	#endif

	int res;
	(void) path;

#ifndef HAVE_FDATASYNC
	(void) isdatasync;
#else
	if (isdatasync)
		res = fdatasync(fi->fh);
	else
#endif
		res = fsync(fi->fh);
	if (res == -1)
		return -errno;

	return 0;
}

#ifdef HAVE_POSIX_FALLOCATE
static int xmp_fallocate(const char *path, int mode,
			off_t offset, off_t length, struct fuse_file_info *fi)
{
	#ifdef TRACK_OP_COUNT
	allCount.fallocate++;
	#endif

	(void) path;

	if (mode)
		return -EOPNOTSUPP;

	#ifdef TRACK_WRITE_COUNT
	write_count++;
	printf("Write count fallocate: %d\n", write_count);
	#endif

	return -posix_fallocate(fi->fh, offset, length);
}
#endif

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	#ifdef TRACK_OP_COUNT
	allCount.setxattr++;
	#endif

	char* new_path = prepend_path(path);
	int res = lsetxattr(new_path, name, value, size, flags);
	free(new_path);
	if (res == -1)
		return -errno;

	#ifdef TRACK_WRITE_COUNT
	write_count++;
	printf("Write count setxattr: %d\n", write_count);
	#endif

	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	#ifdef TRACK_OP_COUNT
	allCount.getxattr++;
	#endif

	char* new_path = prepend_path(path);
	int res = lgetxattr(new_path, name, value, size);
	free(new_path);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	#ifdef TRACK_OP_COUNT
	allCount.listxattr++;
	#endif

	char* new_path = prepend_path(path);
	int res = llistxattr(new_path, list, size);
	free(new_path);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	#ifdef TRACK_OP_COUNT
	allCount.removexattr++;
	#endif

	char* new_path = prepend_path(path);
	int res = lremovexattr(new_path, name);
	free(new_path);
	if (res == -1)
		return -errno;

	#ifdef TRACK_WRITE_COUNT
	write_count++;
	printf("Write count removexattr: %d\n", write_count);
	#endif

	return 0;
}
#endif /* HAVE_SETXATTR */

#ifdef HAVE_LIBULOCKMGR
static int xmp_lock(const char *path, struct fuse_file_info *fi, int cmd,
		    struct flock *lock)
{
	#ifdef TRACK_OP_COUNT
	allCount.lock++;
	#endif

	(void) path;

	#ifdef TRACK_WRITE_COUNT
	write_count++;
	printf("Write count lock: %d\n", write_count);
	#endif

	return ulockmgr_op(fi->fh, cmd, lock, &fi->lock_owner,
			   sizeof(fi->lock_owner));
}
#endif

static int xmp_flock(const char *path, struct fuse_file_info *fi, int op)
{
	#ifdef TRACK_OP_COUNT
	allCount.flock++;
	#endif

	int res;
	(void) path;

	res = flock(fi->fh, op);
	if (res == -1)
		return -errno;
	
	#ifdef TRACK_WRITE_COUNT
	write_count++;
	printf("Write count flock: %d\n", write_count);
	#endif

	return 0;
}

#ifdef HAVE_COPY_FILE_RANGE
static ssize_t xmp_copy_file_range(const char *path_in,
				   struct fuse_file_info *fi_in,
				   off_t off_in, const char *path_out,
				   struct fuse_file_info *fi_out,
				   off_t off_out, size_t len, int flags)
{
	#ifdef TRACK_OP_COUNT
	allCount.copy_file_range++;
	#endif

	ssize_t res;
	(void) path_in;
	(void) path_out;

	res = copy_file_range(fi_in->fh, &off_in, fi_out->fh, &off_out, len,
			      flags);
	if (res == -1)
		return -errno;

	#ifdef TRACK_WRITE_COUNT
	write_count++;
	printf("Write count copy_file_range: %d\n", write_count);
	#endif

	return res;
}
#endif

static off_t xmp_lseek(const char *path, off_t off, int whence, struct fuse_file_info *fi)
{
	#ifdef TRACK_OP_COUNT
	allCount.lseek++;
	#endif

	off_t res;
	(void) path;

	res = lseek(fi->fh, off, whence);
	if (res == -1)
		return -errno;

	return res;
}

static const struct fuse_operations xmp_oper = {
	.init           = xmp_init,
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.opendir	= xmp_opendir,
	.readdir	= xmp_readdir,
	.releasedir	= xmp_releasedir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
#ifdef HAVE_UTIMENSAT
	.utimens	= xmp_utimens,
#endif
	.create		= xmp_create,
	.open		= xmp_open,
	.read		= xmp_read,
	.read_buf	= xmp_read_buf,
	.write		= xmp_write,
	.write_buf	= xmp_write_buf,
	.statfs		= xmp_statfs,
	.flush		= xmp_flush,
	.release	= xmp_release,
	.fsync		= xmp_fsync,
#ifdef HAVE_POSIX_FALLOCATE
	.fallocate	= xmp_fallocate,
#endif
#ifdef HAVE_SETXATTR
	.setxattr	= xmp_setxattr,
	.getxattr	= xmp_getxattr,
	.listxattr	= xmp_listxattr,
	.removexattr	= xmp_removexattr,
#endif
#ifdef HAVE_LIBULOCKMGR
	.lock		= xmp_lock,
#endif
	.flock		= xmp_flock,
#ifdef HAVE_COPY_FILE_RANGE
	.copy_file_range = xmp_copy_file_range,
#endif
	.lseek		= xmp_lseek,
};

void exitHandler() {
	#ifdef TRACK_OP_COUNT
	printf("Number of times all operations were called: \n");
	printf("init: %d\n", allCount.init);
	printf("getattr: %d\n", allCount.getattr);
	printf("access: %d\n", allCount.access);
	printf("readlink: %d\n", allCount.readlink);
	printf("opendir: %d\n", allCount.opendir);
	printf("readdir: %d\n", allCount.readdir);
	printf("releasedir: %d\n", allCount.releasedir);
	printf("mknod: %d\n", allCount.mknod);
	printf("mkdir: %d\n", allCount.mkdir);
	printf("symlink: %d\n", allCount.symlink);
	printf("unlink: %d\n", allCount.unlink);
	printf("rmdir: %d\n", allCount.rmdir);
	printf("rename: %d\n", allCount.rename);
	printf("link: %d\n", allCount.link);
	printf("chmod: %d\n", allCount.chmod);
	printf("chown: %d\n", allCount.chown);
	printf("truncate: %d\n", allCount.truncate);
	printf("utimens: %d\n", allCount.utimens);
	printf("create: %d\n", allCount.create);
	printf("open: %d\n", allCount.open);
	printf("read: %d\n", allCount.read);
	printf("read_buf: %d\n", allCount.read_buf);
	printf("write: %d\n", allCount.write);
	printf("write_buf: %d\n", allCount.write_buf);
	printf("statfs: %d\n", allCount.statfs);
	printf("flush: %d\n", allCount.flush);
	printf("release: %d\n", allCount.release);
	printf("fsync: %d\n", allCount.fsync);
	printf("fallocate: %d\n", allCount.fallocate);
	printf("setxattr: %d\n", allCount.setxattr);
	printf("getxattr: %d\n", allCount.getxattr);
	printf("listxattr: %d\n", allCount.listxattr);
	printf("removexattr: %d\n", allCount.removexattr);
	printf("lock: %d\n", allCount.lock);
	printf("flock: %d\n", allCount.flock);
	printf("copy_file_range: %d\n", allCount.copy_file_range);
	printf("lseek: %d\n", allCount.lseek);
	#endif
	exit(0);
}

int main(int argc, char *argv[])
{
	umask(0);
	// Catch program exit, print useful info
	signal(SIGINT, exitHandler);
	return fuse_main(argc, argv, &xmp_oper, NULL);
}
