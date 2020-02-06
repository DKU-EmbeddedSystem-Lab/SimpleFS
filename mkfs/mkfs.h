/*
 * mkfs.h 
 *
 * Copyright 2020 Lee JeYeon., Dankook Univ.
 *		2reenact@gmail.com
 *
 */

#define _LARGEFILE64_SOURCE

#ifndef _MKFS_H
#define _MFKS_H

#include "sfs_fs.h"

#define SFS_TOOLS_VERSION	20201
#define SFS_TOOLS_DATE		8

struct sfs_configuration c;

/* mkfs_main.c */
static void mkfs_usage(void);
static void sfs_show_info(void);
void sfs_init_configurtaion(void);
static void sfs_parse_option(int argc, char *argv[]);

/* mkfs_lib.c */
char *get_rootdev(void);
static int is_mounted(const char *mpt, const char *device);
int sfs_dev_is_mounted(void);
int log_base(u_int32_t num);
static int open_check_fs(char *path, int flag);
int get_device_info(void);
int sfs_get_device_info(void);

/* mkfs_format.c */
static int sfs_prepare_super_block(void);
static int sfs_add_default_dentry_root(void);
static int sfs_write_root_inode(void);
static int sfs_create_root_dir(void);
static int sfs_write_super_block(void);
int sfs_format_device(void);

/* mkfs_io.c */
int dev_write(void *buf, __u64 offset, size_t len);
int dev_write_block(void *buf, __u64 blk_addr);

#endif /* _MKFS_H */
