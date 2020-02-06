/*
 * mkfs_lib.c 
 *
 * Copyright 2020 Lee JeYeon., Dankook Univ.
 *		2reenact@gmail.com
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>
#include <mntent.h>
#include <time.h>
#include <sys/stat.h>
#ifndef ANDROID_WINDOWS_HOST
#include <sys/mount.h>
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SYSMACROS_H
#include <sys/sysmacros.h>
#endif
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#ifndef WITH_ANDROID
#ifdef HAVE_SCSI_SG_H
#include <scsi/sg.h>
#endif
#endif
#ifdef HAVE_LINUX_HDREG_H
#include <linux/hdreg.h>
#endif
#include <linux/limits.h>
#define __USE_GNU

#include "sfs_fs.h"
#include "mkfs.h"

extern struct sfs_configuration c;

/*
 * sfs bit operations
 */
static const int bits_in_byte[256] = {
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8,
};

int get_bits_in_byte(unsigned char n)
{
	return bits_in_byte[n];
}

int test_and_set_bit_le(u32 nr, u8 *addr)
{
	int mask, retval;

	addr += nr >> 3;
	mask = 1 << ((nr & 0x07));
	retval = mask & *addr;
	*addr |= mask;
	return retval;
}

int test_and_clear_bit_le(u32 nr, u8 *addr)
{
	int mask, retval;

	addr += nr >> 3;
	mask = 1 << ((nr & 0x07));
	retval = mask & *addr;
	*addr &= ~mask;
	return retval;
}

int test_bit_le(u32 nr, const u8 *addr)
{
	return ((1 << (nr & 7)) & (addr[nr >> 3]));
}

int sfs_test_bit(unsigned int nr, const char *p)
{
	int mask;
	char *addr = (char *)p;

	addr += (nr >> 3);
	mask = 1 << (7 - (nr & 0x07));
	return (mask & *addr) != 0;
}

int sfs_set_bit(unsigned int nr, char *addr)
{
	int mask;
	int ret;

	addr += (nr >> 3);
	mask = 1 << (7 - (nr & 0x07));
	ret = mask & *addr;
	*addr |= mask;
	return ret;
}

int sfs_clear_bit(unsigned int nr, char *addr)
{
	int mask;
	int ret;

	addr += (nr >> 3);
	mask = 1 << (7 - (nr & 0x07));
	ret = mask & *addr;
	*addr &= ~mask;
	return ret;
}

static inline u64 __ffs(u8 word)
{
	int num = 0;

	if ((word & 0xf) == 0) {
		num += 4;
		word >>= 4;
	}
	if ((word & 0x3) == 0) {
		num += 2;
		word >>= 2;
	}
	if ((word & 0x1) == 0)
		num += 1;
	return num;
}

/* Copied from linux/lib/find_bit.c */
#define BITMAP_FIRST_BYTE_MASK(start) (0xff << ((start) & (BITS_PER_BYTE - 1)))

static u64 _find_next_bit_le(const u8 *addr, u64 nbits, u64 start, char invert)
{
	u8 tmp;

	if (!nbits || start >= nbits)
		return nbits;

	tmp = addr[start / BITS_PER_BYTE] ^ invert;

	/* Handle 1st word. */
	tmp &= BITMAP_FIRST_BYTE_MASK(start);
	start = round_down(start, BITS_PER_BYTE);

	while (!tmp) {
		start += BITS_PER_BYTE;
		if (start >= nbits)
			return nbits;

		tmp = addr[start / BITS_PER_BYTE] ^ invert;
	}

	return min(start + __ffs(tmp), nbits);
}

u64 find_next_bit_le(const u8 *addr, u64 size, u64 offset)
{
	return _find_next_bit_le(addr, size, offset, 0);
}


u64 find_next_zero_bit_le(const u8 *addr, u64 size, u64 offset)
{
	return _find_next_bit_le(addr, size, offset, 0xff);
}

/*
 * try to identify the root device
 */
char *get_rootdev(void)
{
#if defined(ANDROID_WINDOWS_HOST) || defined(WITH_ANDROID)
        return NULL;
#else   
        struct stat st;
        int fd, ret;
        char buf[PATH_MAX + 1];
        char *uevent, *ptr; 
        char *rootdev;
        
        if (stat("/", &st) == -1)
                return NULL;
                
        snprintf(buf, PATH_MAX, "/sys/dev/block/%u:%u/uevent",
                major(st.st_dev), minor(st.st_dev));
                
        fd = open(buf, O_RDONLY);
        
        if (fd < 0)
                return NULL;
                
        ret = lseek(fd, (off_t)0, SEEK_END);
        (void)lseek(fd, (off_t)0, SEEK_SET);
        
        if (ret == -1) {
                close(fd);
                return NULL;
        }       
        
        uevent = malloc(ret + 1);
        ASSERT(uevent);
        
        uevent[ret] = '\0';
        
        ret = read(fd, uevent, ret);
        close(fd);
        
        ptr = strstr(uevent, "DEVNAME");
        if (!ptr)
                return NULL;
                
        ret = sscanf(ptr, "DEVNAME=%s\n", buf);
        if (strlen(buf) == 0)
                return NULL;
                
        ret = strlen(buf) + 5;
        rootdev = malloc(ret + 1);
        if (!rootdev)
                return NULL;
        rootdev[ret] = '\0';
        
        snprintf(rootdev, ret + 1, "/dev/%s", buf);
        return rootdev;
#endif  
}

static int is_mounted(const char *mpt, const char *device)
{
        FILE *file = NULL;
        struct mntent *mnt = NULL;

        file = setmntent(mpt, "r");
        if (file == NULL)
                return 0;

        while ((mnt = getmntent(file)) != NULL) {
                if (!strcmp(device, mnt->mnt_fsname)) {
/*
#ifdef MNTOPT_RO
                        if (hasmntopt(mnt, MNTOPT_RO))
                                c.ro = 1;
#endif
*/
                        break;
                }
        }
        endmntent(file);
        return mnt ? 1 : 0;
}

int sfs_dev_is_mounted(void)
{
        struct stat *st_buf;
        int is_rootdev = 0;
        int ret = 0;
        char *rootdev_name = get_rootdev();
        char *path = c.path;

        if (rootdev_name) {
                if (!strcmp(path, rootdev_name))
                        is_rootdev = 1;
                free(rootdev_name);
        }

        /*
         * try with /proc/mounts fist to detect RDONLY.
         */
#ifdef __linux__
        ret = is_mounted("/proc/mounts", path);
        if (ret) {
                MSG(0, "Info: Mounted device!\n");
                return -1;
        }
#endif
#if defined(MOUNTED) || defined(_PATH_MOUNTED)
#ifndef MOUNTED
#define MOUNTED _PATH_MOUNTED
#endif
        ret = is_mounted(MOUNTED, path);
        if (ret) {
                MSG(0, "Info: Mounted device!\n");
                return -1;
        }
#endif
        /*
         * If we are supposed to operate on the root device, then
         * also check the mounts for '/dev/root', which sometimes
         * functions as an alias for the root device.
         */
        if (is_rootdev) {
#ifdef __linux__
                ret = is_mounted("/proc/mounts", "/dev/root");
                if (ret) {
                        MSG(0, "Info: Mounted device!\n");
                        return -1;
                }
#endif
        }

        /*
         * If f2fs is umounted with -l, the process can still use
         * the file system. In this case, we should not format.
         */
        st_buf = malloc(sizeof(struct stat));
        ASSERT(st_buf);

        if (stat(path, st_buf) == 0 && S_ISBLK(st_buf->st_mode)) {
                int fd = open(path, O_RDONLY | O_EXCL);

                if (fd >= 0) {
                        close(fd);
                } else if (errno == EBUSY) {
                        MSG(0, "\tError: In use by the system!\n");
                        free(st_buf);
                        return -1;
                }
        }
        free(st_buf);
        return ret;
}

int log_base_2(u_int32_t num)
{
	int ret = 0;
	if (num <= 0 || (num & (num - 1)) != 0)
		return -1;
	while (num >>= 1)
		ret++;
	return ret;
}

static int open_check_fs(char *path, int flag)
{
	/* allow to open ro */
	return open(path, O_RDONLY | flag);
}

int get_device_info(void)
{
        int32_t fd = 0;
        u_int32_t sector_size;
#ifndef BLKGETSIZE64
        u_int32_t total_sectors;
#endif
        struct stat *stat_buf;
#ifdef HDIO_GETGIO
        struct hd_geometry geom;
#endif
        stat_buf = malloc(sizeof(struct stat));
        ASSERT(stat_buf);

        if (stat(c.path, stat_buf) < 0 ) {
                MSG(0, "\tError: Failed to get the device stat!\n");
                free(stat_buf);
                return -1;
        }

        if (S_ISBLK(stat_buf->st_mode)) {
                fd = open(c.path, O_RDWR | O_EXCL);
                if (fd < 0)
                        fd = open_check_fs(c.path, O_EXCL);
        } else {
                fd = open(c.path, O_RDWR);
                if (fd < 0)
                        fd = open_check_fs(c.path, 0);
        }

        if (fd < 0) {
                MSG(0, "\tError: Failed to open the device!\n");
                free(stat_buf);
                return -1;
        }

        c.fd = fd;

        if (S_ISREG(stat_buf->st_mode)) {
                c.total_sectors = stat_buf->st_size / c.sector_size;
        } else if (S_ISBLK(stat_buf->st_mode)) {
#ifdef BLKSSZGET
                if (ioctl(fd, BLKSSZGET, &sector_size) < 0)
                        MSG(0, "\tError: Using the default sector size\n");
                else if (c.sector_size < sector_size)
                        c.sector_size = sector_size;
#endif
#ifdef BLKGETSIZE64
                if (ioctl(fd, BLKGETSIZE64, &c.total_sectors) < 0) {
                        MSG(0, "\tError: Cannot get the device size\n");
                        free(stat_buf);
                        return -1;
                }
#else
                if (ioctl(fd, BLKGETSIZE, &total_sectors) < 0) {
                        MSG(0, "\tError: Cannot get the device size\n");
                        free(stat_buf);
                        return -1;
                }
                c.total_sectors = total_sectors;
#endif
                c.total_sectors /= c.sector_size;

                c.start_blkaddr = 0;
        } else {
                MSG(0, "\tError: Volume type is not supported!!!\n");
                free(stat_buf);
                return -1;
        }

	if(!c.sector_size) {
		c.sector_size = sector_size;
		c.sectors_per_block = SFS_BLKSIZE / c.sector_size;
	}

	if(c.total_sectors)
		c.end_blkaddr = (c.total_sectors >> c.sectors_per_block) - 1;

        free(stat_buf);
        return 0;
}

int sfs_get_device_info(void)
{
        if (get_device_info())
                return -1;
#ifdef SFS_MAX_BLOCK
        if (c.total_sectors * c.sector_size >
                (u_int64_t)SFS_MAX_BLOCK * 512 * 2 * 1024 * 1024) {
                MSG(0, "\tError: SFS can support %d GB at most!!!\n", SFS_MAX_BLOCK / 262144);
                return -1;
        }
#endif

        MSG(0, "Info: sector size = %u\n", c.sector_size);
//	MSG(0, "Info: total sectors = %"PRIu64" (%"PRIu64" MB)\n",
//                                c.total_sectors, (c.total_sectors *
//                                        (c.sector_size >> 9)) >> 11);
	return 0;
}

