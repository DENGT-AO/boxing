/*
 * $Id$ --
 *
 *   Image burn
 *
 * Copyright (c) 2001-2012 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 28/06/2018
 * Author: qinzs
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <wait.h>
#include <openssl/sha.h>

#include "shared.h"
#include "ih_ipc.h"
#include "shared.h"
#include "ih_svcs.h"
#include "python_ipc.h"

#define PYAGENT_IDENT 	"python_agent"

#define PYSDK_ZIP_FILE 	"/tmp/pysdk.zip"
#define PYSDK1_PATH 	"/var/pysdk1"
#define PYSDK2_PATH 	"/var/pysdk2"
#define PYUPAGRADE_FLAG "/var/backups/pysdk.flag"
#define PYAPP_DATA_PATH	"/var/app/data"
#define PYAPP_LOG_PATH	"/var/app/log"
#define PYAPP_PATH		"/var/app"
#define PYAPP_BIN		PYAPP_PATH"/bin"
#define PYAPP_CFG		PYAPP_PATH"/cfg"

#define BUFSIZE	1024 * 16

void restart_python()
{
	char pidfile[128];

	sprintf(pidfile, "/var/run/" PYAGENT_IDENT ".pid");
	kill_pidfile(pidfile, SIGUSR2);
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

int pyapp_install_main(int argc, char* argv[])
{
	int ret = 0;
	char *app = NULL;
	char cmd[256];
	char filename[128];
	char linkfile[128];
	char appname[128];
	char pyapp_file[128];
	char pyapp_info[64];
	char pyapp_sign_file[128];
	char pyapp_tmp[64];
	char pyapp[128];
	char *p = NULL;
	char dir[128];

	if(argc < 2){
		syslog(LOG_ERR, "Please specify the app you want to install");
		printf("Please specify the app you want to install\n");
		return -1;
	}

	app = argv[1];
	if (access(app, F_OK) != 0) {
		syslog(LOG_ERR, "File %s is not found.", app);
		printf("Failed\n");
		return -1;
	}

	p = strrchr(app, '/');
	snprintf(appname, sizeof(appname), "%s", p ? p+1 : app);
	//Backup app name
	strlcpy(filename, appname, sizeof(filename));
	if((p = strchr(appname, '-')) || (p = strchr(appname, '.'))){
		*p = '\0';
	}

	snprintf(pyapp_tmp, sizeof(pyapp_tmp), "/tmp/pyapp");
	if(access(pyapp_tmp, F_OK)){
		mkdir(pyapp_tmp, 0777);
	}
#if (defined INHAND_IG9 || defined INHAND_IG502 || defined INHAND_IG974)
	sprintf(cmd, "cd %s;tar -xf %s;sync;rm -f %s", pyapp_tmp, app, app);
#else
	sprintf(cmd, "cd %s;tar -xzf %s;sync;rm -f %s", pyapp_tmp, app, app);
#endif

	ret = system(cmd);
	if(ret){
		syslog(LOG_ERR, "exec %s failed(%d:%s)", cmd, errno, strerror(errno));
		return -1;
	}

	snprintf(pyapp_file, sizeof(pyapp_file), "%s/%s", pyapp_tmp, filename);
	snprintf(pyapp_info, sizeof(pyapp_info), "%s/%s.info", pyapp_tmp, appname);
	snprintf(pyapp_sign_file, sizeof(pyapp_sign_file), "%s/%s.sign", pyapp_tmp, appname);
	if(_pyapp_verify(pyapp_file, pyapp_sign_file)){
		syslog(LOG_ERR, "PyAPP %s verify failure!", pyapp_file);
		snprintf(cmd, sizeof(cmd), "rm -rf %s", pyapp_tmp);
		system(cmd);
		return -1;
	}

	syslog(LOG_INFO, "PyAPP %s verify OK!", pyapp_file);

	snprintf(pyapp, sizeof(pyapp), "%s/%s", PYAPP_PATH, appname);
	if(!access(pyapp, F_OK)){
		eval("rm", "-rf", pyapp);	
	}
	
	snprintf(pyapp_file, sizeof(pyapp_file), "%s/%s", pyapp_tmp, filename);
	sprintf(cmd, "cd %s;tar -xf %s;cp -f %s/%s.info %s/%s;sync;rm -rf %s", PYAPP_PATH, pyapp_file, pyapp_tmp, appname, PYAPP_PATH, appname, pyapp_tmp);
	ret = system(cmd);
	if(ret){
		syslog(LOG_ERR, "exec %s failed(%d:%s)", cmd, errno, strerror(errno));
		return -1;
	}

	snprintf(filename, sizeof(filename), "%s/%s/src/main.pyc", PYAPP_PATH, appname);
	if(access(filename, F_OK)){
		syslog(LOG_ERR, "pyapp %s incomplete...", appname);
		snprintf(cmd, sizeof(cmd), "rm -rf %s/%s", PYAPP_PATH, appname);
		system(cmd);
		return -1;
	}

	syslog(LOG_INFO, "create %s link file", filename);
	snprintf(linkfile, sizeof(linkfile), "%s/bin/%s", PYAPP_PATH, appname);
	remove(linkfile);
	symlink(filename, linkfile);
	sprintf(dir, "%s/%s", PYAPP_DATA_PATH, appname);
	mkdir(dir, 0777);
	sprintf(dir, "%s/%s", PYAPP_LOG_PATH, appname);
	mkdir(dir, 0777);

	sprintf(cmd, "chmod 777 -R %s/*", PYAPP_PATH);
	system(cmd);
	syslog(LOG_INFO, "pyapp %s install successfully", appname);
	printf("OK");
	return 0;
}

int pyapp_uninstall_main(int argc, char* argv[])
{
	char *app = NULL;
	char cmd[128];
	char filename[128];

	if(argc < 2){
		syslog(LOG_ERR, "Please specify the app you want to uninstall");
		printf("Please specify the app you want to uninstall\n");
		return -1;
	}

	app = argv[1];
	
	sprintf(cmd, "rm -rf %s/%s", PYAPP_PATH, app);
	system(cmd);
	sprintf(filename, "%s/%s", PYAPP_BIN, app);
	remove(filename);
	sprintf(cmd, "rm -rf %s/%s", PYAPP_CFG, app);
	system(cmd);
	sprintf(cmd, "rm -rf %s/%s", PYAPP_DATA_PATH, app);
	system(cmd);
	sprintf(cmd, "rm -rf %s/%s", PYAPP_LOG_PATH, app);
	system(cmd);
	syslog(LOG_INFO, "pyapp %s uninstall ok", app);
	return 0;
}

static int write_pysdk_upgarde_flag(int partition)
{
	FILE *fp = NULL;

	fp = fopen(PYUPAGRADE_FLAG, "w");
	if(!fp){
		syslog(LOG_ERR, "write pysdk upgrade flag failed(%d:%s)", errno, strerror(errno));
		return -1;
	}

	fprintf(fp, "%d", partition);
	fflush(fp);
	fclose(fp);
	
	sync_file_write(PYUPAGRADE_FLAG);

	return 0;
}

static char * pysdk_upgrade_partition(void)
{
	FILE *fp = NULL;
	char buf[8] = {0};
	int flag = 0;

	if(access(PYUPAGRADE_FLAG, F_OK)){
		return PYSDK1_PATH;
	}

	fp = fopen(PYUPAGRADE_FLAG, "r");
	if(!fp){
		syslog(LOG_ERR, "read pysdk upgrade flag failed(%d:%s)", errno, strerror(errno));
		return NULL;
	}

	fgets(buf, sizeof(buf), fp);
	fclose(fp);

	flag = atoi(buf);
	if(flag == 1){
		return PYSDK2_PATH;
	}else if(flag == 2){
		return PYSDK1_PATH;
	}else{
		syslog(LOG_INFO, "pysdk upgrade flag invalid(%d)", flag);
		return NULL;
	}
}

/*useless*/
char *file_sha1sum(const char *filename)
{
    SHA_CTX c;
    unsigned char md[SHA_DIGEST_LENGTH];
    int fd;
    int i;
    unsigned char buf[1024 * 16];
    static char sha1_buf[128];

    if(NULL == filename){
        return sha1_buf;    
    }   

    fd = open(filename, O_RDONLY);
    if(fd < 0){ 
        goto EXIT;
    }   

    SHA1_Init(&c);
    for (;;) {
        i = read(fd, buf, sizeof(buf));
        if (i <= 0)
            break;
        SHA1_Update(&c, buf, (unsigned long)i);
    }
    SHA1_Final(&(md[0]), &c);

    for (i = 0; i < SHA_DIGEST_LENGTH; i++){
        sprintf((char *)&(sha1_buf[i * 2]), "%02X", md[i]);
    }

    close(fd);

EXIT:
    return sha1_buf;
}

/*useless*/
static int gen_pysdk_sha1sum(const char *file, char *sha1, size_t size)
{
	if(!file || !sha1){
		return -1;
	}
	
	snprintf(sha1, size, "%s", file_sha1sum(file));

	return 0;
}

static int pysdk_verify_ok(const char *path)
{
	DIR *d = NULL;
	struct dirent *dir_entry = NULL;
	char pysdk[128] = {0};
	char file[64] = {0};
	char pysdk_sign_file[64] = {0};
	
	if(NULL == path){
		return 0;
	}	

	d = opendir(path);
	if(!d){
		syslog(LOG_ERR, "open dir %s failed(%d:%s)", path, errno, strerror(errno));
		return 0;
	}

	while((dir_entry = readdir(d))){
		 if(strstr(dir_entry->d_name, ".tar.gz")){
			strlcpy(file, dir_entry->d_name, sizeof(file));
			break;
		 }
	}
	closedir(d);

	if(!file[0]){
		syslog(LOG_ERR, "not found pysdk");
		return 0;
	}

	snprintf(pysdk_sign_file, sizeof(pysdk_sign_file), "%s/pysdk.sign", path);	
	snprintf(pysdk, sizeof(pysdk), "%s/%s", path, file);
	if(_pysdk_verify(pysdk, pysdk_sign_file)){
		syslog(LOG_INFO, "PySDK %s verify failed", file);
		return 0;
	}
	
	syslog(LOG_INFO, "PySDK %s verify OK", file);
	return 1;
}

int pysdk_install_main(int argc, char* argv[])
{
	int ret;
	char cmd[128];
	char *part = NULL;

	if (access(PYSDK_ZIP_FILE, F_OK) != 0) {
		syslog(LOG_ERR, "File %s is not found.", PYSDK_ZIP_FILE);
		printf("Failed\n");
		return -1;
	}

	part = pysdk_upgrade_partition();
	if(NULL == part){
		syslog(LOG_ERR, "get pysdk upgrade partition failed");
		return -1;
	}

	sprintf(cmd, "cd %s && rm -rf `ls -1 %s | grep -v \"^lost+found$\"`", part, part);
	system(cmd);

	sprintf(cmd, "cd %s;unzip -q -o %s;sync;rm -f %s", part, PYSDK_ZIP_FILE, PYSDK_ZIP_FILE);
	ret = system(cmd);
	if(ret != 0){
		syslog(LOG_ERR, "unzip %s failed(%d:%s)", PYSDK_ZIP_FILE, errno, strerror(errno));
		return ret;
	}

	syslog(LOG_INFO, "start verify pysdk...");
	if(!pysdk_verify_ok(part)){
		sprintf(cmd, "cd %s && rm -rf `ls -1 %s | grep -v \"^lost+found$\"`", part, part);
		system(cmd);
		return -1;
	}

	if(strcmp(part, PYSDK1_PATH) == 0){
		write_pysdk_upgarde_flag(1);
	}else{
		write_pysdk_upgarde_flag(2);
	}

	//notice pyagent to restart python
	restart_python();
	printf("OK");

	return 0;
}

