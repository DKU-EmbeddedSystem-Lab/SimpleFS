/*
 * mkfs_main.c 
 *
 * Copyright 2020 Lee JeYeon., Dankook Univ.
 *		2reenact@gmail.com
 *
 */

#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#ifndef ANDROID_WINDOWS_HOST
#include <sys/mount.h>
#endif
#include <time.h>
#include <uuid/uuid.h>
#include <errno.h>

#ifdef HAVE_LIBBLKID
#include <blkid/blkid.h>
#endif

#include "sfs_fs.h"
#include "mkfs.h"

extern struct sfs_configuration c;

static void mkfs_usage(void) {
	MSG(0, "\nUsage: mkfs.sfs [options] device [sectors]\n");
	MSG(0, "[options]:\n");
	MSG(0, "  -a heap-based allocation [default:0]\n");
	MSG(0, "  -d debug level [default:0]\n");
	MSG(0, "  -l label\n");
	exit(1);
}

static void sfs_show_info(void) {
        MSG(0, "\n\tSFS-tools: mkfs.sfs Ver: %d (%d)\n\n",
                                SFS_TOOLS_VERSION,
                                SFS_TOOLS_DATE);

	if (c.heap == 0)
		MSG(0, "Info: Disable heap-based policy\n");

	MSG(0, "Info: Debug level = %d\n", c.dbg_lv);

	if (strlen(c.vol_label))
		MSG(0, "Info: Lable = %s\n", c.vol_label);

	MSG(0, "Info: Trim is %s\n", c.trim ? "enalbe": "disable");
}

/*
 * device information
 */
void sfs_init_configuration(void)
{
        c.heap = 1;
        c.trim = 1;
        c.sector_size = DEFAULT_SECTOR_SIZE;
        c.sectors_per_block = DEFAULT_SECTORS_PER_BLOCK;
        c.vol_label = "";
        c.path = NULL;

	c.root_uid = getuid();
	c.root_gid = getgid();
}

static void sfs_parse_options(int argc, char *argv[])
{
        static const char *option_string = "a:d:l:";
        int32_t option=0;

        while ((option = getopt(argc, argv, option_string) != EOF)) {
                switch (option) {
                case 'a':
//			config.heap = atoi(optarg);
//			if (config.heap == 0)
//			MSG(0, "Info: Disable heap-based policy\n");
                        break;
                case 'd':
			c.dbg_lv = atoi(optarg);
			MSG(0, "Info: Debug level = %d\n", c.dbg_lv);
                        break;
                case 'l':
                        if (strlen(optarg) > 512) {
                                MSG(0, "Error: Volume Label should be less than\
                                                512 characters\n");
                                mkfs_usage();
                        }
//			config.vol_label = optarg;
//			MSG(0, "Info: Label = %s\n", config.vol_label);
                        break;
                default:
                        MSG(0, "\tError: Unknown option %c\n", option);
                        mkfs_usage();
                        break;
		}
	}

        if (optind >= argc) {
                MSG(0, "\tError: Device not specified\n");
                mkfs_usage();
        }

	c.path = strdup(argv[optind]);
}

int main(int argc, char *argv[]) {
	sfs_init_configuration();

        sfs_parse_options(argc, argv);

	sfs_show_info();

        if (sfs_dev_is_mounted())
                return -1;

        if (sfs_get_device_info() < 0)
                return -1;

        if (sfs_format_device() < 0)
                return -1;

        MSG(0, "Info: format successful\n");

        return 0;
}

