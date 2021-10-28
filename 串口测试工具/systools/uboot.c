#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef WIN32
#include <direct.h>
#else
#include <unistd.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/ioctl.h>
#include "mtd-user.h"
//#include <bcmdevs.h>
//#ifdef HAVE_WIRELESS
//#include <wlutils.h>
//#endif//HAVE_WIRELESS
#endif//WIN32

#include "shared.h"
#include "ih_logtrace.h"
#include "license.h"
#include "bootenv.h"

void bootenv_usage(char *cmd)
{
	printf("usage:\n");
	printf("	%s [show]\n",cmd);
	printf("	%s get name\n",cmd);
	printf("	%s set name value\n",cmd);
}

static int do_bootenv_show(void)
{
	char *buf;
	char *pdata;
	int len;

	buf = malloc(BOOT_ENV_SIZE);
	if (buf) {
		len = bootenv_show(buf, BOOT_ENV_SIZE);
		if (len) {
			pdata = buf + 4;

			while (*pdata) {
				printf(pdata);
				printf("\n");
				pdata += strlen(pdata) + 1;
			}
		}

		free(buf);
	} else {
		printf("malloc failed\n");
	}

	return 0;
}

#ifdef STANDALONE
int main(int argc, char* argv[])
#else
int bootenv_main(int argc, char* argv[])
#endif
{
	if(argc == 1) { 
		return do_bootenv_show();
	} else if(strcmp(argv[1],"show") == 0) {
		return do_bootenv_show();
	} else if(strcmp(argv[1],"get") == 0) {
		char buf[128];
		
		if (argc < 3) {
			bootenv_usage(argv[0]);
			return -EINVAL;
		}

		if (bootenv_get(argv[2],buf,sizeof(buf)) != NULL) {
			printf(buf);
			printf("\n");
		}
		
		return 0;
	} else if(strcmp(argv[1],"set") == 0) {  
		if (argc < 4) {
			bootenv_usage(argv[0]);
			return -EINVAL;
		}

		if (bootenv_set(argv[2],argv[3]) != 0) {
			printf("Failed\n");
		}

		return 0;
	}

	bootenv_usage(argv[0]);
	return -EINVAL;
}

/****************************About CRC******************************/
/*use this crc instead of libz's crc32. refer mtd.c and uboot*/
static unsigned long *crc_table = NULL;
static void crc_done(void)
{
	free(crc_table);
	crc_table = NULL;
}
static int crc_init(void)
{
	unsigned long c, poly;
//	int i, j;
	int n, k;
	static const char p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};


	if(crc_table == NULL) {
		if((crc_table = malloc(sizeof(unsigned long)*256)) == NULL)
			return 0;
		/* make exclusive-or pattern from polynomial (0xedb88320L) */
		poly = 0L;
		for (n = 0; n < sizeof(p)/sizeof(char); n++)
			poly |= 1L << (31 - p[n]);

		for (n = 0; n < 256; n++) {
			c = (unsigned long)n;
			for (k = 0; k < 8; k++)
				c = c & 1 ? poly ^ (c >> 1) : c >> 1;
			crc_table[n] = c;
		}

/*		for(i=255; i>=0; --i) {
			c = i;
			for(j=8; j>0; --j) {
				if(c & 1) c=(c>>1)^0xEDB88320L;
				else c>>=1;				
			}
			crc_table[i] = c;
		}
*/
	}
	return 1;
}
/* ========================================================================= */
#define DO1(buf) crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);
/* ========================================================================= */
static unsigned long crc_calc(unsigned long crc, char *buf, int len)
{
/*	while (len-- > 0) {
		crc = crc_table[(crc ^ *((char *)buf)) & 0xFF] ^ (crc >> 8);
		(char *)buf++;
	}
	return crc;
*/
  	crc = crc ^ 0xffffffffL;
	while (len >= 8)
	{
	      DO8(buf);
	      len -= 8;
	}
	if (len) do {
		DO1(buf);
	} while (--len);
	return crc ^ 0xffffffffL;
}

unsigned long crc32(unsigned long crc, char* buf, unsigned long len)
{
	unsigned long newcrc;

	crc_init();
	newcrc = crc_calc(crc, buf, len);
	crc_done();
	return newcrc;
}
/*************************END CRC**************************************/

/* name: name of parameter
 * return: value of parameter for success
 * 	   or NULL for failure*/
char* read_ubootcfg(char* name)
{
#ifndef WIN32
	FILE* fp;
	int crc;
	char* pos;
//	char *envptr = (char *)&myenvironment;
//	char *dataptr = envptr + BOOT_ENV_HEADER_SIZE;
	char *envptr, *dataptr;
	unsigned int datasize = BOOT_ENV_DATA_SIZE;//64k-4
	int defaultcrc;
	char* ptmp;
	static char buf[64];

	/*read cfg*/
	fp = fopen(BOOTLOADER_PART, "r");
	if(fp == NULL){
		printf("can not open /dev/mtd0\n");
		return NULL;
	}
	envptr = malloc(BOOT_ENV_SIZE);	
	if(envptr == NULL) {
		printf("alloc memory for env fail\n");	
		fclose(fp);
		return NULL;
	}	
	dataptr = envptr + BOOT_ENV_HEADER_SIZE;
	memset(dataptr,'\0',datasize);
	fseek(fp, CFG_ENV_OFFSET, SEEK_SET);
	fread(envptr, 1, BOOT_ENV_SIZE, fp);
	fclose(fp);

	/*crc32 will use libz. here I neglect it by zly.*/
	/*use my crc checksum*/
	crc_init();
	defaultcrc = *((unsigned long *)envptr);
	crc = crc_calc (0, dataptr, datasize);
	crc_done();
#if 0
	{
		FILE *fp1;
                fp1 = fopen("/var/log/env", "wb");
                if(fp1){
                        syslog(LOG_DEBUG, "save the env to the file - /var/log/env.");
                        fwrite(envptr, 1, sizeof(envptr), fp1);
                        fclose(fp1);
                }else{
                        syslog(LOG_DEBUG, "write env file failed");
		}
	}
#endif
//	printf("read_ubootcfg\n");//zy
	if(crc != defaultcrc){
		printf("uboot cfg crc error %x,%x,crc_size:%x\n", defaultcrc, crc, datasize);
		free(envptr);
		return NULL;
	}

	ptmp = dataptr;
	while(strlen(ptmp)>0){
		pos = strstr(ptmp, name);
		if(pos != 0){//find it.
			pos = index(ptmp, '=');
			snprintf(buf, sizeof(buf), "%s", pos+1);
			free(envptr);
			return buf;
		}
		ptmp = ptmp + strlen(ptmp) +1;
	}
	free(envptr);
#endif
	return NULL;
}
#if 0
void get_ubootcfg(void) 
{

	/*call it when boot and reset default*/
	license_init();
	//if (boot) license_init();
	//boot = 0;
}
#endif


#ifndef WIN32
static int region_erase(int Fd, int start, int count, int unlock, int regcount)
{
        int i, j;
        region_info_t * reginfo;

        reginfo = calloc(regcount, sizeof(region_info_t));
	if(!reginfo){
		LOG_ER("%s out of memory.", __func__);
                return 8;
	}

        for(i = 0; i < regcount; i++)
        {
                reginfo[i].regionindex = i;
		if(ioctl(Fd,MEMGETREGIONINFO,&(reginfo[i])) != 0){
			free(reginfo);
			return 8;
		} else{
			printf("Region %d is at %d of %d sector and with sector "
					"size %x\n", i, reginfo[i].offset, reginfo[i].numblocks,
					reginfo[i].erasesize);
		}
        }

        // We have all the information about the chip we need.

        for(i = 0; i < regcount; i++)
        { //Loop through the regions
                region_info_t * r = &(reginfo[i]);

                if((start >= reginfo[i].offset) &&
                                (start < (r->offset + r->numblocks*r->erasesize)))
                        break;
        }

        if(i >= regcount)
        {
                printf("Starting offset %x not within chip.\n", start);
		free(reginfo);
                return 8;
        }

        //We are now positioned within region i of the chip, so start erasing
        //count sectors from there.

        for(j = 0; (j < count)&&(i < regcount); j++)
        {
                erase_info_t erase;
                region_info_t * r = &(reginfo[i]);

                erase.start = start;
                erase.length = r->erasesize;

                if(unlock != 0)
                { //Unlock the sector first.
                        if(ioctl(Fd, MEMUNLOCK, &erase) != 0)
                        {
                                perror("\nMTD Unlock failure");
				free(reginfo);
                                return 8;
                        }
                }
                printf("\rPerforming Flash Erase of length %u at offset 0x%x",
                                erase.length, erase.start);
                fflush(stdout);
                if(ioctl(Fd, MEMERASE, &erase) != 0)
                {
                        perror("\nMTD Erase failure");
			free(reginfo);
                        return 8;
                }


                start += erase.length;
                if(start >= (r->offset + r->numblocks*r->erasesize))
                { //We finished region i so move to region i+1
                        printf("\nMoving to region %d\n", i+1);
                        i++;
                }
        }

        printf(" done\n");
	free(reginfo);

        return 0;
}
static int non_region_erase(int Fd, int start, int count, int unlock)
{
        mtd_info_t meminfo;

        if (ioctl(Fd,MEMGETINFO,&meminfo) == 0)
        {
                erase_info_t erase;

                erase.start = start;

                erase.length = meminfo.erasesize;

                for (; count > 0; count--) {
                        printf("\rPerforming Flash Erase of length %u at offset 0x%x",
                                        erase.length, erase.start);
                        fflush(stdout);

                        if(unlock != 0)
                        {
                                //Unlock the sector first.
                                printf("\rPerforming Flash unlock at offset 0x%x",erase.start);
                                if(ioctl(Fd, MEMUNLOCK, &erase) != 0)
                                {
                                        perror("\nMTD Unlock failure");
                                        return 8;
                                }
                        }

                        if (ioctl(Fd,MEMERASE,&erase) != 0)
                        {
                                perror("\nMTD Erase failure");
                                return 8;
                        }
                        erase.start += meminfo.erasesize;
                }
                printf(" done\n");
        }
        return 0;
}

int flash_erase(char *devname,int start,int count)
{
        int regcount;
        int Fd;
        int unlock =0 ;
        int res = 0;

        // Open and size the device
        if ((Fd = open(devname,O_RDWR)) < 0)
        {
                fprintf(stderr,"File open error\n");
                return 8;
        }
        printf("Erase Total %d Units\n", count);

        if (ioctl(Fd,MEMGETREGIONCOUNT,&regcount) == 0)
        {
                if(regcount == 0)
                {
                        res = non_region_erase(Fd, start, count, unlock);
                }
                else
                {
                        res = region_erase(Fd, start, count, unlock, regcount);
                }
        }

	close(Fd);

        return res;
}
int flash_eraseall(char *devname)
{
	mtd_info_t mi;
        int fd;
        int count=1;

        // Open and size the device
        if ((fd = open(devname,O_RDWR)) < 0){
		fprintf(stderr,"File open error\n");
		return -1;
        }
	if(ioctl(fd, MEMGETINFO, &mi) == 0) {
		count = mi.size/mi.erasesize; 
	}
	close(fd);

	return flash_erase(devname, 0, count);
}
#endif//!WIN32

/*uboot cfg format: var=value*/
int write_ubootcfg(char *var, char *value)
{
#ifndef WIN32
	FILE* fp;
	int crc;
//	char *envptr = (char*)&myenvironment;
//	char *dataptr = envptr + BOOT_ENV_HEADER_SIZE;
	char *envptr, *dataptr;
	unsigned int datasize = BOOT_ENV_DATA_SIZE;//64k-4
	int defaultcrc;
	char *ptmp, *pbuf, *p;
	int datalen;

//	printf("write_ubootcfg\n");//zy
	fp = fopen(BOOTLOADER_PART, "r");
	if(fp == NULL) {
		printf("can not open /dev/mtd0\n");
		return -1;
	}
	envptr = malloc(BOOT_ENV_SIZE);	
	if(envptr == NULL) {
		printf("alloc memory for env fail\n");	
		fclose(fp);
		return -1;
	}	
	dataptr = envptr + BOOT_ENV_HEADER_SIZE;
	memset(dataptr,'\0',datasize);
	fseek(fp, CFG_ENV_OFFSET, SEEK_SET);
	fread(envptr, 1, BOOT_ENV_SIZE, fp);
	fclose(fp);

	/*use my crc checksum instead of crc32*/
	crc_init();
	defaultcrc = *((unsigned long *)envptr);
	crc = crc_calc(0, dataptr, datasize);
	if(crc != defaultcrc){
		printf("uboot cfg crc error %x,%x,crc_size:%x\n", defaultcrc, crc,datasize);
		free(envptr);
		return -1;
	}

	/*count total len*/
	ptmp = dataptr;
	while(strlen(ptmp)>0){
		ptmp = ptmp + strlen(ptmp) +1;
	}
 	datalen = ptmp - dataptr;
        /*malloc a buf for tmp use, copy cfg to this buf*/
        p = malloc(datalen+1);
	if(!p){
		printf("failed to malloc a buf for tmp use\n");
		free(envptr);
		return -1;
	}
	ptmp = p;
        memcpy(ptmp, dataptr, datalen+1);
        /*clear old data zone*/
        memset(dataptr, '\0', datalen+1);

	/*write to data buf*/
        pbuf = dataptr;
        while(strlen(ptmp)>0)
        {
                //printf("%s\n",ptmp);
		/*item not matched  write to data buf*/
                if(strncmp(ptmp,var,strlen(var)) != 0){
                        /*record item which not match*/
                        strcpy(pbuf, ptmp);
                        pbuf = pbuf + strlen(pbuf) +1;
                }
                ptmp = ptmp + strlen(ptmp) +1;
        }
	free(p);
	sprintf(pbuf, "%s=%s", var, value);

	/*count crc*/
    crc = crc_calc (0, dataptr, datasize);
	crc_done();
	*((unsigned int *)envptr) = crc;

	/*erase flash*/
	flash_erase(BOOTLOADER_PART,CFG_ENV_OFFSET,1);/*skip 128k*/
	/*write to flash*/
	fp = fopen(BOOTLOADER_PART,"w");
        if(fp == NULL){
        	printf("can not open /dev/mtd0\n");
		free(envptr);
	        return -1;
        }
        fseek(fp, CFG_ENV_OFFSET, SEEK_SET);
        fwrite(envptr, 1, BOOT_ENV_SIZE, fp);
	fclose(fp);
	free(envptr);
	sync();
        printf("Revise successfully\n");
#endif//!WIN32
	return 0;
}

/*cfgbuf format: string;string;string;*/
int write_multi_ubootcfg(char *cfgbuf)
{
#ifndef WIN32
	FILE* fp;
	int crc;
//	char *envptr = (char *)&myenvironment;
//	char *dataptr = envptr + BOOT_ENV_HEADER_SIZE;
	char *envptr, *dataptr;
	unsigned int datasize = BOOT_ENV_DATA_SIZE;//64k-4
	int defaultcrc;
	char *ptmp, *pbuf, *p;
	int datalen;
	char *pvar, *pval;
	char tmpbuf[512];//save tmp value
	int found=0;
	char *tmpcfg;

//	printf("write_multi_ubootcfg\n");//zy
	/*read cfg*/
	fp = fopen(BOOTLOADER_PART,"r");
	if(fp == NULL) {
		printf("can not open /dev/mtd0\n");
		return -1;
	}
	envptr = malloc(BOOT_ENV_SIZE);	
	if(envptr == NULL) {
		printf("alloc memory for env fail\n");	
		fclose(fp);
		return -1;
	}	
	dataptr = envptr + BOOT_ENV_HEADER_SIZE;
	memset(dataptr,'\0',datasize);
	fseek(fp, CFG_ENV_OFFSET, SEEK_SET);
	fread(envptr, 1, BOOT_ENV_SIZE, fp);
	fclose(fp);

	/*use my crc checksum instead of crc32*/
	crc_init();
	defaultcrc = *((unsigned long *)envptr);
	crc = crc_calc (0, dataptr, datasize);
	if(crc != defaultcrc){
		printf("uboot cfg crc error %x,%x,crc_size:%x\n", defaultcrc, crc,datasize);
		free(envptr);
		return -1;
	}

	/*count total len*/
	ptmp = dataptr;
	while(strlen(ptmp)>0){
		ptmp = ptmp + strlen(ptmp) +1;
	}
 	datalen = ptmp - dataptr;
        /*malloc a buf for tmp use, copy cfg to this buf*/
        p = malloc(datalen+1);
	if(!p){
		printf("failed to malloc a buf for tmp use.\n");
		free(envptr);
		return -1;
	}
	ptmp = p;
        memcpy(ptmp, dataptr, datalen+1);
        /*clear old data zone*/
        memset(dataptr, '\0', datalen+1);

	/*del from data buf*/
	pbuf = dataptr;
	while(strlen(ptmp)>0) {
		/*copy cfg*/
		strncpy(tmpbuf, cfgbuf, sizeof(tmpbuf));
		tmpcfg = tmpbuf;
		/*find it if revise*/
		while((tmpcfg!=NULL) && (tmpcfg[0]!=0)) {
			pval = strsep(&tmpcfg, ";");
			pvar = strsep(&pval, "=");

	        	if(strncmp(ptmp,pvar,strlen(pvar)) == 0){
				found = 1;
				break;
				//syslog(LOG_DEBUG, "found %s %s", pvar, pval);
			}
		
		}
		if(found == 0) {
	       		/*record item which not found*/
		    	memcpy(pbuf, ptmp, strlen(ptmp)+1);
	        	pbuf = pbuf + strlen(pbuf) +1;
		}
		found = 0;
		ptmp = ptmp + strlen(ptmp) +1;
	}
	free(p);
	/*write to data buf*/
	while((cfgbuf!=NULL) && (cfgbuf[0]!=0)) {
		/*parse cfg string*/
		pval = strsep(&cfgbuf, ";");
		pvar = strsep(&pval, "=");

		sprintf(tmpbuf, "%s=%s", pvar, pval);
		memcpy(pbuf, tmpbuf, strlen(tmpbuf)+1);
	        pbuf = pbuf + strlen(pbuf) +1;
	}
	
	/*count crc*/
        crc = crc_calc (0, dataptr, datasize);
	crc_done();
	*((unsigned int *)envptr) = crc;

	/*erase flash*/
	flash_erase(BOOTLOADER_PART,CFG_ENV_OFFSET,1);/*skip 128k*/
	/*write to flash*/
	fp = fopen(BOOTLOADER_PART,"w");
        if(fp == NULL){
        	printf("can not open /dev/mtd0\n");
		free(envptr);
	        return -1;
        }
        fseek(fp, CFG_ENV_OFFSET, SEEK_SET);
        fwrite(envptr, 1, BOOT_ENV_SIZE, fp);
	fclose(fp);
	free(envptr);
	sync();
        printf("Revise successfully\n");
#endif
		
	return 0;
}

