/*
 * sfs.h
 *
 * Copyright 2020 Lee JeYeon., Dankook Univ.
 *		2reenact@gmail.com
 *
 */

#ifndef _SFS_H
#define _SFS_H

#include <linux/fs.h>
#include <linux/blockgroup_lock.h>
#include <linux/percpu_counter.h>
#include <linux/rbtree.h>

//#include "sfs_fs.h"

struct sfs_inode_info {
	__le32 i_data[15];
	__u32 i_flags;

	rwlock_t i_meta_lock;

	struct mutex truncate_mutex;
	struct inode vfs_inode;
	struct list_head i_orphan;
};


#endif /* _SFS_H */
