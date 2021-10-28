/*
 * $Id$ --
 *
 *   Reboot and halt cmd implementation
 *
 * Copyright (c) 2001-2010 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 06/13/2010
 * Author: Jianliang Zhang
 *
 */

#ifdef WIN32
	#define LINUX_REBOOT_CMD_RESTART	0
#else
	#include <paths.h>
	#include <termios.h>
	#include <sys/mount.h>
	#include <sys/wait.h>
	#include <sys/reboot.h>
	#include <sys/klog.h>
	#include <sys/sysinfo.h>
	#include <sys/time.h>
	#include <syslog.h>
#endif//!WIN32

#include "shared.h"
#include "build_info.h"

#ifdef STANDALONE
int main(int argc, char *argv[])
#else
int reboothalt_main(int argc, char *argv[])
#endif
{
	int n;

	int value = 0;
	int rb = (strstr(argv[0], "reboot") != NULL);

	syslog(LOG_NOTICE,"system is rebooting!");

#ifdef INHAND_VG9
	syslog(LOG_NOTICE,"Notify syswatcher!");
	killall("syswatcher", SIGUSR2);			
#endif
	log_caller_process();

#ifdef INHAND_IG974
	ledcon(LEDMAN_CMD_OFF, LED_WARN);
	ledcon(LEDMAN_CMD_OFF, LED_STATUS);
	ledcon(LEDMAN_CMD_OFF,LED_ERROR);

	ledcon(LEDMAN_CMD_OFF, LED_LEVEL0);
	ledcon(LEDMAN_CMD_OFF, LED_LEVEL1);
	ledcon(LEDMAN_CMD_OFF, LED_LEVEL2);
#ifndef INHAND_IG974 
	ledcon(LEDMAN_CMD_OFF, LED_WWAN);
	ledcon(LEDMAN_CMD_OFF, LED_GNSS);
	ledcon(LEDMAN_CMD_OFF, LED_USR1);
	ledcon(LEDMAN_CMD_OFF, LED_USR2);
#endif
	sleep(15);
	system("sync");

	syslog(LOG_INFO,"umount SD card");
	system("umount /mnt/sd 2>/dev/null");
	sleep(3);
#else
	ledcon(LEDMAN_CMD_FLASH, LED_WARN);
	ledcon(LEDMAN_CMD_OFF, LED_STATUS);
	ledcon(LEDMAN_CMD_ON,LED_ERROR);
#endif

	for(n=0; n<_NSIG; n++) signal(n, SIG_IGN);

	if(argc>1) n = atoi(argv[1]);
	else n = 0;
	if (n > 0) {
		//printf("The system will %s in %d seconds...\n", rb ? "reboot" : "shut down", n);

		//fflush(stdout);
		sleep(n);
	}

	//puts(reboot ? "Rebooting..." : "Shutting down...");
	//fflush(stdout);

	/*when reboot, save time in rtc for add random pool by zly*/
	//saveRtc();

	syslog(LOG_INFO,"Powering off the modem");

	gpio(GPIO_WRITE, GPIO_MPOWER, &value);
	sleep(8);

	/*kill watchdog for avoiding reboot blocking.*/
	syslog(LOG_INFO,"killing watchdog");
	system("killall -9 watchdog");

	//puts("Sending signal to syswatcher...\n");
	kill(1, rb ? SIGTERM : SIGQUIT);
	sleep(10);

#if (defined INHAND_IR8 || defined INHAND_IP812)
	syslog(LOG_INFO,"umount ssd");
	system("umount /mnt/ssd");
	sleep(3);
#endif
	reboot(rb ? RB_AUTOBOOT : RB_HALT_SYSTEM);
	do {
		sleep(1);
	}while(1) ;

	return 0;
}
