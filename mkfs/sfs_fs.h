/*
 * sfs_fs.h
 *
 * Copyright 2020 Lee JeYeon., Dankook Univ.
 *		2reenact@gmail.com
 *
 */

#ifndef _SFS_FS_H
#define _SFS_FS_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

typedef unsigned long	u64;
typedef unsigned int	u32;
typedef unsigned short	u16;
typedef unsigned char	u8;

typedef u64	__u64;
typedef u32	__u32;
typedef u16	__u16;
typedef u8	__u8;
typedef u64	__le64;
typedef u32	__le32;
typedef u16	__le16;
typedef u8	__le8;

typedef u32	block_t;
typedef u8	book;

#if HAVE_BYTESWAP_H
#include <byteswap.h>
#else
/**
 * bswap_16 - reverse bytes in a uint16_t value.
 * @val: value whose bytes to swap.
 *
 * Example:
 *	// Output contains "1024 is 4 as two bytes reversed"
 *	printf("1024 is %u as two bytes reversed\n", bswap_16(1024));
 */
static inline u_int16_t bswap_16(u_int16_t val)
{
	return ((val & (u_int16_t)0x00ffU) << 8)
		| ((val & (u_int16_t)0xff00U) >> 8);
}

/**
 * bswap_32 - reverse bytes in a uint32_t value.
 * @val: value whose bytes to swap.
 *
 * Example:
 *	// Output contains "1024 is 262144 as four bytes reversed"
 *	printf("1024 is %u as four bytes reversed\n", bswap_32(1024));
 */
static inline u_int32_t bswap_32(u_int32_t val)
{
	return ((val & (u_int32_t)0x000000ffUL) << 24)
		| ((val & (u_int32_t)0x0000ff00UL) <<  8)
		| ((val & (u_int32_t)0x00ff0000UL) >>  8)
		| ((val & (u_int32_t)0xff000000UL) >> 24);
}
#endif /* !HAVE_BYTESWAP_H */

#if defined HAVE_DECL_BSWAP_64 && !HAVE_DECL_BSWAP_64
/**
 * bswap_64 - reverse bytes in a uint64_t value.
 * @val: value whose bytes to swap.
 *
 * Example:
 *	// Output contains "1024 is 1125899906842624 as eight bytes reversed"
 *	printf("1024 is %llu as eight bytes reversed\n",
 *		(unsigned long long)bswap_64(1024));
 */
static inline u_int64_t bswap_64(u_int64_t val)
{
	return ((val & (u_int64_t)0x00000000000000ffULL) << 56)
		| ((val & (u_int64_t)0x000000000000ff00ULL) << 40)
		| ((val & (u_int64_t)0x0000000000ff0000ULL) << 24)
		| ((val & (u_int64_t)0x00000000ff000000ULL) <<  8)
		| ((val & (u_int64_t)0x000000ff00000000ULL) >>  8)
		| ((val & (u_int64_t)0x0000ff0000000000ULL) >> 24)
		| ((val & (u_int64_t)0x00ff000000000000ULL) >> 40)
		| ((val & (u_int64_t)0xff00000000000000ULL) >> 56);
}
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define le16_to_cpu(x)	((__u16)(x))
#define le32_to_cpu(x)	((__u32)(x))
#define le64_to_cpu(x)	((__u64)(x))
#define cpu_to_le16(x)	((__u16)(x))
#define cpu_to_le32(x)	((__u32)(x))
#define cpu_to_le64(x)	((__u64)(x))
#elif __BYTE_ORDER == __BIG_ENDIAN
#define le16_to_cpu(x)	bswap_16(x)
#define le32_to_cpu(x)	bswap_32(x)
#define le64_to_cpu(x)	bswap_64(x)
#define cpu_to_le16(x)	bswap_16(x)
#define cpu_to_le32(x)	bswap_32(x)
#define cpu_to_le64(x)	bswap_64(x)
#endif

#define typecheck(type,x) \
	({	type __dummy; \
		typeof(x) __dummy2; \
		(void)(&__dummy == &__dummy2); \
		1; \
	 })

/*
 * Debugging interfaces
 */
#define FIX_MSG(fmt, ...)						\
	do {								\
		printf("[FIX] (%s:%4d) ", __func__, __LINE__);		\
		printf(" --> "fmt"\n", ##__VA_ARGS__);			\
	} while (0)

#define ASSERT_MSG(fmt, ...)						\
	do {								\
		printf("[ASSERT] (%s:%4d) ", __func__, __LINE__);	\
		printf(" --> "fmt"\n", ##__VA_ARGS__);			\
		c.bug_on = 1;						\
	} while (0)

#define ASSERT(exp)							\
	do {								\
		if (!(exp)) {						\
			printf("[ASSERT] (%s:%4d) " #exp"\n",		\
					__func__, __LINE__);		\
			exit(-1);					\
		}							\
	} while (0)

#define ERR_MSG(fmt, ...)						\
	do {								\
		printf("[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__); \
	} while (0)

#define MSG(n, fmt, ...)						\
	do {								\
		if (c.dbg_lv >= n) {					\
			printf(fmt, ##__VA_ARGS__);			\
		}							\
	} while (0)

#define DBG(n, fmt, ...)						\
	do {								\
		if (c.dbg_lv >= n) {					\
			printf("[%s:%4d] " fmt,				\
				__func__, __LINE__, ##__VA_ARGS__);	\
		}							\
	} while (0)

/* these are defined in kernel */
#ifndef PAGE_SIZE
#define PAGE_SIZE		4096
#endif
#define PAGE_CACHE_SIZE		4096
#define BITS_PER_BYTE		8
#define SFS_SUPER_MAGIC		0x202005F5	/* SFS Magic Number */
#define MAX_PATH_LEN		24

#define SFS_BYTES_TO_BLK(bytes)    ((bytes) >> SFS_BLKSIZE_BITS)
#define SFS_BLKSIZE_BITS	12

struct sfs_configuration {
	int heap;
	int dbg_lv;
	int trim;

	int32_t fd;
	u_int32_t sector_size;
	u_int32_t sectors_per_block;
	u_int64_t start_blkaddr;
	u_int64_t end_blkaddr;
	u_int64_t total_sectors;
	u_int32_t total_blocks;

	char *vol_label;
	char *path;

	u_int32_t root_uid;
	u_int32_t root_gid;
} __attribute__((packed));

#define set_sb_le64(member, val)		(sb->member = cpu_to_le64(val))
#define set_sb_le32(member, val)		(sb->member = cpu_to_le32(val))
#define set_sb_le16(member, val)		(sb->member = cpu_to_le16(val))
#define get_sb_le64(member)			le64_to_cpu(sb->member)
#define get_sb_le32(member)			le32_to_cpu(sb->member)
#define get_sb_le16(member)			le16_to_cpu(sb->member)

#define set_sb(member, val)						 \
			do {						 \
				typeof(sb->member) t;			 \
				switch (sizeof(t)) {			 \
				case 8: set_sb_le64(member, val); break; \
				case 4: set_sb_le32(member, val); break; \
				case 2: set_sb_le16(member, val); break; \
				}					 \
			} while(0)

#define get_sb(member)							\
			({						\
				typeof(sb->member) t;			\
				switch (sizeof(t)) {			\
				case 8: t = get_sb_le64(member); break; \
				case 4: t = get_sb_le32(member); break; \
				case 2: t = get_sb_le16(member); break; \
				} 					\
				t;					\
			})

/*
 * Copied from include/linux/kernel.h
 */
#define __round_mask(x, y)	((__typeof__(x))((y)-1))
#define round_down(x, y)	((x) & ~__round_mask(x, y))

#define min(x, y) ({				\
	typeof(x) _min1 = (x);			\
	typeof(y) _min2 = (y);			\
	(void) (&_min1 == &_min2);		\
	_min1 < _min2 ? _min1 : _min2; })

#define max(x, y) ({				\
	typeof(x) _max1 = (x);			\
	typeof(y) _max2 = (y);			\
	(void) (&_max1 == &_max2);		\
	_max1 > _max2 ? _max1 : _max2; })

#define SFS_SUPER_OFFSET		1024	/* byte-size offset */
#define	DEFAULT_SECTOR_SIZE		512	/* sector size is 512 bytes */
#define	DEFAULT_SECTORS_PER_BLOCK	8	
#define SFS_BLKSIZE			4096	/* support only 4KB block */
#define SFS_SECTOR_SIZE			9	/* 9 bits for 512 bytes */
#define SFS_BLOCK_SIZE			12	/* 12 bits for 4KB */
#define SFS_BLOCK_ALIGN(x)	(((x) + SFS_BLKSIZE - 1) / SFS_BLKSIZE)

#define NULL_ADDR		0x0U
#define NEW_ADDR		-1U

#define SFS_ROOT_INO(sbi)	(sbi->root_ino_num)

struct sfs_super_block {
        __le32 magic;                   /* Magic Number */
        __le32 sector_size;		/* sector size in bytes */
        __le32 sectors_per_block;       /* # of sectors per block */
        __le32 block_size;		/* block size in bytes */
        __le64 block_count;             /* total # of user blocks */
	__le32 start_block_addr;	/* block 0 byte address  */
        __le32 imap_blkaddr;            /* start block address of inode bmap */
        __le32 dmap_blkaddr;            /* start block address of data bmap */
        __le32 inodes_blkaddr;          /* start block address of inodes */
        __le32 data_blkaddr;            /* start block address of data */
        __le32 block_count_imap;        /* # of blocks for inode bmap */
        __le32 block_count_dmap;        /* # of blocks for data bmap */
        __le32 block_count_inodes;      /* # of blocks for inode */
        __le32 block_count_data;        /* # of blocks for data */
        __le32 root_addr;               /* root inode blkaddr */
	char path[MAX_PATH_LEN];
} __attribute__((packed));

#define DEF_ADDRS_PER_INODE     12      /* Address Pointers in an Inode */
#define DEF_NIDS_PER_INODE      3       /* Node IDs in an Inode */
#define DEF_ADDRS_PER_BLOCK     1024    /* Address Pointers in a Indirect Block */

#define SFS_NAME_LEN		8

struct sfs_inode {
        __le16 i_mode;                  /* file mode */
        __u8 i_advise;                  /* file hints */
        __u8 i_inline;                  /* file inline flags */
        __le32 i_uid;                   /* user ID */
        __le32 i_gid;                   /* group ID */
        __le32 i_links;                 /* links count */
        __le64 i_size;                  /* file size in bytes */
        __le64 i_blocks;                /* file size in blocks */
        __le64 i_atime;                 /* access time */
        __le64 i_ctime;                 /* creation time */
        __le64 i_mtime;                 /* modification time */
        __le32 i_atime_nsec;            /* access time in nano scale */
        __le32 i_ctime_nsec;            /* creation time in nano scale */
        __le32 i_mtime_nsec;            /* modification time in nano scale */
        __le32 i_flags;                 /* file attributes */
        __le32 i_pino;                  /* parent inode number */
        __le32 i_namelen;               /* file name length */
        __u8 i_name[SFS_NAME_LEN];      /* file name for SPOR */

        __le32 d_addr[DEF_ADDRS_PER_INODE];     /* Pointers to data blocks */
        __le32 i_addr[DEF_NIDS_PER_INODE];      /* indirect, double indirect,
                                                triple_indirect block address*/
} __attribute__((packed));

struct indirect_node {
        __le32 addr[DEF_ADDRS_PER_BLOCK];       /* array of data block address */
} __attribute__((packed));

/* One directory entry slot covers 8byte-long file name */
#define SFS_SLOT_LEN		8
#define SFS_SLOT_LEN_BITS	3

/* the number of dentry in a block */
#define DENTRY_IN_BLOCK		256

/*  */
#define SIZE_OF_DIR_ENTRY	8       /* by byte */
#define SIZE_OF_DENTRY_BITMAP   ((DENTRY_IN_BLOCK + BITS_PER_BYTE - 1) / \
                                        BITS_PER_BYTE)
#define SIZE_OF_RESERVED        (PAGE_SIZE - ((SIZE_OF_DIR_ENTRY + \
                                SFS_SLOT_LEN) * \
                                DENTRY_IN_BLOCK + SIZE_OF_DENTRY_BITMAP))

struct sfs_dir_entry {
        __le16 file_type;               /* file type */
        __le16 name_len;                /* length of file name */
        __le32 i_addr;                  /* inode address */
        __u8 filename[SFS_SLOT_LEN];    /* file name */
} __attribute__((packed));

/* 4KB-sized directory entry block */
struct sfs_dentry_block {
	__u8 dentry_bitmap[SIZE_OF_DENTRY_BITMAP];
//	__u8 reserved[SIZE_OF_RESERVED];
        struct sfs_dir_entry dentry[DENTRY_IN_BLOCK];
} __attribute__((packed));

/* file types used in inode_info->flags */
enum {
        SFS_UNKNOWN,
        SFS_REG_FILE,
        SFS_DIR,
        SFS_SYMLINK
};

#define SFS_IMAP_BLK_OFFSET		2	/* imap block offset is 1 */
#define SFS_IMAP_BYTE_OFFSET		4096	/* imap byte offset is 4096 */
#define MAP_SIZE_ALIGN(size)		((size) + SFS_BLKSIZE) / SFS_BLKSIZE

#define SFS_NODE_RATIO			128	/* node : data ratio is 1 : 128 */

#endif /* _SFS_FS_H */

