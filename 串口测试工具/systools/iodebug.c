/*
 * $Id$ --
 *
 *   Register debug routines
 *
 * Copyright (c) 2001-2010 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 06/04/2010
 * Author: Jianliang Zhang
 *
 */
/*iodebug.c by zly*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifdef WIN32
#include <io.h>
#define ioctl
#else
#include <unistd.h>
#include <sys/ioctl.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "shared.h"

#define IODEBUG_GET 0
#define IODEBUG_PUT 1

struct debug_pack
{
	unsigned int offset;
	unsigned int value;
};

#ifdef STANDALONE
int main(int argc, char *argv[])
#else
int iodebug_main(int argc, char *argv[]) 
#endif
{
	int fd,err;
	struct debug_pack iopack;
	
	if (argc < 3) {
		printf("iodebug get/set offset [val]\n");
		printf("\noffset 	     descriptions\n"	\
				"0xffff e800	    ECC\n"	\
				"0xffff ea00	    SDRAMC\n"	\
				"0xffff ec00	    SMC\n"	\
				"0xffff ee00	    MATRIX\n"	\
				"0xffff ef10	    CCFG\n"	\
				"0xffff f000	    AIC\n"	\
				"0xffff f200	    DBGU\n"	\
				"0xffff f400	    PIOA\n"	\
				"0xffff f600	    PIOB\n"	\
				"0xffff f800	    PIOC\n"	\
				"0xffff fc00	    PMC\n"	\
				"0xffff fd00	    RSTC\n"	\
				"0xffff fd10	    SHDWC\n" \
				"0xffff fd20	    RTTC\n"	\
				"0xffff fd30	    PITC\n"	\
				"0xffff fd40	    WDTC\n"	\
				"0xffff fd50	    GPBR\n");
		return 0;
	}
	
	if ((fd = open("/dev/debug", O_RDWR|O_NONBLOCK)) == -1) {
		printf("can not open /dev/debug.\n");
		return -1;
	}
	
	if (strcasecmp(argv[1], "get") == 0) {
#ifndef WIN32
		iopack.offset = strtoul(argv[2],'\0',16);
		err = ioctl(fd, IODEBUG_GET, &iopack);
		if (err) {
			printf("IODEBUG_GET error = %d\n",err);
		} else {
			printf("offset:0x%x, value:0x%x\n",iopack.offset,iopack.value);
		}
#endif
	} else if (strcasecmp(argv[1], "set") == 0) {
		if (argc < 4) {
			printf("iodebug put offset val\n");
			close(fd);
			return 0;
		}
		
		iopack.offset = strtoul(argv[2],'\0',16);
		iopack.value = strtoul(argv[3],'\0',16);
		
		printf("offset:0x%x, value:0x%x\n",iopack.offset,iopack.value);
		err = ioctl(fd, IODEBUG_PUT, &iopack);
		if (err) {
			printf("IODEBUG_PUT error = %d\n",err);
		}
	} else {
		printf("invalid cmd\n");
	}
	
	close(fd);
	return 0;
}

