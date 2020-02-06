/*
 * super.c
 *
 * Copyright 2020 Lee JeYeon., Dankook Univ.
 *		2reenact@gmail.com
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/statfs.h>
#include <linux/buffer_head.h>
#include <linux/backing-dev.h>
#include <linux/kthread.h>
#include <linux/parser.h>
#include <linux/mount.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/random.h>
#include <linux/exportfs.h>
#include <linux/blkdev.h>
#include <linux/quotaops.h>
#include <linux/sysfs.h>
#include <linux/quota.h>
//#include <linux/sfs_fs.h>

#include "sfs.h"

static struct kmem_cache *sfs_inode_cachep;

static void init_once(void *foo)
{
	struct sfs_inode_info *si = (struct sfs_inode_info *) foo;

//	inode_init_once(&si->vfs_inode);
}

static int __init init_inode_cache(void)
{
	sfs_inode_cachep = kmem_cache_create_usercopy("sfs_inode_cache",
				sizeof(struct sfs_inode_info), 0,
				(SLAB_RECLAIM_ACCOUNT|SLAB_MEM_SPREAD|
					SLAB_ACCOUNT),
				offsetof(struct sfs_inode_info, i_data),
				sizeof_field(struct sfs_inode_info, i_data),
				init_once);
	if (sfs_inode_cachep == NULL)
		return -ENOMEM;
	return 0;
}

static void destroy_inode_cache(void)
{
	/*
	 * Make sure all delayed rcu free inodes are flushed before we
	 * destroy cache.
	 */
	rcu_barrier();
	kmem_cache_destroy(sfs_inode_cachep);
}

static int sfs_fill_super(struct super_block *sb, void *data, int silent)
{
	return 0;
}

static void kill_sfs_super(struct super_block *sb)
{

}

static struct dentry *sfs_mount(struct file_system_type *fs_type, int flags,
		const char *dev_name, void *data)
{
	return mount_bdev(fs_type, flags, dev_name, data, sfs_fill_super);
}

static struct file_system_type sfs_fs_type = {
	.owner		= THIS_MODULE,
	.name		= "sfs",
	.mount		= sfs_mount,
	.kill_sb	= kill_sfs_super,
	.fs_flags	= FS_REQUIRES_DEV,
};

static int __init init_sfs_fs(void)
{
	int err;

	err = init_inode_cache();
	if (err)
		return err;
	err = register_filesystem(&sfs_fs_type);
	if (err)
		goto fail;
	return 0;

fail:
	return err;
}

static void __exit exit_sfs_fs(void)
{
	unregister_filesystem(&sfs_fs_type);
	destroy_inode_cache();
}

module_init(init_sfs_fs)
module_exit(exit_sfs_fs)

MODULE_AUTHOR("Lee Jeyeon at DKU");
MODULE_DESCRIPTION("Simple File System");
MODULE_LICENSE("GPL");
