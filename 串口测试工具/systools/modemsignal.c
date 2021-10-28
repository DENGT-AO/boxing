/*
 * $Id$ --
 *
 *   modem signals get & set
 *
 * Copyright (c) 2001-2019 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 08/14/2019
 * Author: wucl
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int modemsignal_get(int modemfd, unsigned int *modemsignals)
{
        int ret = 0;

	if(!modemsignals){
		return -1;
	}

        ret = ioctl(modemfd,TIOCMGET,modemsignals);
        if (ret != 0) { 
		printf("ioctl(modemfd:%d,TIOCMGET,...) ret: %d, error: %d(%s)", modemfd, ret, errno, strerror(errno));
		return ret;
        }

	return 0;
}

int modemsignal_set_rts(int modemfd, int enable)
{
        int ret = 0;
	int RTS_flag;

	RTS_flag = TIOCM_RTS;	/* Modem Constant for RTS pin */

	if(enable){
		ret = ioctl(modemfd,TIOCMBIS,&RTS_flag);
	}else{
		ret = ioctl(modemfd,TIOCMBIC,&RTS_flag);
	}

        if (ret != 0) { 
		printf("ioctl(modemfd:%d,%s,RTS) ret: %d, error: %d(%s)", modemfd, enable? "TIOCMBIS": "TIOCMBIC", ret, errno, strerror(errno));
		return ret;
        }

	return 0;
}

int modemsignal_set_dtr(int modemfd, int enable)
{
        int ret = 0;
	int DTR_flag;

	DTR_flag = TIOCM_DTR;	/* Modem Constant for DTR pin */

	if(enable){
		ret = ioctl(modemfd,TIOCMBIS,&DTR_flag);
	}else{
		ret = ioctl(modemfd,TIOCMBIC,&DTR_flag);
	}

        if (ret != 0) { 
		printf("ioctl(modemfd:%d,%s,DTR) ret: %d, error: %d(%s)", modemfd, enable? "TIOCMBIS": "TIOCMBIC", ret, errno, strerror(errno));
		return ret;
        }

	return 0;
}

int modemsignal_dump(unsigned int modemsignals, unsigned int mask)
{

	//printf("%-10s: %x\n", "modemsignals", modemsignals);
	//printf("%-10s: %x\n", "mask", mask);
	if(mask & TIOCM_CAR){
		printf("%-16s: %s\n", "CD", (modemsignals & TIOCM_CAR)? "True": "False");
	}

	if(mask & TIOCM_RNG){
		printf("%-16s: %s\n", "RI", (modemsignals & TIOCM_RNG)? "True": "False");
	}

	if(mask & TIOCM_DSR){
		printf("%-16s: %s\n", "DSR", (modemsignals & TIOCM_DSR)? "True": "False");
	}

	if(mask & TIOCM_CTS){
		printf("%-16s: %s\n", "CTS", (modemsignals & TIOCM_CTS)? "True": "False");
	}

	return 0;
}

void print_modemsignal_usage(void)
{
	printf("modemsignal <dev>\n");
	printf("modemsignal <dev> cd\n");
	printf("modemsignal <dev> ri\n");
	printf("modemsignal <dev> cts\n");
	printf("modemsignal <dev> dsr\n");
	printf("modemsignal <dev> rts low|high\n");
	printf("modemsignal <dev> dtr low|high\n");

	return;
}

#ifdef STANDALONE
int main(int argc, char *argv[])
#else
int modemsignal_main(int argc, char *argv[]) 
#endif
{
	int fd,err;
	unsigned int modemsignals = 0;
	
	if (argc < 2) {
		print_modemsignal_usage();
		return 0;
	}
	
	if ((fd = open(argv[1], O_RDWR|O_NOCTTY)) == -1) {
		printf("can not open %s, error: %d(%s)", argv[1], errno, strerror(errno));
		print_modemsignal_usage();
		return -1;
	}

	if (argc == 2) {
		err = modemsignal_get(fd, &modemsignals);
		if(err){
			printf("failed to get modemsignals for %s, err: %d\n", argv[1], err);
			close(fd);
			return err;
		}

		printf("%-16s: %x\n", "modemsignals", modemsignals);
		modemsignal_dump(modemsignals, TIOCM_CAR|TIOCM_RNG|TIOCM_DSR|TIOCM_CTS);

		close(fd);
		return 0;
	}

	if (argc == 3) {
		err = modemsignal_get(fd, &modemsignals);
		if(err){
			printf("failed to get modemsignals for %s, err: %d\n", argv[1], err);
			close(fd);
			return err;
		}

		if(!strcmp(argv[2], "cd")){
			modemsignal_dump(modemsignals, TIOCM_CAR);
		}else if(!strcmp(argv[2], "ri")){
			modemsignal_dump(modemsignals, TIOCM_RNG);
		}else if(!strcmp(argv[2], "cts")){
			modemsignal_dump(modemsignals, TIOCM_CTS);
		}else if(!strcmp(argv[2], "dsr")){
			modemsignal_dump(modemsignals, TIOCM_DSR);
		}else{
			print_modemsignal_usage();
		}

		close(fd);
		return 0;
	}
	
	if (argc == 4) {
		if(!strcmp(argv[2], "rts")){
			if(!strcmp(argv[3], "high")){
				modemsignal_set_rts(fd, 1);
				getchar();
			}else if(!strcmp(argv[3], "low")){
				modemsignal_set_rts(fd, 0);
			}else{
				print_modemsignal_usage();
			}
		}else if(!strcmp(argv[2], "dtr")){
			if(!strcmp(argv[3], "high")){
				modemsignal_set_dtr(fd, 1);
				getchar();
			}else if(!strcmp(argv[3], "low")){
				modemsignal_set_dtr(fd, 0);
			}else{
				print_modemsignal_usage();
			}
		}else{
			print_modemsignal_usage();
		}

		close(fd);
		return 0;
	}

	print_modemsignal_usage();

	close(fd);
	return 0;
}

