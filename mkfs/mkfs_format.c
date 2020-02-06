/*
 * mkfs_format.c
 *
 * Copyright 2020 Lee JeYeon., Dankook Univ.
 *		2reenact@gmail.com
 *
 */
#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE
#endif
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#ifndef ANDROID_WINDOWS_HOST
#include <sys/ioctl.h>
#include <sys/mount.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>

#ifdef HAVE_LINUX_FS_H
#include <linux/fs.h>
#endif
#ifdef HAVE_LINUX_FALLOC_H
#include <linux/falloc.h>
#endif
#include <time.h>
#include <uuid/uuid.h>

#ifndef BLKDISCARD
#define BLKDISCARD	_IO(0x12, 119)
#endif
#ifndef BLKSECDISCARD
#define BLKSECDISCARD	_IO(0x12, 125)
#endif

#include "sfs_fs.h"
#include "mkfs.h"

extern struct sfs_configuration c;

struct sfs_super_block raw_sb;
struct sfs_super_block *sb = &raw_sb;

static int sfs_prepare_super_block(void) {
        u_int32_t block_size, total_block_count;
        u_int32_t sector_size, sectors_per_block;
        u_int32_t imap_blkaddr, dmap_blkaddr;
        u_int32_t inodes_blkaddr, data_blkaddr;
        u_int32_t block_count_imap, block_count_dmap;
        u_int32_t block_count_inodes, block_count_data;
        u_int32_t root_addr;

        set_sb(magic, SFS_SUPER_MAGIC);

        sector_size = log_base_2(c.sector_size);
        sectors_per_block = log_base_2(c.sectors_per_block);
        block_size = sector_size + sectors_per_block;
        total_block_count = c.total_sectors >> sectors_per_block;

	set_sb(sector_size, sector_size);
        set_sb(sectors_per_block, sectors_per_block);
        set_sb(block_size, block_size);
	set_sb(block_count, total_block_count);

	set_sb(start_block_addr, c.start_blkaddr);
	memcpy(sb->path, c.path, MAX_PATH_LEN);

	imap_blkaddr = SFS_IMAP_BLK_OFFSET;
	set_sb(imap_blkaddr, imap_blkaddr);

	total_block_count = total_block_count - 2;
	block_count_inodes = total_block_count >> log_base_2(SFS_NODE_RATIO);
	block_count_imap = MAP_SIZE_ALIGN(block_count_inodes);
	set_sb(block_count_imap, block_count_imap);

	dmap_blkaddr = imap_blkaddr + block_count_imap;
	set_sb(dmap_blkaddr,dmap_blkaddr);

	total_block_count = total_block_count - (block_count_imap + block_count_inodes);
	block_count_data = SFS_BLKSIZE * (1 + total_block_count) / (SFS_BLKSIZE + 1);
	block_count_dmap = MAP_SIZE_ALIGN(block_count_data);
	set_sb(block_count_dmap, block_count_dmap);

	inodes_blkaddr = dmap_blkaddr + block_count_dmap;
	set_sb(inodes_blkaddr, inodes_blkaddr);
	set_sb(block_count_inodes, block_count_inodes);

	data_blkaddr = inodes_blkaddr + block_count_inodes;
	set_sb(data_blkaddr, data_blkaddr);
	set_sb(block_count_data, block_count_data);

	root_addr = inodes_blkaddr;
	set_sb(root_addr, root_addr);

	return 0;
}

static int trim_device(void)
{
        unsigned long long range[2];
        struct stat *stat_buf;
	u_int64_t bytes = c.total_sectors * c.sector_size;
        int fd = c.fd;

        stat_buf = malloc(sizeof(struct stat));
        if (stat_buf == NULL) {
                MSG(1, "\tError: Malloc Failed for trim_stat_buf!!!\n");
                return -1;
        }

        if (fstat(fd, stat_buf) < 0 ) {
                MSG(1, "\tError: Failed to get the device stat!!!\n");
                free(stat_buf);
                return -1;
        }

        range[0] = 0;
        range[1] = bytes;

        MSG(0, "Info: [%s] Discarding device\n", c.path);
        if (S_ISBLK(stat_buf->st_mode)) {
                if (ioctl(fd, BLKDISCARD, &range) < 0) {
                        MSG(0, "Info: This device doesn't support BLKDISCARD\n");
                } else {
                        MSG(0, "Info: Discarded %llu MB\n", range[1] >> 20);
                }
        } else {
                free(stat_buf);
                return -1;
        }
        free(stat_buf);
        return 0;
}

static int sfs_add_default_dentry_root(void)
{
	struct sfs_dentry_block *dent_blk = NULL;
	u_int64_t data_blk_offset = 0;

	dent_blk = calloc(SFS_BLKSIZE, 1);
	if (dent_blk == NULL) {
		MSG(1, "\tError: Calloc Failed for dent_blk!!!\n");
		return -1;
	}
	
	dent_blk->dentry[0].file_type = SFS_DIR;
	dent_blk->dentry[0].i_addr = sb->root_addr;
	dent_blk->dentry[0].name_len = cpu_to_le16(1);
	memcpy(dent_blk->dentry[0].filename, ".", 1);

	dent_blk->dentry[1].file_type = SFS_DIR;
	dent_blk->dentry[1].i_addr = sb->root_addr;
	dent_blk->dentry[1].name_len = cpu_to_le16(2);
	memcpy(dent_blk->dentry[1].filename, "..", 2);

	test_and_set_bit_le(0, dent_blk->dentry_bitmap);
	test_and_set_bit_le(1, dent_blk->dentry_bitmap);

	data_blk_offset = get_sb(data_blkaddr);
/*
	DBG(1, "\tWriting default dentry root, at offset 0x%08"PRIx64"\n",
			 data_blk_offset);
*/
	if (dev_write_block(dent_blk, data_blk_offset)) {
		MSG(1, "\tError: While writing the dentry_blk to disk!!!\n");
		free(dent_blk);
		return -1;
	}

	free(dent_blk);
	return 0;
}

static int sfs_write_root_inode(void)
{
	struct sfs_inode *raw_node = NULL;
	u_int64_t block_size_byte, data_blk_nor;
	u_int64_t inodes_offset = 0;

	raw_node = calloc(SFS_BLKSIZE, 1);
	if (raw_node == NULL) {
		MSG(1, "\tError: Calloc Failed for raw_node!!!\n");
		return -1;
	}
	raw_node->i_mode = cpu_to_le16(0x41ed);
	raw_node->i_uid = cpu_to_le32(c.root_uid);
	raw_node->i_gid = cpu_to_le32(c.root_gid);

	block_size_byte = 1 << get_sb(block_size);
	raw_node->i_size = cpu_to_le64(1 * block_size_byte);
	raw_node->i_blocks = cpu_to_le64(2);
	raw_node->d_addr[0] = get_sb(data_blkaddr);

	raw_node->i_atime = cpu_to_le64(time(NULL));
	raw_node->i_atime_nsec = 0;
	raw_node->i_ctime = cpu_to_le64(time(NULL));
	raw_node->i_ctime_nsec = 0;
	raw_node->i_mtime = cpu_to_le64(time(NULL));
	raw_node->i_mtime_nsec = 0;
	raw_node->i_flags = 0;
	raw_node->i_pino = 0;

	inodes_offset = get_sb(inodes_blkaddr);

/*
	DBG(1, "\tWriting root inode, %x %x %x at offset 0x%08"PRIu64"\n",
			get_sb(root_blkaddr));
*/
	if (write_inode(raw_node, inodes_offset) < 0) {
		MSG(1, "\tError: While writing the raw_node to disk!!!\n");
		free(raw_node);
		return -1;
	}
	free(raw_node);
	return 0;
}

static int sfs_update_imap(u32 blkaddr)
{
	char *imap = NULL;
	u_int64_t offset;
	u_int64_t imap_blkaddr = get_sb(imap_blkaddr);

	imap = calloc(SFS_BLKSIZE, 1);
	if (imap == NULL) {
		MSG(1, "\tError: Calloc Failed for imap!!!\n");
		return -1;
	}

	offset = blkaddr - imap_blkaddr;
	
	imap_blkaddr += offset >> get_sb(block_size);
	offset = offset & 0xfff;

	test_and_set_bit_le(offset, imap);

	if (dev_write_block(imap, imap_blkaddr) < 0) {
		MSG(1, "\tError: While writing the imap to disk!!!\n");
		free(imap);
		return -1;
	}
	free(imap);
	return 0;
}

static int sfs_update_dmap(u32 blkaddr)
{
	char *dmap = NULL;
	u_int64_t offset;
	u_int64_t dmap_blkaddr = get_sb(dmap_blkaddr);

	dmap = calloc(SFS_BLKSIZE, 1);
	if (dmap == NULL) {
		MSG(1, "\tError: Calloc Failed for dmap!!!\n");
		return -1;
	}

	offset = blkaddr - dmap_blkaddr;
	
	dmap_blkaddr += offset >> get_sb(block_size);
	offset = offset & 0xfff;

	test_and_set_bit_le(offset, dmap);


	if (dev_write_block(dmap, dmap_blkaddr) < 0) {
		MSG(1, "\tError: While writing the dmap to disk!!!\n");
		free(dmap);
		return -1;
	}
	free(dmap);
	return 0;
}

static int sfs_update_maps_root(void)
{
	int err = 0;
	u_int64_t imap_blkaddr = get_sb(imap_blkaddr);
	u_int64_t dmap_blkaddr = get_sb(dmap_blkaddr);

	err = sfs_update_imap(imap_blkaddr);
	if (err < 0) {
		MSG(1, "\tError: Failed to update imap for root!!!\n");
		return -1;
	}

	err = sfs_update_dmap(dmap_blkaddr);
	if (err < 0) {
		MSG(1, "\tError: Failed to update dmap for root!!!\n");
		return -1;
	}
	return err;
}

static int sfs_create_root_dir(void)
{
	int err = 0;

	err = sfs_add_default_dentry_root();
	if (err < 0) {
		MSG(1, "\tError: Failed to add default dentries for root!!!\n");
		goto exit;
	}

	err = sfs_write_root_inode();
	if (err < 0) {
		MSG(1, "\tError: Failed to write root inode!!!\n");
		goto exit;
	}

	err = sfs_update_maps_root();
	if (err < 0) {
		MSG(1, "\tError: Failed to update maps for root!!!\n");
		goto exit;
	}
exit:
	if (err)
		MSG(1, "\tError: Could not create the root directory!!!\n");

	return err;
}

static int sfs_write_super_block(void)
{
	int index;
	u_int8_t *zero_buff;

	zero_buff = calloc(SFS_BLKSIZE, 1);
	if (zero_buff == NULL) {
		MSG(1, "\tError: Calloc Failed for super_blk_zero_buf!!!\n");
		return -1;
	}

	memcpy(zero_buff + SFS_SUPER_OFFSET, sb, sizeof(*sb));
	DBG(1, "\tWriting super block, at offset 0x%08x\n", 0);
	for (index = 0; index < 2; index++) {
		if (dev_write_block(zero_buff, index)) {
			MSG(1, "\tError: While while writing super_blk "
				"on disk!!! index : %d\n", index);
			free(zero_buff);
			return -1;
		}
	}

	free(zero_buff);
	return 0;
}

int sfs_format_device(void)
{
        int err = 0;

        err= sfs_prepare_super_block();
        if (err < 0) {
                MSG(0, "\tError: Failed to prepare a super block!!!\n");
                goto exit;
        }
        if (c.trim) {
                err = trim_device();
                if (err < 0) {
                        MSG(0, "\tError: Failed to trim whole device!!!\n");
                        goto exit;
                }
        }

        err = sfs_create_root_dir();
        if (err < 0) {
                MSG(0, "\tError: Failed to create the root directory!!!\n");
                goto exit;
        }

        err = sfs_write_super_block();
        if (err < 0) {
                MSG(0, "\tError: Failed to write the super block!!!\n");
                goto exit;
        }
exit:
        if (err)
                MSG(0, "\tError: Could not format the device!!!\n");

        return err;
}

