/*
 * mkfs_io.c
 *
 * Copyright 2020 Lee JeYeon., Dankook Univ.
 *		2reenact@gmail.com
 *
 */

#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef HAVE_MNTENT_H
#include <mntent.h>
#endif
#include <time.h>
#ifndef ANDROID_WINDOWS_HOST
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#endif
#ifdef HAVE_LINUX_HDREG_H
#include <linux/hdreg.h>
#endif

#include "sfs_fs.h"
#include "mkfs.h"

extern struct sfs_configuration c;

static int __check_offset(__u64 *offset)
{
	__u64 blk_addr = *offset >> SFS_BLKSIZE_BITS;
	int i;

	if (c.start_blkaddr <= blk_addr && c.end_blkaddr >= blk_addr) {
		*offset -= c.start_blkaddr << SFS_BLKSIZE_BITS;
		return c.fd;
	}
	return -1;
}

static int sparse_read_blk(__u64 block, int count, void *buf) { return 0; }
static int sparse_write_blk(__u64 block, int count, const void *buf) { return 0; }

int dev_write(void *buf, __u64 offset, size_t len)
{
	int fd;

	fd = __check_offset(&offset);
	if (fd < 0)
		return fd;

	if (lseek64(fd, (off64_t)offset, SEEK_SET) < 0)
		return -1;
	if (write(fd, buf, len) < 0)
		return -1;
	return 0;
}

int dev_write_block(void *buf, __u64 blk_addr)
{
	return dev_write(buf, blk_addr << SFS_BLKSIZE_BITS, SFS_BLKSIZE);
}

int write_inode(struct sfs_node *inode, u64 blkaddr)
{
	return dev_write_block(inode, blkaddr);
}
