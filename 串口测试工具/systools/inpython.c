/*
 * $Id$ --
 *
 *   upgrade pysdk, pyapp and configuration pyapp 
 *
 * Copyright (c) 2001-2018 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 01/25/2018
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
#include <libgen.h>
#include <jansson.h>

#include "ih_ipc.h"
#include "ih_cmd.h"
#include "cli_api.h"
#include "shared.h"
#include "image.h"
#include "ih_ipc.h"
#include "bootenv.h"

#define PYSDK_PATH		  "/var/pycore"
#define PYAPP_PATH 		  "/var/app"
#define PYAPP_CFG  		  PYAPP_PATH"/cfg"
#define DATAPATH		  "/var/python/data"
#define LOGPATH			  "/var/python/log"

#ifndef OPENDEVICE_OLD_PYSDK
#define PYSDK_ZIP_FILE 		"/tmp/pysdk.zip"
#define PYSDK_UPGRADE_BIN	"/usr/bin/pysdk-install"
#define PYAPP_UPGRADE_BIN	"/usr/bin/pyapp-install"
#endif

IH_SVC_ID gl_my_svc_id = IH_SVC_INPYTHON;
BOOL    gl_boot = FALSE;
pid_t g_child_pid = -1; //pid for programs excuted by cli
char g_shutdown_flag = 0;


static void usage(void)
{
	printf("Usage: inpython [-p <pysdk>] [-a <pyapp>] [-c <pyappcfg dir>] [-s <cmdfile>]\n");
	printf("	-a               upgrade python app.\n");
	printf("	-p               upgrade python sdk.\n");
	printf("	-c               import python app config.\n");
	printf("	-s               setup python app start.\n");
	printf("	-h               see help\n");

	exit(0);
}

static int dir_exists(const char *dir)
{
	DIR *dp = NULL;
	
	if(NULL == dir){
		return 0;
	}

	dp = opendir(dir);
	if(NULL == dp){
		return 0;
	}

	closedir(dp);
	return 1;
}

static int upgrade_pysdk(const char *pysdk)
{	
	int ret = 0;
	
	if(NULL == pysdk){
		ret = -1;
		goto UPSDK_ERR;
	}

	syslog(LOG_INFO, "upgrade python sdk");
	if(!f_exists(pysdk)){
		syslog(LOG_ERR, "%s not exist", pysdk);
		ret = -1;
		goto UPSDK_ERR;
	}
#ifdef OPENDEVICE_OLD_PYSDK
	if(!strstr(pysdk, ".tar.gz")){	
#else
	if(!strstr(pysdk, ".zip")){
#endif
		syslog(LOG_ERR, "pysdk format err");
		ret = -1;
		goto UPSDK_ERR;
	}

#ifdef OPENDEVICE_OLD_PYSDK
	ret = eval("pybackup.sh", "-p", pysdk, "-r", "no");
#else
	eval("mv", pysdk, PYSDK_ZIP_FILE);
	ret = eval(PYSDK_UPGRADE_BIN);
#endif
UPSDK_ERR:
	if(ret){
		syslog(LOG_ERR, "upgrade python sdk failed");
	}else{
		syslog(LOG_INFO, "upgrade python sdk successfully");
	}
	return ret;
}

static int pyapp_install(const char *app)
{
#ifdef OPENDEVICE_OLD_PYSDK
	char cmd[128] = {0};
	char path[128] = {0};
	char slinkf[128] = {0};
	char dlinkf[128] = {0};
	char *p = NULL;
#endif
	char name[64] = {0};
	int ret = 0;
	
	if(NULL == app){
		return -1;
	}

#ifdef OPENDEVICE_OLD_PYSDK
	if(NULL == strstr(app, ".tar")){
		syslog(LOG_ERR, "python app format err");
		return -1;    
	}

	if(NULL == (p = basename((char *)app))){
		syslog(LOG_ERR, "get %s base name failed(%d:%s)", app, errno, strerror(errno));
		return -1;
	}
	strlcpy(name, p, sizeof(name));
	p = name;
	*(strchr(p, '.')) = '\0';
	
	syslog(LOG_INFO, "start install app %s...", name); 
	snprintf(cmd, sizeof(cmd), "cd %s && tar -xf %s", PYAPP_PATH, app);
	ret = system(cmd);
	if(ret){
		syslog(LOG_ERR, "tar %s err(%d:%s)", app, errno, strerror(errno));
		ret = -1;
		goto INSTALL_EXIT;
	}

	if(!dir_exists(DATAPATH)){
		if(mkdir(DATAPATH, 0777)){
			syslog(LOG_ERR, "creat %s failed(%d:%s)", DATAPATH, errno, strerror(errno));
			ret = -1;
			goto INSTALL_EXIT;
		}
	}
	
	snprintf(path, sizeof(path), "%s/%s", DATAPATH, name);
	if(!dir_exists(path)){
		if(mkdir(path, 0777)){
			syslog(LOG_ERR, "creat %s failed(%d:%s)", path, errno, strerror(errno));
			ret = -1;
			goto INSTALL_EXIT;
		}
	}

	if(!dir_exists(LOGPATH)){
		if(mkdir(LOGPATH, 0777)){
			syslog(LOG_ERR, "creat %s failed(%d:%s)", LOGPATH, errno, strerror(errno));
			ret = -1;
			goto INSTALL_EXIT;
		}
	}

	snprintf(path, sizeof(path), "%s/%s", LOGPATH, name);
	if(!dir_exists(path)){
		if(mkdir(path, 0777)){
			syslog(LOG_ERR, "creat %s failed(%d:%s)", path, errno, strerror(errno));
			ret = -1;
			goto INSTALL_EXIT;
		}
	}

	snprintf(path, sizeof(path), "%s/bin/%s", PYAPP_PATH, name);
	remove(path);
	snprintf(slinkf, sizeof(slinkf), "%s/%s/src/main.pyc", PYAPP_PATH, name);
	snprintf(dlinkf, sizeof(dlinkf), "%s/bin/%s", PYAPP_PATH, name);
	if(symlink(slinkf, dlinkf)){
		syslog(LOG_ERR, "creat soft link %s failed(%d:%s)", dlinkf, errno, strerror(errno));
		ret = -1;
		goto INSTALL_EXIT;
	}

	snprintf(path, sizeof(path), "%s/", PYAPP_PATH);
	chmod(path, 0777);
INSTALL_EXIT:
#else
	ret = eval(PYAPP_UPGRADE_BIN, app);
#endif
	if(ret){
		syslog(LOG_ERR, "insatll app %s failed", name);
	}else{
		syslog(LOG_INFO, "install app %s successfully", name);
	}
	return ret;
}

static int upgrade_pyapp(const char *pyapp)
{
	int ret = 0;
	struct stat sb;
	char pysdk[128] = {0};
	char pyapp_path[128] = {0};
	char pyapp_s[128] = {0};
	DIR *dp = NULL;
	struct dirent *dirp = NULL;
	
	if(NULL == pyapp){
		ret = -1;
		goto UPAPP_ERR;
	}
	
	syslog(LOG_INFO, "upgrade python app");
	snprintf(pysdk, sizeof(pysdk), "%s/bin/python", PYSDK_PATH);
	if(!f_exists(pysdk)){
		syslog(LOG_ERR, "not install PySDK");
		ret = -1;
		goto UPAPP_ERR;
	}

	if(stat(pyapp, &sb) == -1){
		syslog(LOG_ERR, "get %s stat failed(%d:%s)", pyapp, errno, strerror(errno));
		ret = -1;
		goto UPAPP_ERR;
	}

	if((sb.st_mode & S_IFMT) == S_IFDIR){
		strlcpy(pyapp_path, pyapp, sizeof(pyapp_path));
		if(pyapp_path[strlen(pyapp_path) - 1] == '/'){
			pyapp_path[strlen(pyapp_path) - 1] = '\0';
		}
		
		dp = opendir(pyapp_path);
		if(NULL == dp){
			syslog(LOG_ERR, "open dir %s failed(%d:%s)", pyapp_path, errno, strerror(errno));
			ret = -1;
			goto UPAPP_ERR;
		}
		
		while((dirp = readdir(dp)) != NULL){
			if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")){
				continue;
			}
			
			if(!strstr(dirp->d_name, ".tar")){
				continue;
			}
			snprintf(pyapp_s, sizeof(pyapp_s), "%s/%s", pyapp_path, dirp->d_name);
			ret = pyapp_install(pyapp_s);
			if(ret){
				break;
			}
		}
		closedir(dp);
	}else if((sb.st_mode & S_IFMT) == S_IFREG){
		ret = pyapp_install(pyapp);
	}else{
		ret = -1;
	}

UPAPP_ERR:
	if(ret){
		syslog(LOG_ERR, "upgrade python app failed");
	}else{
		syslog(LOG_INFO, "upgrade python app successfully");
	}
	return ret;
}

static int import_pyapp_cfg(const char *pycfg)
{
	DIR *dp = NULL, *dp_s = NULL;
	struct dirent *dirp, *dirp_s;
	struct stat sb;
	char path_base[128] = {0};
	char path[128] = {0};
	char cmd[128] = {0};
	int ret = 0;
	char slinkf[128], dlinkf[128];
	char fname[128] = {0};
	
	if(NULL == pycfg){
		ret = -1;
		goto IMPORT_ERR;
	}

	syslog(LOG_INFO, "import python app cfg");
	if(!dir_exists(PYAPP_CFG)){
		if(mkdir(PYAPP_CFG, 0777)){
			syslog(LOG_ERR, "creat %s failed(%d:%s)", PYAPP_CFG, errno, strerror(errno));
			ret = -1;
			goto IMPORT_ERR;
		}
	}
	
	dp = opendir(pycfg);
	if(NULL == dp){
		syslog(LOG_ERR, "open dir %s failed(%d:%s)", pycfg, errno, strerror(errno));
		ret = -1;
		goto IMPORT_ERR;
	}

	strlcpy(path_base, pycfg, sizeof(path_base));
	if(path_base[strlen(path_base)-1] == '/'){
		path_base[strlen(path_base)-1] = '\0';
	}
	
	while((dirp = readdir(dp)) != NULL){
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")){
			continue;
		}
		snprintf(path, sizeof(path), "%s/%s", path_base, dirp->d_name);
		syslog(LOG_INFO, "path:%s", path);
		if(stat(path, &sb) == -1) {
			syslog(LOG_ERR, "get %s stat failed(%d:%s)", dirp->d_name, errno, strerror(errno));
			ret = -1;
			break;
		}

		if((sb.st_mode & S_IFMT) != S_IFDIR){
			continue;
		}
		
		memset(path, 0, sizeof(path));
		snprintf(path, sizeof(path), "%s/%s", PYAPP_PATH, dirp->d_name);
		if(!dir_exists(path)){
			syslog(LOG_INFO, "%s app not installed", dirp->d_name);
			ret = -1;
			break;
		}
	
		memset(path, 0, sizeof(path));
		memset(cmd, 0, sizeof(cmd));
		snprintf(path, sizeof(path), "%s/%s", PYAPP_CFG, dirp->d_name);
		if(!dir_exists(path)){
			snprintf(cmd, sizeof(cmd), "cp -rf %s/%s %s", pycfg, dirp->d_name, PYAPP_CFG);
		}else{
			snprintf(cmd, sizeof(cmd), "cp -rf %s/%s/* %s/%s", pycfg, dirp->d_name, PYAPP_CFG, dirp->d_name);	
		}
		
		if(system(cmd)){
			syslog(LOG_ERR, "copy app cfg failed(%d:%s)", errno, strerror(errno));
			ret = -1;
			break;
		}

		dp_s = opendir(path);
		if(NULL == dp_s){
			syslog(LOG_ERR, "opendir %s err(%d:%s)", path, errno, strerror(errno));
			ret = -1;
			break;
		}

		while((dirp_s = readdir(dp_s)) != NULL){
			if(!strcmp(dirp_s->d_name, ".") || !strcmp(dirp_s->d_name, "..")){
				continue;
			}

			strlcpy(fname, dirp_s->d_name, sizeof(fname));
			break;
		}
		closedir(dp_s);

		snprintf(slinkf, sizeof(slinkf), "%s/%s/%s", PYAPP_CFG, dirp->d_name, fname);
#ifdef OPENDEVICE_OLD_PYSDK
		snprintf(dlinkf, sizeof(dlinkf), "%s/%s.cfg", PYAPP_CFG, dirp->d_name);		
#else
		snprintf(dlinkf, sizeof(dlinkf), "%s/%s/%s.cfg", PYAPP_CFG, dirp->d_name, dirp->d_name);
#endif
		remove(dlinkf);
		if(symlink(slinkf, dlinkf)){
			syslog(LOG_ERR, "creat soft link %s failed(%d:%s)", dlinkf, errno, strerror(errno));
			ret = -1;
			break;
		}
	}
	closedir(dp);
IMPORT_ERR:
	if(ret){
		syslog(LOG_ERR, "import app config failed");
	}else{
		syslog(LOG_INFO, "import app config successfully");
	}
	return ret;
}

static int service_exec_cli(char *cmd, int flags, FILE *ofp)
{
	FILE *orig_ofp = NULL;
    IH_USER_INFO user;
    int orig_flag = 0;
    int ret = 0;
    char *pcmd = NULL;

	if(!cmd || !ofp){
		return 0;
	}

    orig_ofp = cli_set_output_stream(ofp);

    user = g_user_info; //save user info
    g_user_info = g_user_system_info; //run as system user
    orig_flag = cli_set_flags(flags);
    change_view(NULL, &g_view_configure, "");

    pcmd = cmd;
	pcmd = strtok(pcmd, "\r\n");
    while (pcmd && *pcmd) {
	    syslog(LOG_DEBUG, "exec cmd: %s", pcmd);
	    ret = cli_do_cmd(pcmd);
	    if (ret!=ERR_OK) {
	        break;
	    }   
	    pcmd = strtok(NULL, "\r\n");
	} 

    //restore
    cli_set_flags(orig_flag);
    g_user_info = user;
    cli_set_output_stream(orig_ofp);

    return 0;
}

static json_t *load_json(const char *file) {
    json_t *root = NULL;
    json_error_t error;

    root = json_load_file(file, 0, &error);
	if(NULL == root){
		syslog(LOG_ERR, "json error on line %d: %s\n", error.line, error.text);
	}

	return root;   
}

static void char_replace(char *str, char s, char d)
{
	char *p = NULL;
	
	if(!str){
		return;
	}

	p = str;

	while(p && *p){
		if(*p == s){
			*p = d;
		}
		p++;
	}
}

static int parse_json_and_exec(json_t *obj)
{
	int ret = 0;
	FILE *fp;
	char fname[MAX_PATH];
	int index = 0;
	size_t size = 0;
	json_t *obj_s = NULL;
	char name[64] = {0};
	BOOL enable = FALSE;
	char cmd[128] = {0};
	char cli[512] = {0};
	int logsize = 0;
	const char *key;
    json_t *value;
	
	if(NULL == obj){
		return -1;
	}

	snprintf(fname, sizeof(fname), "/tmp/cli_out_%d.txt", getpid());
	fp = fopen(fname, "w");
	if(!fp){
		syslog(LOG_ERR, "creat %s failed(%d:%s)", fname, errno, strerror(errno));
		return -1;
	}
	
	if(json_typeof(obj) != JSON_ARRAY){
		ret = 1;
		goto JSON_ERR;
	}

	size = json_array_size(obj);
	for(index = 0; index < size; index++){
		obj_s = json_array_get(obj, index);
		if(NULL == obj_s){
			ret = 2;
			goto JSON_ERR;
		}

		memset(name, 0, sizeof(name));
		memset(cmd, 0, sizeof(cmd));
		
		json_object_size(obj_s);
		json_object_foreach(obj_s, key, value){
			if(!strcmp(key, "name")){
				if(json_typeof(value) != JSON_STRING){
					ret = 3;
					goto JSON_ERR;
				}

				strlcpy(name, json_string_value(value), sizeof(name));
			}else if(!strcmp(key, "enable")){
				if(json_typeof(value) != JSON_TRUE && json_typeof(value) != JSON_FALSE){
					ret = 3;
					goto JSON_ERR;
				}

				enable = json_boolean_value(value);
			}else if(!strcmp(key, "cmd")){
				if(json_typeof(value) != JSON_STRING){
					ret = 3;
					goto JSON_ERR;
				}

				strlcpy(cmd, json_string_value(value), sizeof(cmd));
			}else if(!strcmp(key, "logsize")){
				if(json_typeof(value) != JSON_INTEGER){
					ret = 3;
					goto JSON_ERR;
				}

				logsize = json_integer_value(value);
				if(logsize < 0 || logsize > 70){
					ret = 4;
					goto JSON_ERR;
				}
			}
		}

		char_replace(cmd, ' ', '+');
		snprintf(cli, sizeof(cli), "python import web %d %s logsize %d\npython app %d %s", index+1, cmd, logsize,
																						   index+1, enable ? "on" : "off");
		printf("cli:%s\n", cli);
		service_exec_cli(cli, IH_CMD_FLAG_CLI, fp);
	}

	service_exec_cli("python enable", IH_CMD_FLAG_CLI, fp);
JSON_ERR:
	if(ret == 1){
		syslog(LOG_ERR, "start app json format err");
	}else if(ret == 2){
		syslog(LOG_ERR, "parse json err");
	}else if(ret == 3){
		syslog(LOG_ERR, "value type err");
	}else if(ret == 4){
		syslog(LOG_ERR, "logsize too big");
	}else{
		service_exec_cli("copy running-config startup-config", IH_CMD_FLAG_CLI, fp);	
	}
	fclose(fp);
	unlink(fname);
	return ret;
}

static int setup_pyapp_start(const char *config)
{
	int ret = 0;
	json_t *root = NULL;

	if(NULL == config){
		ret = -1;
		goto SETUP_ERR;
	}

	syslog(LOG_INFO, "configure the app to start");
	if(!f_exists(config)){
		syslog(LOG_ERR, "%s not exist", config);
		ret = -1;
		goto SETUP_ERR;
	}

	root = load_json(config);
	if(NULL == root){
		ret = -1;
		goto SETUP_ERR;
	}

	ret = parse_json_and_exec(root);
SETUP_ERR:
	if(root){
		json_decref(root);
	}
	if(ret){
		syslog(LOG_ERR, "configuration app failed to start");
	}else{
		syslog(LOG_INFO, "configuration app successfuly to start");
	}
	return ret;
}

static int upgrade_python_sdk(const char *pysdk)
{
	int ret = -1;

	if(pysdk){	
		if((ret = upgrade_pysdk(pysdk))){
			return ret;
		}
	}

	return ret;
}

static int upgrade_python_app(const char *pyapp)
{
	int ret = -1;

	if(pyapp){
		if((ret = upgrade_pyapp(pyapp))){
			return ret;
		}
	}

	return ret;
}

int inpython_main(int argc, char *argv[])
{
	int ret = 0;
	char *pysdk = NULL;
	char *pyapp = NULL;
	char *pyapp_cfg = NULL;
	char *pyapp_cmd = NULL;
	BOOL upgrade_pysdk_flag = FALSE;
	BOOL upgrade_pyapp_flag = FALSE;
	BOOL import_cfg_flag = FALSE;
	BOOL setup_cmd_flag = FALSE;
	int c;

	if(argc < 2){
		usage();
	}
	
	while((c = getopt(argc, argv, "a:p:c:s:i:h")) != -1){
		switch (c) {
		case 'h':
			usage();
			break;
		case 'a':
			upgrade_pyapp_flag = TRUE;
			pyapp = optarg;
			break;
		case 'p':
			upgrade_pysdk_flag = TRUE;
			pysdk = optarg;
			break;
		case 'c':
			import_cfg_flag = TRUE;
			pyapp_cfg = optarg;
			break;
		case 's':
			setup_cmd_flag = TRUE;
			pyapp_cmd = optarg;
			break;
		case 'i':
			gl_my_svc_id = atoi(optarg);
			break;
		default:
			printf("Illegal option: -%c %s\n", c, optarg ? optarg : "");
			usage();
			break;
		}
	}

	if(!pysdk && !pyapp && !pyapp_cfg && !pyapp_cmd){
		usage();
	}

	if(upgrade_pysdk_flag && upgrade_python_sdk(pysdk)){
		return -1;
	}

	if(upgrade_pyapp_flag && upgrade_python_app(pyapp)){
		return -1;
	}
	
	if(import_cfg_flag && import_pyapp_cfg(pyapp_cfg)){
		return -1;
	}

	if(setup_cmd_flag){
		ih_ipc_open(gl_my_svc_id);
		ret = setup_pyapp_start(pyapp_cmd);
		ih_ipc_close();
		return ret;
	}
	
	return 0;
}

