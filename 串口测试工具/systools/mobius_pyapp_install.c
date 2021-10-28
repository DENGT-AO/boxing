/*
 * $Id$ --
 *
 *   Mobius pyapp installation
 *
 * Copyright (c) 2001-2012 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 09/12/2019
 * Author: zhengyb
 *
 */
#include <errno.h>
#include <wait.h>
#include <unistd.h>
#include "ih_logtrace.h"
#include "shared.h"
#include "mobius_pysdk_img.h"
#include "python_ipc.h"

extern char *optarg;

typedef enum{
    PYAPP_CMD_NULL,
    PYAPP_CMD_INSTALL,
    PYAPP_CMD_UNINSTALL,
    PYAPP_CMD_CLEAR
}PYAPP_CMD;

extern void notice_restart_python();

static void mobius_pyapp_install_main_help(void)
{
	printf("usage: \n");
	printf("mobius-pyapp-install [Option]\n");
	printf("  install or clear the pyapp image.\n\n");
	printf("  Options:\n");
	printf("    -c: clear all the pyapp enviroment. This operation will delete\
			the all the app file and data.\n");
	printf("    -h: display this message\n");
	printf("    -i <file>: install the pyapp file. No default file name.\n");    
	printf("    -s: restart the python world after installing or recovering\n");
	printf("    -u <app>: uninstall the app file\n");
	printf("\n");
}

static int pyapp_uninstall(char *app)
{
	char cmd[128];
	char filename[128];

	if ((app == NULL) || (*app == '\0')){
		return -1;
	}

	sprintf(cmd, "rm -rf %s/%s", PYUSER_APP_PATH, app);
	system(cmd);
	sprintf(filename, "%s/%s", PYUSER_BIN_PATH, app);
	remove(filename);

	sprintf(filename, "%s/bin/%s", PYUSER_APP_PATH, app);
	remove(filename);
	sprintf(cmd, "rm -rf %s/%s", PYUSER_CFG_PATH, app);
	system(cmd);
	sprintf(cmd, "rm -rf %s/%s", PYUSER_DATA_PATH, app);
	system(cmd);
	sprintf(cmd, "rm -rf %s/%s", PYUSER_DBHOME_PATH, app);
	system(cmd);
	sprintf(cmd, "rm -rf %s/%s", PYUSER_LOG_PATH, app);
	system(cmd);

	return 0;
}

static int pyapp_clear()
{
	printf("TODO\n");
	return 0;
}

static int _pyapp_verify(const char *pyappfile, const char *signfile)
{
	FILE *fp;
	char cmd[512];
	char buf[64];
	char *certs = "/tmp/rsa_pyapp_public.key";
	
	if(!pyappfile || !signfile){
		return -1;
	}
	syslog(LOG_ERR, "app file %s, signfile %s\n", pyappfile, signfile);
	if(access(pyappfile, F_OK) || access(signfile, F_OK)){
		syslog(LOG_ERR, "pyapp or sign file not exists");
		return -1;
	}

	snprintf(cmd, sizeof(cmd), "openssl2 enc -d -aes256 -pass pass:%s -in %s -out %s ", PYAPP_PW, PYAPP_RSA_PUB_KEY, certs);
	system(cmd);

	snprintf(cmd, sizeof(cmd), "openssl2 dgst -verify %s -sha1 -signature %s %s", certs, signfile, pyappfile);
	fp = popen(cmd, "r");
	if(!fp){
		syslog(LOG_ERR, "popen cmd:%s failed(%s)", cmd, strerror(errno));
		unlink(certs);
		return -1;
	}

	fgets(buf, sizeof(buf), fp);
	pclose(fp);
		
	trim_str(buf);
	unlink(certs);
	
	if(strstr(buf, "OK")){
		return 0;
	}

	return -1;
}

static int pyapp_install(char *app)
{
	char cmd[MAX_BUFF_LEN];
	char filename[128] = {0};
	char *p = NULL;
	char appname[128] = {0};
	char pyapp_file[128];
	char pyapp_info[64];
	char pyapp_sign_file[128];
	char pyapp_tmp[64];
	char pyapp[128];
	char linkfile[128];
	int ret = 0;
	char dir[128];
	char buf[128] = {0};
	FILE *fp = NULL;

	if ((app == NULL) || (*app == '\0')){
		return -1;
	}

	/* get app file name*/
	p = strrchr(app, '/');
	snprintf(filename, sizeof(filename), "%s", p ? p+1 : app);

	/* untar to /tmp/ */
	snprintf(pyapp_tmp, sizeof(pyapp_tmp), "/tmp/pyapp");
	if(access(pyapp_tmp, F_OK)){
		mkdir(pyapp_tmp, 0777);//TODO: 0777 is too high
	}
	sprintf(cmd, "cd %s;tar -xf %s;sync;rm -f %s", pyapp_tmp, app, app);

	ret = system(cmd);
	if(ret){
		syslog(LOG_ERR, "exec %s failed(%d:%s)", cmd, errno, strerror(errno));
		return -1;
	}

	/* get app name */
	snprintf(buf, sizeof(buf), "ls %s | grep info | awk -F '.' '{printf $1}'", pyapp_tmp);
	fp = popen(buf, "r");
	if (!fp) {
		syslog(LOG_ERR, "failed to popen for get app name\n");
		return -1;
	} 

	fscanf(fp, "%s", appname);
	pclose(fp);
	if (!strlen(appname)) {
		syslog(LOG_ERR, "failed to get app name\n");
		return -1;
	}

	/* get file name */
	snprintf(buf, sizeof(buf), "ls %s | grep tar.gz", pyapp_tmp);
	fp = popen(buf, "r");
	if (!fp) {
		syslog(LOG_ERR, "failed to popen for get python app install file\n");
		return -1;
	}
	fscanf(fp, "%s", filename);
	pclose(fp);
	if (!strlen(filename)) {
		syslog(LOG_ERR, "failed to get app install file\n");
		return -1;
	}

	/* verify */
	snprintf(pyapp_file, sizeof(pyapp_file), "%s/%s", pyapp_tmp, filename);
	snprintf(pyapp_info, sizeof(pyapp_info), "%s/%s.info", pyapp_tmp, appname);
	snprintf(pyapp_sign_file, sizeof(pyapp_sign_file), "%s/%s.sign", pyapp_tmp, appname);
	if(_pyapp_verify(pyapp_file, pyapp_sign_file)){
		syslog(LOG_ERR, "PyAPP %s verify failure!", pyapp_file);
		snprintf(cmd, sizeof(cmd), "rm -rf %s", pyapp_tmp);
		system(cmd);
		return -1;
	}
	printf("PyAPP %s verify OK!\n", pyapp_file);

	/* remove the old app */
	snprintf(pyapp, sizeof(pyapp), "%s/%s", PYUSER_APP_PATH, appname);
	if(!access(pyapp, F_OK)){
		eval("rm", "-rf", pyapp);	
	}

	/* 2nd time untar */
	snprintf(pyapp_file, sizeof(pyapp_file), "%s/%s", pyapp_tmp, filename);
	sprintf(cmd, "cd %s;tar -xf %s;cp -f %s/%s.info %s/%s;sync;rm -rf %s", PYUSER_APP_PATH, 
			pyapp_file, pyapp_tmp, appname, PYUSER_APP_PATH, appname, pyapp_tmp);
	ret = system(cmd);
	if(ret){
		syslog(LOG_ERR, "exec %s failed(%d:%s)", cmd, errno, strerror(errno));
		return -1;
	}

	/* check the main.pyc */
	snprintf(filename, sizeof(filename), "%s/%s/src/main.pyc", PYUSER_APP_PATH, appname);
	if(access(filename, F_OK)){
		syslog(LOG_ERR, "pyapp %s incomplete...", appname);
		snprintf(cmd, sizeof(cmd), "rm -rf %s/%s", PYUSER_APP_PATH, appname);
		system(cmd);
		return -1;
	}

	printf("create %s link file\n", filename);
	snprintf(linkfile, sizeof(linkfile), "%s/%s", PYUSER_BIN_PATH, appname);
	remove(linkfile);
	symlink(filename, linkfile);

	snprintf(cmd, sizeof(cmd), "chmod 777 %s/%s -R", PYUSER_APP_PATH, appname);	
	system(cmd);

	snprintf(cmd, sizeof(cmd), "cp -rf %s/lib/* %s/;rm -rf %s/lib", PYUSER_APP_PATH, PYUSER_LIB_PATH, PYUSER_APP_PATH);
	ret = system(cmd);
	if(ret){
		syslog(LOG_ERR, "exec %s failed(%d:%s)", cmd, errno, strerror(errno));
		return -1;
	}

	sprintf(dir, "%s/%s", PYUSER_DATA_PATH, appname);
	mkdir(dir, 0666);
	sprintf(dir, "%s/%s", PYUSER_LOG_PATH, appname);
	mkdir(dir, 0666);

	sync();

	//sprintf(cmd, "chmod 755 -R %s/*", PYAPP_PATH);
	//system(cmd);
	printf("pyapp %s install successfully\n", appname);
	printf("OK\n");

	return 0;
}

int mobius_pyapp_install_main(int argc, char *argv[])
{
    int c, ret, restart=0;
    PYAPP_CMD cmd = PYAPP_CMD_NULL;
    char file[128] = {0};

    if (argc < 2) {
        mobius_pyapp_install_main_help();
        return -1;
    }

    while ((c = getopt(argc, argv,"chsi:u:")) != -1) {
        switch (c) {
        case 'h':
                mobius_pyapp_install_main_help();
                return 0;
        case 's':
                restart = 1;
                break;
        case 'c':
                cmd = PYAPP_CMD_CLEAR;
                break;
        case 'i':
                if (argc < 3){
                    mobius_pyapp_install_main_help();
                    return -1;
                }
                cmd = PYAPP_CMD_INSTALL;
                snprintf(file, sizeof(file), "%s", optarg);
                break;
        case 'u':
                if (argc < 3){
                    mobius_pyapp_install_main_help();
                    return -1;
                }
                cmd = PYAPP_CMD_UNINSTALL;
                snprintf(file, sizeof(file), "%s", optarg);
                break;
        default:
                printf("ignore unknown arg: -%c %s\n", c, optarg ? optarg : "");
                return -1;
        }
    }

    switch(cmd) {
    case PYAPP_CMD_INSTALL:
        ret = pyapp_install(file);
        break;
    case PYAPP_CMD_UNINSTALL:
        ret = pyapp_uninstall(file);
        break;
    case PYAPP_CMD_CLEAR:
        ret = pyapp_clear();
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

