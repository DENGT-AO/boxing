/*
 * $Id$ --
 *
 *   MTD routines
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


#include "ih_ipc.h"
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

#ifdef WIN32
#define		O_SYNC	0
#else
#include <unistd.h>
#include <error.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <sys/sysmacros.h>
#include "mtd-user.h"
#include <stdint.h>
#endif//!WIN32

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "image.h"
#include "shared.h"

void main_help(void)
{
	printf("usage:\n");
	printf("mtd-read -f file -m mtd-name -s start -l len\n");
	printf("mtd-write -f file -m mtd-name -s start -l len\n");
	printf("mtd-erase -m mtd-name -s start -n number\n");
}

int do_mtd_write(char *file,char *mtd_name,int start,int len)
{
	int mf = -1,err = 0;
	FILE *pf = NULL;
	int wrel,n;
	char buf[2*1024];

	if (!file || !mtd_name) return ERR_INVAL;
	if (start < 0) start = 0;
	if (len <= 0) len = 0x7fffffff;

#if 0
	wait_action_idle(10);
	if (check_action() != ACT_IDLE) {
		printf("mtd is busy\n");
		return -1;
	}
	set_action(ACT_UPDATE);
#endif

	mf = part_open(mtd_name);
	if (mf < 0) {
		printf("unable to open %s\n",mtd_name);
		err = mf;
		goto do_exit;
	}

	if (file) {
		pf = fopen(file,"rb");
		if (pf == NULL) {
			printf("unable to open %s\n",file);
			err = -1;
			goto do_exit;
		}
	}

	wrel = 0;
	while (wrel < len) {
		n = fread(buf,1,sizeof(buf),pf);
		if (n <= 0) goto do_exit;
		else if(n < sizeof(buf)) memset(buf+n,0xFF,sizeof(buf)-n); /*padding 0xff*/

		start = part_write(mf,start,(const unsigned char *)buf,sizeof(buf),0);
		if (start < 0) {
			err = start;
			goto do_exit;
		}

		wrel += n;
	}
	
do_exit:
	set_action(ACT_IDLE);
	if (mf >= 0) part_close(mf);
	if (pf) fclose(pf);
	return err;	
}

int do_mtd_read(char *file,char *mtd_name,int start,int len)
{
	int mf = -1;
	int rrel,n,i,len2;
	char buf[2*1024];
	FILE *pf = NULL;
	int err = -1;

	if (mtd_name == NULL) return -1;
	if (start < 0) start = 0;
	if (len <= 0) len = sizeof(buf);

#if 0	
	wait_action_idle(10);
	if (check_action() != ACT_IDLE) {
		printf("mtd is busy\n");
		return -1;
	}
	set_action(ACT_UPDATE);
#endif

	mf = part_open(mtd_name);
	if (mf < 0) {
		printf("unable to open %s\n",mtd_name);
		err = mf;
		goto do_exit;
	}

	if (file) {
		pf = fopen(file,"wb");
		if (pf == NULL) {
			printf("unable to open %s\n",file);
			err = -1;
			goto do_exit;
		}
	}

	printf("read %s %d %d\n",mtd_name,start,len);
	rrel = 0;
	while (rrel < len) {
		len2 = min(sizeof(buf),len-rrel);
		n = part_read(mf,start,(const unsigned char *)buf,len2,0);
		if (n < 0) {
			printf("err %d\n",n);
			err = n;
			goto do_exit;
		}

		if (pf) {
			fwrite(buf,len2,1,pf);
			fflush(pf);
		} else {
			for (i=0;i<len2;i++) {
				if (i!=0&&i%32==0) printf("\n");
				printf("%02x ",buf[i]);
			}

			printf("\n");
		}

		start = n;
		rrel += len2;
	}

	err = ERR_OK;
do_exit:
	set_action(ACT_IDLE);
	if (mf >= 0) part_close(mf);
	if (pf) fclose(pf);
	return err;
}

int do_mtd_erase(char *mtd_name,int start,int len)
{
	if (start < 0) start = 0;
	if (len <= 0) len = 0x7fffffff;
	
	return part_erase2(mtd_name,start,len,0,0);
}

int mtd_main(int argc, char *argv[])
{
	char *file = NULL;
	char *mtd_name = NULL;
	int start = -1;
	int len = 0;
	int err;

	if (argc < 2) {
		main_help();
		return -1;
	}
	
	if (strstr(argv[1],"help")) {
		main_help();
		return 0;
	} else { 
		int c;

		if (argc < 3) return -1;
		
		while ((c = getopt(argc, argv,"f:m:s:l:n:")) != -1) {
			switch (c) {
			case 'f':
				file = optarg;
				break;
			case 'm':
				mtd_name = optarg;
				break;
			case 's':
				start = atoi(optarg);
				break;
			case 'l':
			case 'n':
				len = atoi(optarg);
				break;
			default:
				printf("ignore unknown arg: -%c %s\n", c, optarg ? optarg : "");
			}
		}

		if (mtd_name == NULL) {
			main_help();
			return -1;
		}

		if (strcmp(argv[0],"mtd-write") == 0) {
			err = do_mtd_write(file,mtd_name,start,len);
		} else if (strcmp(argv[0],"mtd-read") == 0) {
			err = do_mtd_read(file,mtd_name,start,len);
		} else if (strcmp(argv[0],"mtd-erase") == 0) {
			err = do_mtd_erase(mtd_name,start,len);
		} else {
			main_help();
			return -1;
		}

		if (err == 0) {
			printf("Success!\n");
		} else {
			printf("Failed:%d!\n",err);
		}
	}

	return -1;
}

