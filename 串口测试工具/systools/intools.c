/*
 * $Id$ --
 *
 *   Diagnose tools for manufacture testing
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

/*arm-linux-gcc -o uboottools uboottools.c -lz*/
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
//#include <zlib.h>
//#include <linux/i2c.h>
//#include <linux/i2c-dev.h>

#include "mtd-user.h"
#endif//!WIN32

#include "ih_config.h"
#include "build_info.h"
#include "shared.h"
#include "bootenv.h"

#define BUF_LEN 128

//typedef	struct myenvironment_s {
//	unsigned long	crc;		/* CRC32 over data bytes	*/
//	unsigned char	data[BOOT_ENV_DATA_SIZE]; /* Environment data		*/
//} myenv_t;

/*define a small data buf, not BOOT_ENV_DATA_SIZE*/
struct mydefaultenv_t {
	unsigned long	crc;		/* CRC32 over data bytes	*/
	unsigned char	data[1024]; /* Environment data		*/
};

static struct mydefaultenv_t defaultenv = {
	0xffffffff,
#if (defined INHAND_IG9 || defined INHAND_IG502)
	{
		"version="			"2017.01.r10211"	"\0"
		"bootargs="     "console=ttyO0,115200n8 root=/dev/ram0 rootfstype=squashfs rw rootwait" "\0"
		"bootdelay="		"3"						 "\0"
		"baudrate="			"115200"			 "\0"
		"ipaddr="			  "192.168.2.2"	 "\0"
		"serverip="			"192.168.2.1"	 "\0"
		"autoload="			"n"						 "\0"
		"netmask="			"255.255.255.0"			"\0"
		"stdin="			  "serial"				"\0"
		"stdout="			  "serial"				"\0"
		"stderr="			  "serial"				"\0"
		"fileaddr="			"800000"				"\0"
		"hardware_init=" 	"1"						"\0"
		"family_name=" 		"IG"					"\0"
#ifdef INHAND_IG9
		"model_name=" 		"902B"				"\0"
#else
		"model_name=" 		"502L"				"\0"
#endif
		"oem_name="			  "inhand"			"\0"
		"ethaddr="			  "00:18:05:A0:00:01"		"\0"
		"ethaddr2="			  "00:18:05:A0:00:02"		"\0"
		"ethaddr_re="			"00:18:05:A0:00:03"		"\0"
		"ethaddr_re1="		"00:18:05:A0:00:04"		"\0"
		"ethaddr_re2="		"00:18:05:A0:00:05"		"\0"
		"productnumber="	"EN00"				"\0"
		"serialnumber="		"00000000"		"\0"
		"bootm_size="     "0x20000000"  "\0"  
		"initrd_high="    "0xffffffff"  "\0"
		"mmcdev="         "1"           "\0"  
		"description="		"www.inhand.com.cn"		"\0"
		"\0"	/* Term. myenv_t.data with 2 NULs */
	}
#else
	{
		"version="			"1.1.3.r4103"	"\0"
		"bootargs="			"console=ttyS0,115200"	"\0"
		"bootdelay="		"0"						"\0"
		"baudrate="			"115200"				"\0"
		"ipaddr="			"192.168.2.2"			"\0"
		"serverip="			"192.168.2.1"			"\0"
		"autoload="			"n"						"\0"
		"netmask="			"255.255.255.0"			"\0"
		"stdin="			"serial"				"\0"
		"stdout="			"serial"				"\0"
		"stderr="			"serial"				"\0"
		"fileaddr="			"800000"				"\0"
		"hardware_init=" 	"1"						"\0"
		"family_name=" 		"IR"					"\0"
		"model_name=" 		"912P"					"\0"
		"oem_name="			"inhand"				"\0"
		"ethaddr="			"00:18:05:A0:00:01"		"\0"
		"ethaddr2="			"00:18:05:A0:00:02"		"\0"
		"wlanaddr="			"00:18:05:A0:00:03"		"\0"
		"productnumber="	"UE00"				"\0"
		"serialnumber="		"00000000"				"\0"
		"description="		"www.inhand.com.cn"		"\0"
		"\0"	/* Term. myenv_t.data with 2 NULs */
	}
#endif
};

#ifndef WIN32
/*define in uboot.c*/
extern int flash_erase(char *devname,int start,int count);
#endif//!WIN32

#if (defined INHAND_IR8 || defined INHAND_IP812)
#define MT7620_EEPROM_FILE	"/usr/etc/MT7620_AP_2T2R-4L_V15.BIN"
static int eeprom_get_wlanmac(char *mac)
{
	int fd;
	unsigned char buf[64];
	
	strcpy(mac, "00:18:05:00:00:03");
	
	fd = open(FACTORY_MTD, O_RDONLY);
	if (fd >= 0) {
		read(fd, buf, sizeof(buf));
		close(fd);
		sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", 
			buf[4], buf[5], buf[6], buf[7], buf[8], buf[9]);
	}

	return 0;	
}

static int init_mt7620_eeprom(char * mac)
{
	int fd;
	unsigned char buf[512];
	char tmpbuf[10];
	int i;

	/*read factory mtd*/
	fd = open(FACTORY_MTD, O_RDONLY);
	if (fd<0) {
		printf("Cannot open factory mtd\n");	
		return -1;
	}
	read(fd, buf, sizeof(buf));
	close(fd);
	for (i = 0; i < 20; i++) 
		printf("%02x ", buf[i]);
	printf("\n");
	/*no valid eeprom*/
	if (buf[0] == 0xff) {
		fd = open(MT7620_EEPROM_FILE, O_RDONLY);
		if (fd<0) {
			printf("Cannot open factory file\n");	
			return -1;
		}
		read(fd, buf, sizeof(buf));
		close(fd);
	}
	/*erase factory mtd*/
	flash_eraseall(FACTORY_MTD);
	/*write factory mtd*/
	fd = open(FACTORY_MTD, O_WRONLY);
	if (fd<0) {
		printf("Cannot open factory mtd\n");	
		return -1;
	}
	printf("Mac: %s\n", mac);
	sscanf(mac, "%02x:%02x:%02x:%02x:%02x:%02x",
		 tmpbuf, tmpbuf+1, tmpbuf+2, tmpbuf+3, tmpbuf+4, tmpbuf+5);
	buf[4] = tmpbuf[0];
	buf[5] = tmpbuf[1];
	buf[6] = tmpbuf[2];
	buf[7] = tmpbuf[3];
	buf[8] = tmpbuf[4];
	buf[9] = tmpbuf[5];
	for (i = 0; i < 20; i++) 
		printf("%02x ", buf[i]);
	printf("\n");
	write(fd, buf, sizeof(buf));
	close(fd);
	return 0;
}

#endif

#ifdef INHAND_VG9
#define WIFI0_DATA_START 0x1000
#define WIFI1_DATA_START 0x5000
#define MAC_ADDR_OFFSET  6
#define CHECKSUM_OFFSET  2
#define ART_PARTITION_SIZE  256*1024

#define QC98XX_EEPROM_SIZE_LARGEST_AR900B   12064

uint32_t QC98XX_EEPROM_SIZE_LARGEST = QC98XX_EEPROM_SIZE_LARGEST_AR900B;

uint16_t le16_to_cpu(uint16_t val)
{
        return val; //wucl: note that we are little endian.
}

int qc98xx_verify_checksum(void *eeprom)
{
    uint16_t *p_half;
    uint16_t sum = 0;  
    int i;

    printf("%s flash checksum 0x%x\n", __func__, le16_to_cpu(*((uint16_t *)eeprom + 1)));

    p_half = (uint16_t *)eeprom;
    for (i = 0; i < QC98XX_EEPROM_SIZE_LARGEST / 2; i++) {
        sum ^= le16_to_cpu(*p_half++);
    }        
    if (sum != 0xffff) {
        printf("%s error: flash checksum 0x%x, computed 0x%x \n", __func__,
                le16_to_cpu(*((uint16_t *)eeprom + 1)), sum ^ 0xFFFF);
        return -1; 
    }        
    printf("%s: flash checksum passed: 0x%4x\n", __func__, le16_to_cpu(*((uint16_t *)eeprom + 1)));
    return 0;
}

uint16_t qc98xx_calc_checksum(void *eeprom)
{
    uint16_t *p_half;
    uint16_t sum = 0;  
    int i;

    //printf("%s flash checksum 0x%x\n", __func__, le16_to_cpu(*((uint16_t *)eeprom + 1)));

    *((uint16_t *)eeprom + 1) = 0;

    p_half = (uint16_t *)eeprom;
    for (i = 0; i < QC98XX_EEPROM_SIZE_LARGEST / 2; i++) {
        sum ^= le16_to_cpu(*p_half++);
    }   

    sum ^= 0xFFFF;
    //printf("%s: calculated checksum: 0x%4x\n", __func__, sum);

    return sum;
}

int set_wifi_mac(char *mac_2g_str, char *mac_5g_str)
{
	int datalen = 0;
	int imageFd = -1;
	off_t offset = 0;
	char mac_2g[6] = {0};
	char mac_5g[6] = {0};
	uint16_t csum = 0;
	unsigned char read_buff[QC98XX_EEPROM_SIZE_LARGEST_AR900B] = {0};

	if(!mac_2g_str || !mac_5g_str){
		printf("invalid mac_2g or mac_5g\n");
		return -1;
	}

	/* open source file*/
	imageFd = open(WIFI_ART_PART, O_RDWR);
	if(imageFd<0) {
		printf("open %s failed! %s(%d)", WIFI_ART_PART, strerror(errno), errno);
		return -1;
	}

	sscanf(mac_2g_str, "%02x:%02x:%02x:%02x:%02x:%02x",
		 mac_2g, mac_2g+1, mac_2g+2, mac_2g+3, mac_2g+4, mac_2g+5);

	sscanf(mac_5g_str, "%02x:%02x:%02x:%02x:%02x:%02x",
		 mac_5g, mac_5g+1, mac_5g+2, mac_5g+3, mac_5g+4, mac_5g+5);

	offset = WIFI0_DATA_START + MAC_ADDR_OFFSET;
	lseek(imageFd, offset, SEEK_SET);
	write(imageFd, mac_2g, sizeof(mac_2g));

	offset = WIFI1_DATA_START + MAC_ADDR_OFFSET;
	lseek(imageFd, offset, SEEK_SET);
	write(imageFd, mac_5g, sizeof(mac_5g));

	offset = WIFI0_DATA_START;
	lseek(imageFd, offset, SEEK_SET);
	memset(read_buff, 0, sizeof(read_buff));
	datalen = read(imageFd, read_buff, sizeof(read_buff));
	if(datalen < 0){
		printf("read wifi0 data from %s failed! %s(%d)", WIFI_ART_PART, strerror(errno), errno);
		goto got_error_1;
	}

	//printf("wifi0 cal data read: %dbytes\n", datalen);
	csum = qc98xx_calc_checksum(read_buff);
	//printf("wifi0 data caculated checksum: %04x\n", csum);

	offset = WIFI0_DATA_START + CHECKSUM_OFFSET;
	lseek(imageFd, offset, SEEK_SET);
	datalen = write(imageFd, &csum, sizeof(csum));
	if(datalen < 0){
		printf("update wifi0 checksum to %s failed! %s(%d)", WIFI_ART_PART, strerror(errno), errno);
		goto got_error_1;
	}


	offset = WIFI1_DATA_START;
	lseek(imageFd, offset, SEEK_SET);
	memset(read_buff, 0, sizeof(read_buff));
	datalen = read(imageFd, read_buff, sizeof(read_buff));
	if(datalen < 0){
		printf("read wifi1 data from %s failed! %s(%d)", WIFI_ART_PART, strerror(errno), errno);
		goto got_error_1;
	}

	//printf("wifi1 cal data read: %dbytes\n", datalen);
	csum = qc98xx_calc_checksum(read_buff);
	//printf("wifi1 data caculated checksum: %04x\n", csum);

	offset = WIFI1_DATA_START + CHECKSUM_OFFSET;
	lseek(imageFd, offset, SEEK_SET);
	datalen = write(imageFd, &csum, sizeof(csum));
	if(datalen < 0){
		printf("update wifi1 checksum to %s failed! %s(%d)", WIFI_ART_PART, strerror(errno), errno);
		goto got_error_1;
	}

	close(imageFd);

	return 0;

got_error_1:
	close(imageFd);

	return -1;
}
#endif

/* normal user: intools
 * root user: intools -s*/
#ifdef STANDALONE
int main (int argc, char **argv)
#else
int intools_main (int argc, char **argv)
#endif
{
	int crc, datalen, defaultcrc;
	char *envptr, *dataptr;
	unsigned int datasize = BOOT_ENV_DATA_SIZE;
	FILE* fp;
	char lanmacname[] = "ethaddr=";
	char lan2macname[] = "ethaddr2=";
#ifdef INHAND_IG9
	/* real-time ethernet addresses*/
	char macaddr_re[] = "ethaddr_re=";
	char macaddr_re1[] = "ethaddr_re1=";
	char macaddr_re2[] = "ethaddr_re2=";
#endif
	char wlanmacname[] = "wlanaddr=";

#ifdef INHAND_VG9
	char wlanmacname_5g[] = "wlanaddr_5g=";
	char btmacname[] = "btaddr=";
#endif

	char productnumname[] = "productnumber=";
	char serialnumname[] = "serialnumber=";
	char descname[] = "description=";
	char hdinitname[] = "hardware_init=";
	char modelname[] = "model_name=";
	char familyname[] = "family_name=";
	char oemname[] = "oem_name=";

	char lanmacbuf[BUF_LEN] = "00:18:05:A0:00:01";
	char lan2macbuf[BUF_LEN] = "00:18:05:A0:00:02";
	char wlanmacbuf[BUF_LEN] = "00:18:05:A0:00:03";
#ifdef INHAND_VG9
	char btmacbuf[BUF_LEN] = "00:18:05:A0:00:02";
	char wlanmacbuf_5g[BUF_LEN] = "00:18:05:A0:00:04";
#endif

#ifdef INHAND_IG9
	char re_macbuf[BUF_LEN] = "00:18:05:A0:00:03";
	char re1_macbuf[BUF_LEN] = "00:18:05:A0:00:04";
	char re2_macbuf[BUF_LEN] = "00:18:05:A0:00:05";
#endif

#if (defined INHAND_IG9 || defined INHAND_IG5 || defined INHAND_IG502)
	char productnumbuf[BUF_LEN] = "EN00";
	char familynamebuf[BUF_LEN] = "IG";
#ifdef INHAND_IG9
	char modelnamebuf[BUF_LEN] = "902B";
#elif (defined INHAND_IG5)
	char modelnamebuf[BUF_LEN] = "501L";
#elif (defined INHAND_IG502)
	char modelnamebuf[BUF_LEN] = "502L";
#endif
#else
	char productnumbuf[BUF_LEN] = "UE00";
	char familynamebuf[BUF_LEN] = "IR";
	char modelnamebuf[BUF_LEN] = "912P";
#endif
	char hdinitbuf[BUF_LEN] = "0";
	char descbuf[BUF_LEN] = "www.inhand.com.cn";
	char serialnumbuf[BUF_LEN] = "00000000";
	char oemnamebuf[BUF_LEN] = "inhand";
	char tmpbuf[BUF_LEN];

	char* ptmp, *p;
	char *pbuf;
	//struct traffic_data data;

#ifdef WIN32
	fp = fopen(BOOTLOADER_PART, "rw");
#else
	fp = fopen(BOOTLOADER_PART, "r");
#endif
	if(fp == NULL){
		printf("can not open %s\n", BOOTLOADER_PART);
		return -1;
	}
	
	envptr = malloc(BOOT_ENV_SIZE);
	if(envptr == NULL) {
		fclose(fp);
		printf("can not alloc mem for uboot cfg\n");
		return -1;
	}
	
	dataptr = envptr + BOOT_ENV_HEADER_SIZE;
	memset(dataptr, '\0', datasize);
	fseek(fp, CFG_ENV_OFFSET, SEEK_SET);
	fread(envptr, 1, BOOT_ENV_SIZE, fp);
	fclose(fp);
	
	//printf("BOOT_ENV_SIZE=%x, BOOT_ENV_HEADER_SIZE=%x, CFG_ENV_OFFSET=%x\n", BOOT_ENV_SIZE, BOOT_ENV_HEADER_SIZE, CFG_ENV_OFFSET);

	defaultcrc = *((unsigned int *)envptr);
	crc = crc32(0, (char *)dataptr, datasize);
	//printf("default crc=%x, calculated crc=%x, datasize=%d\n", defaultcrc, crc, datasize);
	if(crc != defaultcrc) {
		printf("copy default uboot config\n");
		memset(dataptr, '\0', datasize);
		memcpy(envptr, (char *)&defaultenv, sizeof(defaultenv));
	}
	
	printf("欢迎使用硬件检测/配置程序\n");
	printf("############ 当前配置 ###########\n");
	ptmp = dataptr;
	/*count total len*/
	while(strlen(ptmp)>0) {
		printf("%s\n",ptmp);
		ptmp = ptmp + strlen(ptmp) +1;

		/*Fixed IG501-IR intools bug*/
		if (strlen(ptmp) > 128) {
			*ptmp = '\0';
			break;
		}
	}
	printf("############# 结束 ##############\n");
	datalen = ptmp - dataptr;

	/*malloc a buf for tmp use, copy cfg to this buf
	 * cfg list format: 
	 * 	crc checksum
	 * 	string1
	 * 	string2
	 * 	\0
	 * so len is datalen + 1	*/
	p = malloc(datalen+1);
	if(p == NULL) {
		printf("can not alloc mem\n");
		free(envptr);
		return -1;
	}
	ptmp = p;
	memcpy(ptmp, dataptr, datalen+1);
	
	/*clear old data zone*/
	//memset(dataptr, '\0', datalen+1);
	/*must memset 0 in all bootenv*/
	memset(dataptr, '\0', BOOT_ENV_SIZE - BOOT_ENV_HEADER_SIZE);
	pbuf = dataptr;
	while(strlen(ptmp)>0) {
		//printf("%s, %d\n",ptmp, ptmp-p);
#ifdef INHAND_IG9
		if(strncmp(ptmp,macaddr_re,strlen(macaddr_re))==0) {
			strcpy(re_macbuf,ptmp+strlen(macaddr_re));
		} 
		else if(strncmp(ptmp,macaddr_re1,strlen(macaddr_re1))==0) {
			strcpy(re1_macbuf,ptmp+strlen(macaddr_re1));
		} 
		else if(strncmp(ptmp,macaddr_re2,strlen(macaddr_re2))==0) {
			strcpy(re2_macbuf,ptmp+strlen(macaddr_re2));
		}
#elif defined(INHAND_VG9)
		if(strncmp(ptmp,wlanmacname_5g,strlen(wlanmacname_5g))==0) {
			strcpy(wlanmacbuf_5g,ptmp+strlen(wlanmacname_5g));
		}
		else if(strncmp(ptmp,wlanmacname,strlen(wlanmacname))==0) {
			strcpy(wlanmacbuf,ptmp+strlen(wlanmacname));
		} 
		else if(strncmp(ptmp,btmacname,strlen(btmacname))==0) {
			strcpy(btmacbuf,ptmp+strlen(btmacname));
		}
#else
		if(strncmp(ptmp,wlanmacname,strlen(wlanmacname))==0) {
			strcpy(wlanmacbuf,ptmp+strlen(wlanmacname));
		}
#endif

		else if(strncmp(ptmp,lanmacname,strlen(lanmacname))==0) {
			strcpy(lanmacbuf,ptmp+strlen(lanmacname));
		}
		else if(strncmp(ptmp,lan2macname,strlen(lan2macname))==0) {
			strcpy(lan2macbuf,ptmp+strlen(lan2macname));
		}
		else if(strncmp(ptmp,productnumname,strlen(productnumname))==0) {
			strcpy(productnumbuf,ptmp+strlen(productnumname));
		}
		else if(strncmp(ptmp,serialnumname,strlen(serialnumname))==0) {
			strcpy(serialnumbuf,ptmp+strlen(serialnumname));
		}
		else if(strncmp(ptmp,descname,strlen(descname))==0) {
			strcpy(descbuf,ptmp+strlen(descname));
		}
		else if(strncmp(ptmp,hdinitname,strlen(hdinitname))==0) {
			strcpy(hdinitbuf,ptmp+strlen(hdinitname));
		}
		else if(strncmp(ptmp,familyname,strlen(familyname))==0) {
			strcpy(familynamebuf,ptmp+strlen(familyname));
		}
		else if(strncmp(ptmp,modelname,strlen(modelname))==0) {
			strcpy(modelnamebuf,ptmp+strlen(modelname));
		}
		else if(strncmp(ptmp,oemname,strlen(oemname))==0) {
			strcpy(oemnamebuf,ptmp+strlen(oemname));
		}
		else{
			/*record item which not match*/
			strcpy(pbuf, ptmp);
			pbuf += strlen(pbuf) + 1;
		}
		ptmp += strlen(ptmp) +1;
	}
	free(p);
//	printf("%s,%s,%s,%s\n",wanmacbuf,lanmacbuf,serialnumbuf,descbuf);

	printf("配置产品信息:\n");
	printf("LAN MAC[%s]: ",lanmacbuf);
	fgets(tmpbuf,BUF_LEN, stdin);
	if(strcmp(tmpbuf,"\n")!=0){
		tmpbuf[strlen(tmpbuf)-1] = '\0';
		strcpy(lanmacbuf,tmpbuf);
	}
	strcpy(pbuf, lanmacname);
	pbuf += strlen(pbuf);
	strcpy(pbuf, lanmacbuf);
	pbuf += strlen(pbuf) + 1;

	printf("LAN2 MAC[%s]: ",lan2macbuf);
	fgets(tmpbuf,BUF_LEN, stdin);
	if(strcmp(tmpbuf,"\n")!=0){
		tmpbuf[strlen(tmpbuf)-1] = '\0';
		strcpy(lan2macbuf,tmpbuf);
	}
	strcpy(pbuf, lan2macname);
	pbuf += strlen(pbuf);
	strcpy(pbuf, lan2macbuf);
	pbuf += strlen(pbuf) + 1;

#ifdef INHAND_VG9
	printf("BT MAC[%s]: ",btmacbuf);
	fgets(tmpbuf,BUF_LEN, stdin);
	if(strcmp(tmpbuf,"\n")!=0){
		tmpbuf[strlen(tmpbuf)-1] = '\0';
		strcpy(btmacbuf,tmpbuf);
	}
	strcpy(pbuf, btmacname);
	pbuf += strlen(pbuf);
	strcpy(pbuf, btmacbuf);
	pbuf += strlen(pbuf) + 1;
#endif

#if (defined INHAND_IR8 || defined INHAND_IP812)
	eeprom_get_wlanmac(wlanmacbuf);
#endif

#ifdef INHAND_IG9
	if(strchr(modelnamebuf, 'H') && (strstr(productnumbuf, "RE") != NULL)){
		printf("RTETH MAC[%s]: ",re_macbuf);
		fgets(tmpbuf,BUF_LEN, stdin);
		if(strcmp(tmpbuf,"\n")!=0){
			tmpbuf[strlen(tmpbuf)-1] = '\0';
			strcpy(re_macbuf,tmpbuf);
		}
		strcpy(pbuf, macaddr_re);
		pbuf += strlen(pbuf);
		strcpy(pbuf, re_macbuf);
		pbuf += strlen(pbuf) + 1;

		printf("RTETH1 MAC[%s]: ",re1_macbuf);
		fgets(tmpbuf,BUF_LEN, stdin);
		if(strcmp(tmpbuf,"\n")!=0){
			tmpbuf[strlen(tmpbuf)-1] = '\0';
			strcpy(re1_macbuf,tmpbuf);
		}
		strcpy(pbuf, macaddr_re1);
		pbuf += strlen(pbuf);
		strcpy(pbuf, re1_macbuf);
		pbuf += strlen(pbuf) + 1;

		printf("RTETH2 MAC[%s]: ",re2_macbuf);
		fgets(tmpbuf,BUF_LEN, stdin);
		if(strcmp(tmpbuf,"\n")!=0){
			tmpbuf[strlen(tmpbuf)-1] = '\0';
			strcpy(re2_macbuf,tmpbuf);
		}
		strcpy(pbuf, macaddr_re2);
		pbuf += strlen(pbuf);
		strcpy(pbuf, re2_macbuf);
		pbuf += strlen(pbuf) + 1;
	}else{/*We should write RT ethernet address back.*/
		strcpy(pbuf, macaddr_re);
		pbuf += strlen(pbuf);
		strcpy(pbuf, re_macbuf);
		pbuf += strlen(pbuf) + 1;

		strcpy(pbuf, macaddr_re1);
		pbuf += strlen(pbuf);
		strcpy(pbuf, re1_macbuf);
		pbuf += strlen(pbuf) + 1;

		strcpy(pbuf, macaddr_re2);
		pbuf += strlen(pbuf);
		strcpy(pbuf, re2_macbuf);
		pbuf += strlen(pbuf) + 1;
	}
#else
	printf("WLAN MAC[%s]: ",wlanmacbuf);
	fgets(tmpbuf,BUF_LEN, stdin);
	if(strcmp(tmpbuf,"\n")!=0){
		tmpbuf[strlen(tmpbuf)-1] = '\0';
		strcpy(wlanmacbuf,tmpbuf);
	}
	strcpy(pbuf, wlanmacname);
	pbuf += strlen(pbuf);
	strcpy(pbuf, wlanmacbuf);
	pbuf += strlen(pbuf) + 1;
#endif

#ifdef INHAND_VG9
	printf("WLAN 5G MAC[%s]: ",wlanmacbuf_5g);
	fgets(tmpbuf,BUF_LEN, stdin);
	if(strcmp(tmpbuf,"\n")!=0){
		tmpbuf[strlen(tmpbuf)-1] = '\0';
		strcpy(wlanmacbuf_5g,tmpbuf);
	}
	strcpy(pbuf, wlanmacname_5g);
	pbuf += strlen(pbuf);
	strcpy(pbuf, wlanmacbuf_5g);
	pbuf += strlen(pbuf) + 1;
#endif

	printf("序列号[%s]: ",serialnumbuf);
	fgets(tmpbuf,BUF_LEN, stdin);
	if(strcmp(tmpbuf,"\n")!=0)
	{
		tmpbuf[strlen(tmpbuf)-1] = '\0';
		strcpy(serialnumbuf,tmpbuf);
	}
	strcpy(pbuf, serialnumname);
	pbuf += strlen(pbuf);
	strcpy(pbuf, serialnumbuf);
	pbuf += strlen(pbuf) + 1;
	
	printf("描述信息[%s]: ",descbuf);
	fgets(tmpbuf,BUF_LEN, stdin);
	if(strcmp(tmpbuf,"\n")!=0)
	{
		tmpbuf[strlen(tmpbuf)-1] = '\0';
		strcpy(descbuf,tmpbuf);
	}
	strcpy(pbuf, descname);
	pbuf += strlen(pbuf);
	strcpy(pbuf, descbuf);
	pbuf += strlen(pbuf) + 1;
	
	/*only for root user!*/
	//shandy: allow for all user!
	//if(argc >1) {
	if(1) {
		printf("定型标记(hardware_init)[%s]: ",hdinitbuf);
		fgets(tmpbuf,BUF_LEN, stdin);
		if(strcmp(tmpbuf,"\n")!=0) {
			tmpbuf[strlen(tmpbuf)-1] = '\0';
			strcpy(hdinitbuf,tmpbuf);
		}
		strcpy(pbuf, hdinitname);
		pbuf += strlen(pbuf);
		strcpy(pbuf, hdinitbuf);
		pbuf += strlen(pbuf) + 1;
		
		printf("型号(family_name)[%s]: ", familynamebuf);
		fgets(tmpbuf,BUF_LEN, stdin);
		if(strcmp(tmpbuf,"\n")!=0) {
			tmpbuf[strlen(tmpbuf)-1] = '\0';
			strcpy(familynamebuf,tmpbuf);
		}
		strcpy(pbuf, familyname);
		pbuf += strlen(pbuf);
		strcpy(pbuf, familynamebuf);
		pbuf += strlen(pbuf) + 1;
		
		printf("型号(model_name)[%s]: ", modelnamebuf);
		fgets(tmpbuf,BUF_LEN, stdin);
		if(strcmp(tmpbuf,"\n")!=0) {
			tmpbuf[strlen(tmpbuf)-1] = '\0';
			strcpy(modelnamebuf,tmpbuf);
		}
		strcpy(pbuf, modelname);
		pbuf += strlen(pbuf);
		strcpy(pbuf, modelnamebuf);
		pbuf += strlen(pbuf) + 1;
		
		printf("OEM名称(oem_name)[%s]: ", oemnamebuf);
		fgets(tmpbuf,BUF_LEN, stdin);
		if(strcmp(tmpbuf,"\n")!=0) {
			tmpbuf[strlen(tmpbuf)-1] = '\0';
			strcpy(oemnamebuf,tmpbuf);
		}
		strcpy(pbuf, oemname);
		pbuf += strlen(pbuf);
		strcpy(pbuf, oemnamebuf);
		pbuf += strlen(pbuf) + 1;

		printf("Product Number[%s]: ",productnumbuf);
		fgets(tmpbuf,BUF_LEN, stdin);
		if(strcmp(tmpbuf,"\n")!=0) {
			tmpbuf[strlen(tmpbuf)-1] = '\0';
			strcpy(productnumbuf,tmpbuf);
		}
		strcpy(pbuf, productnumname);
		pbuf += strlen(pbuf);
		strcpy(pbuf, productnumbuf);
		pbuf += strlen(pbuf) + 1;
	}
	
/*for debug
	ptmp = dataptr;
	while(strlen(ptmp)>0)
	{
		printf("%s\n",ptmp);
		ptmp = ptmp + strlen(ptmp) +1;
	}
*/
	/*save new crc*/
	crc = crc32(0, (char *)dataptr, datasize);
	*((unsigned int *)envptr) = crc;

//	printf ("new config crc=%x\n", crc);

	printf("Save Config(y/n)[n]: ");
	fgets(tmpbuf,BUF_LEN, stdin);
	if((tmpbuf[0] == 'y') || tmpbuf[0] == 'Y') {
#ifndef WIN32
#ifdef INHAND_IG9
		int bootenv_fd = -1;

		bootenv_fd = part_open(BOOTLOADER_PART);
		if(bootenv_fd < 0){
			printf("Can not open bootloader partition. %s(%d)\n", strerror(errno), errno);
			free(envptr);
			envptr = NULL;
			return -1;
		}

		part_erase(bootenv_fd, CFG_ENV_OFFSET, BOOT_ENV_SIZE/EMMC_WRITE_BUFF_SIZE, 0, 0);
		part_close(bootenv_fd);
#else
		flash_erase(BOOTLOADER_PART, CFG_ENV_OFFSET, 1);
#endif
#endif

		fp = fopen(BOOTLOADER_PART, "w");
		if (fp == NULL) {
			printf("无法打开设备文件（/dev/mtd0），配置失败！\n");
			free(envptr);
			return -1;
		}
		
		fseek(fp, CFG_ENV_OFFSET, SEEK_SET);
		fwrite(envptr, 1, BOOT_ENV_SIZE, fp);
		fflush(fp);
		fsync(fileno(fp));
		fclose(fp);
#if (defined INHAND_IR8 || defined INHAND_IP812)
		init_mt7620_eeprom(wlanmacbuf);
#endif

#ifdef INHAND_VG9
		set_wifi_mac(wlanmacbuf, wlanmacbuf_5g);
#endif
		sync();
		printf("配置成功\n");
	} else {
		printf("取消\n");
	}
	
	free(envptr);

#if 0
	printf("测试实时时钟(y/n)[n]: ");
	printf("取消\n");
#elif 0
	/*Test RTC*/
	printf("测试实时时钟(y/n)[n]: ");
	fgets(tmpbuf,BUF_LEN, stdin);
	if((tmpbuf[0] == 'y') || tmpbuf[0] == 'Y') {
		if ((fd = open("/dev/i2c-0",O_RDWR)) < 0) {
			printf("无法打开实时时钟设备！\n");
			exit(1);
		}
	
		int addr = 0x68; /* The I2C address */
		if (ioctl(fd,I2C_SLAVE_FORCE,addr) < 0) {
			printf("检测实时时钟设备失败！\n");
			exit(1);
		}

		unsigned char cregister = 0x4; /* Device register to access */
		int res;
		char buf[10];
		buf[0] = cregister;
		buf[1] = 0x43;
		buf[2] = 0x65;
		res = i2c_smbus_read_byte_data(fd,2);
		printf("当前时间为%x分", res);
		res = i2c_smbus_read_byte_data(fd,1);
		printf("%x秒\n", res);
		sleep(3);
		res = i2c_smbus_read_byte_data(fd,2);
		printf("三秒后时间为%x分", res);
		res = i2c_smbus_read_byte_data(fd,1);
		printf("%x秒\n", res);
		close(fd);
		printf("测试完毕\n");
	}else {
		printf("取消\n");
	}
#endif

#if 0
	/*test dtu com*/
	printf("测试串口,波特率115200(y/n)[n]: ");
	fgets(tmpbuf,BUF_LEN, stdin);
	if((tmpbuf[0] == 'y') || tmpbuf[0] == 'Y') {
		system("sdebug /dev/ttyS4 115200");
	}else {
		printf("取消\n");
	}
	printf("灯全闪(y/n)[n]: ");
	fgets(tmpbuf,BUF_LEN, stdin);
	if((tmpbuf[0] == 'y') || tmpbuf[0] == 'Y') {
		system("ledcon flash status");
		system("ledcon flash warn");
		system("ledcon flash error");
		system("ledcon flash level0");
		system("ledcon flash level1");
		system("ledcon flash level2");
		
	}else {
		printf("取消\n");
	}
	/*Test beep*/
	printf("测试蜂鸣器(y/n)[n]: ");
	//printf("Test beep(y/n)[n]: ");
	fgets(tmpbuf,BUF_LEN, stdin);
	if((tmpbuf[0] == 'y') || tmpbuf[0] == 'Y') {
		system("beep beep 3");
		//sleep(1);
		//system("beep beep off");
		printf("测试完毕\n");
		//printf("Test completed\n");
	}else {
		printf("取消\n");
		//printf("Canneled\n");
	}
	/*Test leds*/
	printf("测试灯(y/n)[n]: ");
	fgets(tmpbuf,BUF_LEN, stdin);
	if((tmpbuf[0] == 'y') || tmpbuf[0] == 'Y') {
		system("ledcon off status");
		system("ledcon off warn");
		system("ledcon off error");
		system("ledcon off level0");
		system("ledcon off level1");
		system("ledcon off level2");
		//printf("status led flash...\n");
		printf("STATUS灯闪烁...\n");
		system("ledcon flash status");
		sleep(2);
		system("ledcon off status");
		//printf("warn led flash...\n");
		printf("WARN灯闪烁...\n");
		system("ledcon flash warn");
		sleep(2);
		system("ledcon off warn");
		//printf("error led flash...\n");
		printf("ERR灯闪烁...\n");
		system("ledcon flash error");
		sleep(2);
		system("ledcon off error");
		//printf("level0 led flash...\n");
		printf("信号灯1闪烁...\n");
		system("ledcon flash level0");
		sleep(2);
		system("ledcon off level0");
		//printf("level1 led flash...\n");
		printf("信号灯2闪烁...\n");
		system("ledcon flash level1");
		sleep(2);
		system("ledcon off level1");
		//printf("level2 led flash...\n");
		printf("信号灯3闪烁...\n");
		system("ledcon flash level2");
		sleep(2);
		system("ledcon off level2");
		printf("测试完毕\n");
		//printf("Test completed\n");
	}else {
		printf("取消\n");
		//printf("Canneled\n");
	}

	/*System Info*/
	printf("显示系统信息(y/n)[n]: ");
	fgets(tmpbuf,BUF_LEN, stdin);
	if((tmpbuf[0] == 'y') || tmpbuf[0] == 'Y') {
		fp = fopen(BOOTFLAG_MTD, "r");
		if(fp == NULL) {
			printf("无法打开系统文件（/dev/mtd5）!\n");
		}else {
			fread(&bootflag, 1, 2, fp);
			printf("启动标记（bootflag）为 0x%x\n", bootflag);
			fclose(fp);
		}
	}else {
		printf("取消\n");
	}
#endif

	return 0;
}



