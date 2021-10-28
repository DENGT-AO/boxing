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
 * Creation Date: 06/18/2012
 * Author: Liyin Zhang
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <wait.h>
#include <sys/mman.h>

#include "shared.h"
#include "image.h"
#include "ih_ipc.h"
#include "bootenv.h"
#include "libfdt.h"

#define FIRMWARE_FILE "/var/tmp/FW.bin"

#define TMP_PATH			"/var/tmp"
#define TMP_PYTHON_PATH		TMP_PATH"/python"
#define TMP_FIRMWARE_IMAGE  TMP_PATH"/fwimage.img"
#define TMP_PYTHON_SDK		TMP_PYTHON_PATH"/PySDK/pysdk.tar.gz"
#define TMP_PYTHON_APP		TMP_PYTHON_PATH"/app"
#define TMP_PYTHON_APP_CFG  TMP_PYTHON_APP"/cfg"
#define TMP_PYTHON_APP_CMD	TMP_PYTHON_APP"/app.json"


void usage(void)
{
	printf("Usage: iburn <source file> [-u]\n");
	printf("	<source file>    image file path\n");
	printf("	-u               upgrade boot.\n");

	exit(0);
}

void dump_fdt_header(struct fdt_header *fdt_header)
{
	if(!fdt_header){
		return;
	}

	syslog(LOG_INFO, "magic: %#x", fdt_header->magic);
	syslog(LOG_INFO, "totalsize: %#x", fdt_header->totalsize);
	syslog(LOG_INFO, "off_dt_struct: %#x", fdt_header->off_dt_struct);
	syslog(LOG_INFO, "off_dt_strings: %#x", fdt_header->off_dt_strings);
	syslog(LOG_INFO, "off_mem_rsvmap: %#x", fdt_header->off_mem_rsvmap);
	syslog(LOG_INFO, "version: %#x", fdt_header->version);
	syslog(LOG_INFO, "last_comp_version: %#x", fdt_header->last_comp_version);
	syslog(LOG_INFO, "boot_cpuid_phys: %#x", fdt_header->boot_cpuid_phys);
	syslog(LOG_INFO, "size_dt_strings: %#x", fdt_header->size_dt_strings);
	syslog(LOG_INFO, "size_dt_struct: %#x", fdt_header->size_dt_struct);
}

/* iburn srcfile */
int upgrade_main_board(char *file, BOOL upgrade_boot)
{
	int err, i;
	uint8 buf[EMMC_WRITE_BUFF_SIZE]; 
	int n,datalen,index;
	unsigned int bootflag;
	int imageFd, OsFd,bootFd,fd;
	struct stat statbuff;
	char *pbuf;
	char cmdbuf[128], mtdx[128];
	int first_flag = 1;
	unsigned int size = 0;
	unsigned int read_size = 0;
	struct fdt_header fit_image_header;
	void *p_map = NULL;
	void *img_start = NULL;
	off_t offset, pa_offset;
	unsigned int len = 0;
	int ret = -1;
	int setenv_num = 0;

	if(!file || !f_exists(file)){
		syslog(LOG_ERR, "firmware file doesn't exist");
		return -1;
	}
	
	/* open source file*/
	imageFd = open(file, O_RDONLY);
	if(imageFd<0) {
		syslog(LOG_ERR, "open %s failed! %s(%d)", file, strerror(errno), errno);
		goto got_error;
	}

	fstat(imageFd, &statbuff);
	datalen = statbuff.st_size;

	/* copy uboot to Boot Loader */
	if(upgrade_boot){
		LOG_IN("erasing bootloader...");
		snprintf(cmdbuf, sizeof(cmdbuf), "mtd-erase -m %s -s %d -n %d", BOOTLOADER_PART, BOOTLOADER_PART_START, BOOTLOADER_SIZE);
		system(cmdbuf);
		LOG_IN("bootloader is erased");

		bootFd = part_open(BOOTLOADER_PART);
		if (bootFd < 0) {
			LOG_ER("unable to open Boot Loader");
			goto got_error_1;
		}

		/* we should skip parttion table */
		lseek(imageFd, BOOTLOADER_IMG_START, SEEK_SET);

		lseek(bootFd, BOOTLOADER_PART_START, SEEK_SET);
		n = BOOTLOADER_SIZE * sizeof(buf);
		while(n > 0){
		  read_size = MIN(n, sizeof(buf));
			size = read(imageFd, buf, read_size);
			/* FIXME ? */
			write(bootFd, buf, size);
			n -= size; 	
		}
		part_close(bootFd);
	

		/* wucl: search bootenv for uboot version */
		lseek(imageFd, CFG_ENV_OFFSET, SEEK_SET);

		for(i = 0; i < BOOT_ENV_SIZE/EMMC_WRITE_BUFF_SIZE; i++){
			/* set uboot version */
			n=read(imageFd, buf, sizeof(buf));

			pbuf = (char *)buf;
			if(first_flag){
				pbuf += 4; /* skip crc */
				first_flag = 0;
			}

			while (*pbuf && pbuf - (char *)buf < sizeof(buf)) {
				if (strncmp(pbuf,"version",strlen("version")) == 0) {
					bootenv_set("version", pbuf+strlen("version")+1);
					setenv_num ++;
				}

#if 0
				if (strncmp(pbuf,"model_name",strlen("model_name")) == 0) {
					bootenv_set("model_name", pbuf+strlen("model_name")+1);
					setenv_num ++;
				}
#endif

				/* we have modified two uboot env*/
				if(setenv_num == BOOT_ENV_MODIFIED_NUM){
					break;
				}

				pbuf += strlen(pbuf) + 1;
			}

			if ((*pbuf && pbuf - (char *)buf < sizeof(buf)) 
				&& (setenv_num == BOOT_ENV_MODIFIED_NUM)) {
				break;
			}
		}
	}

	/* verify the upgrade image */
	lseek(imageFd, OS_IMG_START, SEEK_SET);
	memset(&fit_image_header, 0, sizeof(fit_image_header));
	n=read(imageFd, &fit_image_header, sizeof(fit_image_header));
	if (n < 0) {
		syslog(LOG_ERR, "read firmware image failed, errno:%d(%s)", errno, strerror(errno));
		goto got_error_1;
	}

	//dump_fdt_header(&fit_image_header);

	/* check image magic and version */
	if(fdt_check_header(&fit_image_header) != 0){
		syslog(LOG_ERR, "This upgrade Image magic and version is error.");
		goto got_error_1;
	}

	/* check size */
	len = fdt_totalsize(&fit_image_header);
	if (datalen < OS_IMG_START + len) {
		syslog(LOG_ERR, "image length error,%d < %d!", datalen, OS_IMG_START + len);
		goto got_error_1;
	}

	lseek(imageFd, 0, SEEK_SET);
	offset = OS_IMG_START;
	pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);
	//syslog(LOG_INFO, "fdt_totalsize:%lu, offset: %#x, pa_offset: %#x", len, (unsigned int)offset, (unsigned int)pa_offset);

	
	//syslog(LOG_INFO, "mmap len:%lu, offset is %#x", len + offset - pa_offset, (unsigned int)pa_offset);
	p_map = mmap(NULL, len + offset - pa_offset, PROT_READ, MAP_PRIVATE, imageFd, pa_offset);
	if(p_map == MAP_FAILED){
		syslog(LOG_ERR, "mmap failed. %s(%d)", strerror(errno), errno);
		goto got_error_1;
	}

	img_start = p_map + offset - pa_offset;

	/* check image format */
	if (!fit_check_format(img_start)){
		syslog(LOG_INFO, "Bad image format!\n");	
		munmap(p_map, len + offset - pa_offset);
		goto got_error_1;
	}

	/* check image integrity check */
	if (!fit_all_image_verify(img_start)) {
		syslog(LOG_INFO, "Image hash check error!\n");	
		munmap(p_map, len + offset - pa_offset);
		goto got_error_1;
	}

	ret = munmap(p_map, len + offset - pa_offset);
	if(ret < 0){
		syslog(LOG_ERR, "munmap failed. %s(%d)", strerror(errno), errno);
		goto got_error_1;
	}

	/*
	*	0xff--have upgrade
	*	0x0 --no upgrade
	*/
	fd = part_open(BOOTFLAG_PART);
	if (fd < 0) {
		syslog(LOG_ERR, "unable to open Boot Flag. %s(%d)", strerror(errno), errno);
		goto got_error_1;
	}

	read(fd, (char *)&bootflag, sizeof(bootflag));
	part_close(fd);
	syslog(LOG_INFO, "boot flag is %#x", bootflag);

	/*FIXME:maybe should be check CNT*/
	/*if using second now, erase fisrT, ELse erase second
	 *    31      30~8      7~0
	 *  Index    Reserved   CNT
	 *  if upgrade successfully, set (~I, R, 0xfe);
	 *  if CNT not zero, keep last mtd
	 */
	if(bootflag & 0xff){
		bootflag = ~(bootflag&0x80000000);
	}

	if(bootflag & 0x80000000) {
		snprintf(cmdbuf, sizeof(cmdbuf), "mtd-erase -m %s -s 0 -n 1024", FIRSTOS_PART);
		snprintf(mtdx, sizeof(mtdx), FIRSTOS_PART);
		bootflag = 0x7ffffffe; /*(I=0, R, 0xfe)*/
	}else {
		snprintf(cmdbuf, sizeof(cmdbuf), "mtd-erase -m %s -s 0 -n 1024", SECONDOS_PART);
		snprintf(mtdx, sizeof(mtdx), SECONDOS_PART);
		bootflag = 0xfffffffe; /*(I=1, R, 0xfe)*/
	}   

	system(cmdbuf);
	syslog(LOG_INFO, "%s, flash have erased.",cmdbuf);

	printf("Upgrading .");	
	fflush(stdout);
	syslog(LOG_INFO, "write new firmware to %s.", mtdx);
	OsFd = part_open(mtdx);
	if (OsFd < 0) {
		syslog(LOG_ERR, "unable to open %s, %s(%d)", mtdx, strerror(errno), errno);
		goto got_error_1;
	}

	printf(".");
	fflush(stdout);
	syslog(LOG_DEBUG, "begin write %s", mtdx);
	lseek(OsFd, 0, SEEK_SET);
	index = 0;

	/* skip bootstrap,uboot,uboot-config */
	lseek(imageFd, OS_IMG_START, SEEK_SET);
	while((n=read(imageFd, buf, sizeof(buf)))>0) {
		index += n;
		syslog(LOG_INFO, "upgrade: %d / %d", index, datalen);
		/*FIXME*/
		write(OsFd, buf, n);
		printf(".");	
		fflush(stdout);
		usleep(2000); //sleep 2ms
	}
	part_close(OsFd);
	close(imageFd);

	printf(".");	
	fflush(stdout);

	/* record flag */
	syslog(LOG_INFO, "set boot flag to %#x", bootflag);
	err = set_bootflag(bootflag);
	if(err<0) goto got_error;

	printf(" OK\n");
	syslog(LOG_INFO, "upgrade successfully.");
	return 0;

got_error_1:
	close(imageFd);
got_error:
	syslog(LOG_INFO, "upgrade failure.");
	return -1;
}

static int get_file_type(const char *file, char *type, unsigned int len)
{
	FILE *fp;
	char cmd[MAX_SYS_CMD_LEN] = {0};
	int nr;

	if(!file || !type){
		return -1;
	}

	if(!f_exists(file)){
		syslog(LOG_ERR, "%s not exist", file);
		return -1;
	}
	snprintf(cmd, sizeof(cmd), "file --mime-type %s", file);
	if ((fp = popen(cmd, "r")) == NULL) {
		syslog(LOG_ERR, "invalid file type, %s(%d)", strerror(errno), errno);
		return -1;
	}

	if ((nr = fread(type, 1, len, fp)) <= 0 ) {
		syslog(LOG_ERR, "read file error. %s(%d)", strerror(errno), errno);
		pclose(fp);
		return -1;
	}

	pclose(fp);
	return 0;
}

int iburn_main(int argc, char* argv[])
{
	char *srcfile = NULL;
	char cmd[MAX_SYS_CMD_LEN];
	pid_t pid = -1;
	int ret = -1;
	int status;
	int c;
	char buf[MAX_BUFF_LEN];
	BOOL upgrade_boot = FALSE;
	pid_t pid_p, py_pid = -1;
	BOOL upgrade_python = FALSE;

	if(argc>1) {
		srcfile = argv[1];
	} else {
		usage();
	}

	if (!f_exists(srcfile)) {
		printf("file doesn't exist!\n");
		return -1;
	}

	while ((c = getopt(argc, argv, "hu")) != -1) {
		switch (c) {
		case 'h':
			usage();
			break;
		case 'u':
			upgrade_boot = TRUE;
			syslog(LOG_INFO, "upgrade bootloader");
			break;
		default:
			printf("ignore unknown arg: -%c %s", c, optarg ? optarg : "");
			break;
		}
	}

	chdir("/tmp");

	if(get_file_type(srcfile, buf, sizeof(buf))){
		return -1;
	}

	if(strstr(buf, "application/zip")){
		upgrade_python = TRUE;
		snprintf(cmd, sizeof(cmd), "unzip -o %s fwimage.img", srcfile);
		if(system(cmd)){
			syslog(LOG_ERR, "unzip %s err(%d:%s)", srcfile, errno, strerror(errno));
			goto EXIT;
		}
		
		snprintf(cmd, sizeof(cmd), "unzip -o %s -x fwimage.img -d /var/app", srcfile);
		if(system(cmd)){
			syslog(LOG_ERR, "unzip %s err(%d:%s)", srcfile, errno, strerror(errno));
			goto EXIT;
		}
		unlink(srcfile);
		snprintf(cmd, sizeof(cmd), "mv /var/app/python /var/tmp/");
		system(cmd);
		srcfile = TMP_FIRMWARE_IMAGE;

		memset(buf, 0, sizeof(buf));
		if(get_file_type(srcfile, buf, sizeof(buf))){
			goto EXIT;
		}
	}

	/*unpackage */
	memset(cmd, 0, sizeof(cmd));
	if (strstr(buf, "application/octet-stream")) {
		rename(srcfile, FIRMWARE_FILE);
	} else if (strstr(buf, "application/x-tar")) {
		snprintf(cmd, sizeof(cmd), "tar -xvf %s > /dev/null", srcfile);
		system(cmd);
		unlink(srcfile);
	} else {
		printf("invalid file type");
		goto EXIT;
	}

	if(upgrade_python){
		pid_p = fork();
		if (pid_p < 0) {
			syslog(LOG_ERR, "fork failed! %s(%d)", strerror(errno), errno);
			goto EXIT;
		} else if (pid_p == 0) {
			ret = start_program(NULL, "inpython", "-p", TMP_PYTHON_SDK, "-a", TMP_PYTHON_APP, 
					"-c", TMP_PYTHON_APP_CFG, "-s", TMP_PYTHON_APP_CMD);

			if(ret){
				exit(EXIT_FAILURE);
			}else{
				exit(EXIT_SUCCESS);
			}
		}

		py_pid = pid_p;
	}

	
	ret = upgrade_main_board(FIRMWARE_FILE, upgrade_boot);
	if (ret) {
		ret = -1;
		syslog(LOG_ERR, "Upgrade firmware failed");
	}

	if(!upgrade_python){
		goto EXIT;
	}

	while ((pid = waitpid(-1, &status, 0)) > 0
			||(pid < 0 && errno == EINTR)) {
		if (pid < 0) {
			continue;
		}

		/*FIXME*/
		if (pid == py_pid){
			syslog(LOG_DEBUG, "wait child pid %d success", pid);
			py_pid = -1;
			if (!ret) {
				if (!WIFEXITED(status) || EXIT_SUCCESS != WEXITSTATUS(status)){
					syslog(LOG_ERR, "Upgrade Python failed");
					printf("Upgrade Python failed");
					ret = -3;
				}
			}
		}

		if(py_pid == -1){
			break;
		}
	}

EXIT:
	unlink(FIRMWARE_FILE);
	if(upgrade_python){
		snprintf(cmd, sizeof(cmd), "rm -rf %s", TMP_PYTHON_PATH);
		system(cmd);
	}
	return ret;
}

