/*
 * $Id$ --
 *
 *   Mobius PYSDK installation
 *
 * Copyright (c) 2001-2012 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 09/11/2019
 * Author: zhengyb
 *
 */

#include <errno.h>
#include <wait.h>
#include <unistd.h>
#include "ih_logtrace.h"
#include "shared.h"
#include "mobius_pysdk_img.h"

extern char *optarg;

static void mobius_pysdk_install_main_help(void)
{
	printf("usage: \n");
	printf("mobius-pysdk-install [Option]\n");
	printf("  install or recover the pysdk image.\n\n");
	printf("  Options:\n");
	printf("    -h: display this message\n");
	printf("    -i <file>: install the pysdk file. The default file name is /tmp/pysdk.zip\n");
	printf("    -r: recover with the backup image\n");    
	printf("    -s: restart the python world after installing or recovering\n");
	printf("    -u: uninstall the python sdk image. This operation will not delete the app file and data.\n");
	printf("\n");
}

static int mobius_pysdk_install(char *file)
{
	int flag;
	int ret;
	char cmd[128];

	if(!file){
		return -1;
	}

	if (access(file, F_OK)) {
		syslog(LOG_ERR, "Bad zip file name: %s\n", file);
		return -1;
	}

	flag = pysdk_img_flag();
	if (flag == PYSDK_IMG_FLAG_S3) {
		/* backup the main image */
		if (!backup_pysdk_img()) {
			syslog(LOG_ERR, "Fail to backup the main pysdk image.");
			return -1;
		}
	}

	erase_pysdk_img_flag();//S0
	/*Erase 1st img partiton*/
	snprintf(cmd, sizeof(cmd), "rm -rf %s/*;sync", MAIN_PYSDK_IMG_PATH);
	system(cmd);
	set_pysdk_img_flag(PYSDK_IMG_FLAG_S1);
	/*Unzip pysdk*/
	sprintf(cmd, "cd %s;unzip -q -o %s", MAIN_PYSDK_IMG_PATH, file);
	ret = system(cmd);
	if(ret != 0){
		syslog(LOG_ERR, "unzip %s failed(%d:%s)", file, errno, strerror(errno));
		return ret;
	}
	snprintf(cmd, sizeof(cmd), "sync;rm -f %s", file);
	system(cmd);
	set_pysdk_img_flag(PYSDK_IMG_FLAG_S2);
#if 0    
	/*Verify the main image*/
	ret = verify_pysdk_img_sign(MAIN_PYSDK_IMG_PATH);
	if (!ret) {
		syslog(LOG_ERR, "");
	}
#endif

	return 0;
}

static int mobius_pysdk_img_uninstall()
{    
	char cmd[128];

	erase_pysdk_img_flag();//S0
	/*Erase 1st & 2nd img partitons*/
	snprintf(cmd, sizeof(cmd), "rm -rf %s/*;rm -rf %s/*;sync", MAIN_PYSDK_IMG_PATH, BACKUP_PYSDK_IMG_PATH);
	system(cmd);

	return 0;
}

static int mobius_pysdk_img_recover()
{
	int ret;

	ret = verify_pysdk_img_sign(BACKUP_PYSDK_IMG_PATH);
	if (!ret){
		syslog(LOG_ERR, "The backup pysdk image is broken, recover error.");
		return -1;
	}

	ret = recover_pysdk_img();
	if (!ret) {
		syslog(LOG_ERR, "recover error.");
		return -1;
	}

	return 0;
}

void notice_restart_python()
{
	char pidfile[128];

	sprintf(pidfile, "/var/run/" PYAGENT_IDENT ".pid");
	kill_pidfile(pidfile, SIGUSR2);
}

int mobius_pysdk_install_main(int argc, char *argv[])
{
	int c, ret, restart=0;
	PYSDK_IMG_CMD cmd = PYSDK_IMG_CMD_INSTALL;
	char file[128] = {0};

	if (argc < 2) {
		mobius_pysdk_install_main_help();

		return 0;
	}

	while ((c = getopt(argc, argv,"rhsi:")) != -1) {
		switch (c) {
			case 'r':
				cmd = PYSDK_IMG_CMD_RECOVER;
				break;
			case 'h':
				mobius_pysdk_install_main_help();
				return 0;
			case 'i':
				if (argc < 3){
					mobius_pysdk_install_main_help();

					return -1;
				}

				cmd = PYSDK_IMG_CMD_INSTALL;
				snprintf(file, sizeof(file), "%s", optarg);
				break;
			case 's':
				restart = 1;
				break;
			case 'u':
				cmd = PYSDK_IMG_CMD_UNINSTALL;
				break;
			default:
				printf("ignore unknown arg: -%c %s\n", c, optarg ? optarg : "");
				return -1;
		}
	}

	switch(cmd) {
		case PYSDK_IMG_CMD_UNINSTALL:
			ret = mobius_pysdk_img_uninstall();
			break;
		case PYSDK_IMG_CMD_INSTALL:
			if (file[0] == '\0'){
				snprintf(file, sizeof(file), "%s", DEFAULT_PYSDK_ZIP_FILE);
			}
			ret = mobius_pysdk_install(file);
			break;
		case PYSDK_IMG_CMD_RECOVER:
			ret = mobius_pysdk_img_recover();
			break;
		default:
			printf("Error command.\n");
			return -1;
	}

	if (ret < 0){
		printf("error\n");
		return -1;
	}

	if (restart){
		notice_restart_python();
	}

	printf("OK\n");

	return 0;
}

