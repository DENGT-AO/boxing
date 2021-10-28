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

#include "shared.h"
#include "image.h"
#include "ih_ipc.h"
#include "bootenv.h"

#ifdef INHAND_IDTU9
#include <sys/mman.h>
#include "openssl/hmac.h"
#include "openssl/sha.h"
#include "openssl/evp.h"

#define INHAND_HMAC_KEY	"AM335x_Inhand@hmac"

#define SECURITY_CHIP_FILE	"/var/tmp/SECURITY.bin"
#define SECURITY_COS_FILE	"/var/tmp/SECURITY_cos.bin"
#define SECURITY_BOOT_FILE	"/var/tmp/SECURITY_boot.bin"

#define SECURITY_BOOT_FILE_LEN	8192	//8K
#define SHA256_LEN				32
#endif

#define IR9_FIRMWARE_FILE "/var/tmp/IR9.bin"
#define IR6_FIRMWARE_FILE "/var/tmp/IR6.bin"

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
	printf("	-i               upgrade wifi board boot.\n");

	exit(0);
}

static int verify_nor_image(int fd)
{
	uint8 buf[64*1024]; /* Must 64K */
	int index, datalen, len, n;
	image_header_t image_header;	
	unsigned long newcrc;	

	/* image head */
	index = read(fd, buf, sizeof(buf));
	if (index <= 0) return -1;

	memcpy(&image_header,buf,sizeof(image_header));
	if(ntohl(image_header.ih_dcrc)==0xffffffff) return -1;

	datalen = sizeof(image_header) + ntohl(image_header.ih_size);
	if (datalen>mtd_size(fd)){
		syslog(LOG_INFO, "image size too large: %d/%d", datalen, mtd_size(fd));
		 return -1;
	}

	len = sizeof(buf);
	newcrc = crc32(0,(char *)buf+sizeof(image_header),sizeof(buf) - sizeof(image_header));
	
	for (;len < datalen;) {
		n = MIN(sizeof(buf),datalen - len);
		index = read(fd, buf, sizeof(buf));
		if (index <= 0){
			syslog(LOG_INFO, "read error index: %d", index);
			 return -1;
		}

		len += sizeof(buf);

		newcrc = crc32(newcrc,(char *)buf,n);
	}

	if (newcrc != ntohl(image_header.ih_dcrc)) {
		syslog(LOG_DEBUG, "%08lx != %08x",newcrc,ntohl(image_header.ih_dcrc));
		return -1;
	}

	return 0;
}

static int verify_nand_image(int fd)
{
	uint8 buf[64*1024]; /* Must 64K */
	int index, offs, datalen, len=0, n;
	image_header_t image_header;	
	unsigned long newcrc;	

	offs = 0;
	/* image head */
	index = part_read(fd,offs,buf,sizeof(buf),1);
	if (index <= 0) return -1;

	memcpy(&image_header,buf,sizeof(image_header));
	datalen = sizeof(image_header) + ntohl(image_header.ih_size);	

	offs = index;
	len += sizeof(buf);
	newcrc = crc32(0,(char *)buf+sizeof(image_header),sizeof(buf) - sizeof(image_header));
	
	for (;len < datalen;) {
		n = MIN(sizeof(buf),datalen - len);
		index = part_read(fd,offs,buf,sizeof(buf),1);
		if (index <= 0) return -1;

		offs = index;
		len += sizeof(buf);

		newcrc = crc32(newcrc,(char *)buf,n);
	}

	if (newcrc != ntohl(image_header.ih_dcrc)) {
		LOG_ER("crc of image data error");
		LOG_ER("%08lx != %08x",newcrc,ntohl(image_header.ih_dcrc));
		return -1;
	}

	return 0;
}

static int upgrade_wifi_board(char *file, BOOL upgrade_boot)
{
	long file_len;
	char cmd[MAX_SYS_CMD_LEN];
	char buf[MAX_BUFF_LEN];
	FILE *fp = NULL;
	int ret = -1;

	if (!ih_license_support(IH_FEATURE_WLAN_MTK)) {
		syslog(LOG_DEBUG, "Not Wifi device!");
		return 0;
	}

	if(!file || !f_exists(file)){
		syslog(LOG_DEBUG, "wifi firmware file doesn't exist");
		return 0;
	}

	file_len=f_size(file);
	//syslog(LOG_DEBUG, "Wifi firmware size[%ld]", file_len);
	if(file_len < 1024){
		syslog(LOG_INFO, "Wifi board firmware not valid!");
		return -1;
	}

	memset(cmd, 0, sizeof(cmd));
	if (upgrade_boot) {
		snprintf(cmd, sizeof(cmd), "curl -s -o reply.url -F upload=@%s http://192.168.129.234/upgrade.cgi?upgrade_boot=1", IR6_FIRMWARE_FILE);
	} else {
		snprintf(cmd, sizeof(cmd), "curl -s -o reply.url -F upload=@%s http://192.168.129.234/upgrade.cgi", IR6_FIRMWARE_FILE);
	}
	//syslog(LOG_DEBUG, "cmd[%s]", cmd);
	syslog(LOG_INFO, "begin upgrade wifi board.");
	system(cmd);

	if ((fp = fopen("reply.url", "r")) == NULL) {
		syslog(LOG_INFO, "open reply recode fail");
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	while ( fgets(buf, sizeof(buf), fp) ) {
		if (strstr(buf, "reboot")) {
			ret = 0;
			syslog(LOG_INFO, "upgrade Wifi board successfully");
			break;
		}
		memset(buf, 0, sizeof(buf));
	}

	fclose(fp);
	unlink("reply.url");

	return ret;
}

#ifdef INHAND_IDTU9
static int upgrade_security_board(char *file, BOOL upgrade_boot)
{
	int file_len, img_len;
	char cmd[MAX_SYS_CMD_LEN];
	int ret = -1;
	int imageFd;
	unsigned int hash_len = SHA256_LEN;
	unsigned char hash[EVP_MAX_MD_SIZE], hash_saved[EVP_MAX_MD_SIZE];
	void *p_map = NULL;
	void *img_start = NULL;
	off_t offset, pa_offset;

	if(!file || !f_exists(file)){
		syslog(LOG_DEBUG, "security chip firmware file doesn't exist");
		return 0;
	}

	file_len=f_size(file);
	if(file_len < 1024){
		syslog(LOG_INFO, "security chip firmware not valid!");
		return -1;
	}

	//check the availability of the firmware
	imageFd = open(file, O_RDONLY);
	if(imageFd<0) {
		syslog(LOG_ERR, "open %s failed errno %d(%s)!", file, errno, strerror(errno));
		return -1;
	}

	img_len = file_len - hash_len;
	offset = 0;
	pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);

	p_map = mmap(NULL, file_len + (offset - pa_offset), 
			PROT_READ, MAP_PRIVATE, imageFd, pa_offset);
	if(p_map == MAP_FAILED){
		syslog(LOG_ERR, "mmap failed. %s(%d)", strerror(errno), errno);
		close(imageFd);
		return -1;
	}

	img_start = p_map + offset - pa_offset;
	syslog(LOG_DEBUG, "get image len %d", img_len);
	SHA256(img_start, img_len + offset - pa_offset, hash);
	memcpy(hash_saved, img_start + img_len + offset - pa_offset, hash_len);
	munmap(p_map, file_len + offset - pa_offset);
	close(imageFd);
	syslog(LOG_DEBUG, "get security image hash OK");

	if (memcmp(hash, hash_saved, hash_len)) {
		syslog(LOG_INFO, "check security image hash failed");
		return -1;
	}

	//bootloader 8K, COS after bootloader
	if (upgrade_boot) {
		memset(cmd, 0, sizeof(cmd));
		snprintf(cmd, sizeof(cmd), "dd if=%s of=%s bs=1 count=%d",
				SECURITY_CHIP_FILE, SECURITY_BOOT_FILE, SECURITY_BOOT_FILE_LEN);
		ret = system(cmd);

		snprintf(cmd, sizeof(cmd), "pki -e -o upgrade-boot -i %s", SECURITY_BOOT_FILE);
		syslog(LOG_INFO, "begin upgrade security chip bootloader.");
		ret = system(cmd);
	} 

	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "dd if=%s of=%s bs=1 skip=%d count=%d", 
			SECURITY_CHIP_FILE, SECURITY_COS_FILE, SECURITY_BOOT_FILE_LEN, 
			file_len - SECURITY_BOOT_FILE_LEN - hash_len);
	ret = system(cmd);

	snprintf(cmd, sizeof(cmd), "pki -e -o upgrade-cos -i %s", SECURITY_COS_FILE);
	syslog(LOG_INFO, "begin upgrade security chip cos.");
	ret = system(cmd);

	return ret;
}
#endif

/* iburn srcfile */
int upgrade_main_board(char *file, BOOL upgrade_boot)
{
	int err, i;
	uint8 buf[64*1024]; /* 必须为64K */
	//int total,offs,n,datalen,index,nblock;
	int offs,n,datalen,index,nblock;
	image_header_t image_header;
	unsigned long newcrc;
	unsigned long hcrc;
	unsigned int bootflag;
	int imageFd,norFd,nandFd,bootFd,fd;
	struct stat statbuff;
	char *pbuf;

#ifdef INHAND_IDTU9
	//check sha256 HMAC
	unsigned int hmac_len;
	unsigned char hmac[EVP_MAX_MD_SIZE], hmac_saved[EVP_MAX_MD_SIZE];
	void *p_map = NULL;
	void *img_start = NULL;
	off_t offset, pa_offset;
#endif

	if(!file || !f_exists(file)){
		syslog(LOG_ERR, "firmware file doesn't exist");
		return -1;
	}

	/*
	*	0xff--have upgrade
	*	0x0 --no upgrade
	*/
	fd = part_open(BOOTFLAG_PART);
	if (fd < 0) {
		syslog(LOG_ERR, "unable to open Boot Flag");
		goto got_error;
	}
	read(fd, (char *)&bootflag, sizeof(bootflag));
	bootflag &= 0xff;
	part_close(fd);
	
	/* open source file*/
	imageFd = open(file, O_RDONLY);
	if(imageFd<0) {
		syslog(LOG_ERR, "open %s fail!", file);
		goto got_error;
	}
	fstat(imageFd, &statbuff);
	datalen = statbuff.st_size;
	/* skip uboot, read image header */
	lseek(imageFd, OS_IMG_START, SEEK_SET);
	read(imageFd, buf, sizeof(buf));
	memcpy(&image_header,buf,sizeof(image_header));
	if (ntohl(image_header.ih_magic) != IH_MAGIC) {
		syslog(LOG_ERR, "image magic error!");
		goto got_error_1;
	}

	/* check image name */
	if (strcmp((char *)image_header.ih_name, IMAGENAME)) {
		syslog(LOG_ERR, "image name error!");
		goto got_error_1;
	}

	/* check size */
	if (datalen < OS_IMG_START 
				+ ntohl(image_header.ih_size) 
				+ sizeof(image_header)) {
		syslog(LOG_ERR, "image length error,%d < %d!",
				datalen,
				OS_IMG_START + ntohl(image_header.ih_size) + sizeof(image_header));
		goto got_error_1;
	}
	//total = OS_IMG_START + sizeof(image_header) + ntohl(image_header.ih_size);
	datalen = sizeof(image_header) + ntohl(image_header.ih_size);
	
	/* check crc */
	hcrc = image_header.ih_hcrc;
	image_header.ih_hcrc = 0;
	newcrc = crc32(0,(char *)&image_header,sizeof(image_header));
	if (newcrc != ntohl(hcrc)) {
		syslog(LOG_ERR, "%08lx != %08x",newcrc,ntohl(hcrc));
		syslog(LOG_ERR, "crc of image head error");
		goto got_error_1;
	}
	image_header.ih_hcrc = hcrc;	

	printf("Upgrading .");	
	fflush(stdout);
	norFd = part_open(FIRSTOS_PART);
	if (norFd < 0) {
		syslog(LOG_ERR, "unable to open OS Kernel");
		goto got_error_1;
	}
	/* backup norflash image to nand */
	// if(bootflag==0) {//first upgrade
	if(1) {//force backup main image
		syslog(LOG_DEBUG, "begin backup main image");
		err = verify_nor_image(norFd);
		if(err < 0) {
			syslog(LOG_DEBUG, "no valid main image, skip it");
		} else {
			printf(".");
			fflush(stdout);
			nandFd = part_open(SECONDOS_PART);
			if (nandFd < 0) {
				syslog(LOG_ERR, "unable to open Second Kernel");
				goto got_error_2;
			}
			err = part_erase(nandFd,0,-1,0,1);//erase all
			if (err < 0) {
				syslog(LOG_ERR, "unable to erase Second Kernel(%d:%s)",err,strerror(err));
				goto got_error_3;
			}
			printf(".");
			fflush(stdout);
			//dump all
			offs = 0;
			lseek(norFd, 0, SEEK_SET);
			while((n=read(norFd, buf, sizeof(buf)))>0) {
				n = part_write(nandFd,offs,buf,sizeof(buf),1);
				offs = n;
			}
			//TODO:verify image
			syslog(LOG_DEBUG, "begin verify backup image");
			err = verify_nand_image(nandFd);
			if (err < 0) {
				syslog(LOG_ERR, "backup OS Kernel fail.");
				goto got_error_3;
			}
			part_close(nandFd);
		}
	}

	printf(".");
	fflush(stdout);
	/* record flag */
	syslog(LOG_DEBUG, "set boot flag 0x1");
	sync();
	printf("\nbackup main image success\n");
	err = set_bootflag(0x1);
	if(err<0) goto got_error_2;

#ifdef INHAND_IDTU9
	//check sha256 HMAC
	syslog(LOG_DEBUG, "check sha256 hmac");

	lseek(imageFd, 0, SEEK_SET);

	offset = OS_IMG_START;
	pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);

	p_map = mmap(NULL, datalen + (offset - pa_offset) + SHA256_LEN, 
			PROT_READ, MAP_PRIVATE, imageFd, pa_offset);
	if(p_map == MAP_FAILED){
		syslog(LOG_ERR, "mmap failed. %s(%d)", strerror(errno), errno);
		goto got_error_2;
	}

	img_start = p_map + offset - pa_offset;

	syslog(LOG_DEBUG, "get image datalen %d", datalen);
	HMAC(EVP_sha256(), INHAND_HMAC_KEY, sizeof(INHAND_HMAC_KEY), 
			img_start, datalen + offset - pa_offset, hmac, &hmac_len);
	memcpy(hmac_saved, img_start + datalen + offset - pa_offset, hmac_len);
	munmap(p_map, datalen + offset - pa_offset + SHA256_LEN);
	syslog(LOG_DEBUG, "get image hmac OK");

	if (memcmp(hmac, hmac_saved, hmac_len)) {
		syslog(LOG_INFO, "check image hmac failed");
		goto got_error_2;
	}

	syslog(LOG_INFO, "check image hmac OK");

#endif

	/* skip bootstrap,uboot,uboot-config */
	lseek(imageFd, OS_IMG_START, SEEK_SET);

	printf(".");	
	fflush(stdout);
	/* update norflash image */
	nblock = (datalen>>16) + 1;//64k/sector
	syslog(LOG_DEBUG, "begin erase norflash(%d sectors)", nblock);
	lseek(norFd, 0, SEEK_SET);
	err = part_erase(norFd,0,nblock,0,1);//erase nblock
	if (err < 0) {
		syslog(LOG_ERR, "unable to erase OS Kernel(%d:%s)",err,strerror(err));
		goto got_error_2;
	}
	printf(".");
	fflush(stdout);
	syslog(LOG_DEBUG, "begin write norflash");
	lseek(norFd, 0, SEEK_SET);
	index = 0;
	while((n=read(imageFd, buf, sizeof(buf)))>0) {
		index += n;
		syslog(LOG_INFO, "upgrade: %d / %d", index,datalen);
		write(norFd, buf, n);
		printf(".");	
		fflush(stdout);
	}
	part_close(norFd);
	/* copy bootstrap & uboot to Boot Loader */
	if (upgrade_boot) {
		LOG_IN("upgrade bootloader");
		bootFd = part_open(BOOTLOADER_PART_NAME);
		if (bootFd < 0) {
			LOG_ER("unable to open Boot Loader");
			goto got_error_1;
		}

		nblock = mtd_size(bootFd)/sizeof(buf) - 1;//Block size of bootloader except ENV
		part_erase(bootFd,0,nblock,0,1);

		/* size : nblock*64K */
		lseek(imageFd, 0, SEEK_SET);
		for (i=0 ; i<nblock ; i++) {
			n=read(imageFd, buf, sizeof(buf));
			write(bootFd, buf, sizeof(buf));
		}
		part_close(bootFd);

		/* set uboot version */
		n=read(imageFd, buf, sizeof(buf));

		pbuf = (char *)buf;
		pbuf += 4; /* skip crc */
		while (*pbuf && pbuf - (char *)buf < sizeof(buf)) {
			if (strncmp(pbuf,"version",strlen("version")) == 0) {
				break;
			}

			pbuf += strlen(pbuf) + 1;
		}

		if (*pbuf && pbuf - (char *)buf < sizeof(buf)) {
			bootenv_set("version",pbuf+strlen("version")+1);
		}
	}

	close(imageFd);

	printf(".");	
	fflush(stdout);
	/* record flag */
	syslog(LOG_INFO, "set boot flag 0xff");
	sync();
	err = set_bootflag(0xff);
	if(err<0) goto got_error;

	printf("\nupgrade successfully\n");
	syslog(LOG_INFO, "upgrade successfully.");
	return 0;

got_error_3:
	part_close(nandFd);
got_error_2:
	part_close(norFd);
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
		syslog(LOG_ERR, "invalid file type");
		return -1;
	}

	if ((nr = fread(type, 1, len, fp)) <= 0 ) {
		syslog(LOG_ERR, "invalid file type");
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
	pid_t pid = -1, ir6_pid = -1, sec_pid = -1;
	int ret = -1;
	int status;
	int c;
	char buf[MAX_BUFF_LEN];
	BOOL upgrade_boot = FALSE, upgrade_io_boot = FALSE;
	pid_t pid_p, py_pid = -1;
	BOOL upgrade_python = FALSE;

	if(argc>1) srcfile = argv[1];
	else {
		usage();
	}

	if (!f_exists(srcfile)) {
		printf("file doesn't exist!\n");
		return -1;
	}

	while ((c = getopt(argc, argv, "huis")) != -1) {
		switch (c) {
		case 'h':
			usage();
			break;
		case 'u':
			upgrade_boot = TRUE;
			syslog(LOG_INFO, "upgrade bootloader");
			break;
		case 'i':
			upgrade_io_boot = TRUE;
#ifdef INHAND_IDTU9
			syslog(LOG_INFO, "upgrade security chip bootloader");
#else
			syslog(LOG_INFO, "upgrade wifi board bootloader");
#endif
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
	}

	memset(buf, 0, sizeof(buf));
	if(get_file_type(srcfile, buf, sizeof(buf))){
		goto EXIT;
	}
	/*unpackage */
	memset(cmd, 0, sizeof(cmd));
	if (strstr(buf, "application/octet-stream")) {
		rename(srcfile, IR9_FIRMWARE_FILE);
	} else if (strstr(buf, "application/x-tar")) {
		snprintf(cmd, sizeof(cmd), "tar -xvf %s > /dev/null", srcfile);
		system(cmd);
		unlink(srcfile);
	} else {
		printf("invalid file type");
		goto EXIT;
	}

#ifdef INHAND_IDTU9
	pid = fork();	//fork child for upgrading security chip
	if (pid < 0) {
		syslog(LOG_ERR, "fork failed");
		goto EXIT;
	} else if (pid == 0) {	//Child
		ret = upgrade_security_board(SECURITY_CHIP_FILE, upgrade_io_boot);
		if (ret) {
			exit(EXIT_FAILURE);
		} else {
			exit(EXIT_SUCCESS);
		}
	}
	
	sec_pid = pid;

#else

	pid = fork();	//fork child for IR9 upgrade
	if (pid < 0) {
		syslog(LOG_ERR, "fork failed");
		goto EXIT;
	} else if (pid == 0) {	//Child
		ret = upgrade_wifi_board(IR6_FIRMWARE_FILE, upgrade_io_boot);
		if (ret) {
			exit(EXIT_FAILURE);
		} else {
			exit(EXIT_SUCCESS);
		}
	}
	
	ir6_pid = pid;

	pid_p = fork();
	if (pid_p < 0) {
		syslog(LOG_ERR, "fork failed");
		goto EXIT;
	} else if (pid_p == 0) {
		ret = 0;
		if (upgrade_python){
			ret = start_program(NULL, "inpython", "-p", TMP_PYTHON_SDK, "-a", TMP_PYTHON_APP, 
										"-c", TMP_PYTHON_APP_CFG, "-s", TMP_PYTHON_APP_CMD);
		}

		if(ret){
			exit(EXIT_FAILURE);
		}else{
			exit(EXIT_SUCCESS);
		}
	}

	py_pid = pid_p;
#endif
	
	ret = upgrade_main_board(IR9_FIRMWARE_FILE, upgrade_boot);
	if (ret) {
		ret = -1;
		syslog(LOG_ERR, "Upgrade IR9 failed");
	}

	while ((pid = waitpid(-1, &status, 0)) > 0
			||(pid < 0 && errno == EINTR)) {
		if (pid < 0) {
			continue;
		}

		/*FIXME*/
		if (pid == ir6_pid) {
			syslog(LOG_DEBUG, "wait child pid %d success", pid);
			ir6_pid = -1;
			if (!ret) {
				if (!WIFEXITED(status) || EXIT_SUCCESS != WEXITSTATUS(status)){
					syslog(LOG_ERR, "Upgrade Wifi board failed");
					printf("Upgrade Wifi board failed");
					ret = -2;
				}
			}
		}

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

#ifdef INHAND_IDTU9
		/*FIXME*/
		if (pid == sec_pid) {
			syslog(LOG_DEBUG, "wait child pid %d success", pid);
			ir6_pid = -1;
			if (!ret) {
				if (!WIFEXITED(status) || EXIT_SUCCESS != WEXITSTATUS(status)){
					syslog(LOG_ERR, "Upgrade security board failed");
					ret = -4;
				}
			}
		}
#endif

		if(ir6_pid == -1 && py_pid == -1 && sec_pid == -1){
			break;
		}
	}

EXIT:
	unlink(IR9_FIRMWARE_FILE);
	unlink(IR6_FIRMWARE_FILE);

#ifdef INHAND_IDTU9
	unlink(SECURITY_CHIP_FILE);
	unlink(SECURITY_COS_FILE);
	unlink(SECURITY_BOOT_FILE);
#endif

	if(upgrade_python){
		snprintf(cmd, sizeof(cmd), "rm -rf %s", TMP_PYTHON_PATH);
		system(cmd);
	}
	return ret;
}
