/*
 * $Id$ --
 *
 *   Serial port debug
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#ifdef WIN32
	#include <winsock2.h>
	#include <io.h>
	#define STDIN_FILENO	0
#else
	#include <unistd.h>
	#include <termios.h>
#endif

#include "shared.h"

//#include "serialport.h"

//////////////////////////////////////////////////////////////////////////
//	for serial debug
//////////////////////////////////////////////////////////////////////////
int sdebug_main(int argc, char* argv[])
{
	int fd;
	char *lpszDev = "/dev/ttyS0";
	char dev[32];
	char devBuf[1024];
	char consoleBuf[1024];
	int nDevLen, nConsoleLen;
	fd_set fdset;
	int BaudRate = 19200, DataBits = 8, StopBit = 1;
	int Xonoff = 0;

#ifndef WIN32
	struct termios oldtio;
#endif	

	//strlcpy(dev, nvram_safe_get("console_iface"), sizeof(dev));;

	if(argc>1) lpszDev = argv[1];
	else lpszDev = dev;

	if(argc>2) BaudRate = atoi(argv[2]);

	if(argc>3) DataBits = atoi(argv[3]);
	if(argc>4) StopBit = atoi(argv[4]);
	if(argc>5) Xonoff = atoi(argv[5]);

	//fd = open(lpszDev, O_RDWR|O_NONBLOCK, 0666);
	//fd = open(lpszDev, O_RDWR|O_NONBLOCK);
	fd = open(lpszDev, O_RDWR);
	if(fd<0){
		printf("cannot open device: %s\n, err: %d,%s\n", 
			lpszDev, errno, strerror(errno));
		exit(-1);
	}
	

	printf("open device %s for operation(fd:%d), baudrate:%d, databits:%d, stopbit:%d, xonoff:%d\n", lpszDev,fd, BaudRate, DataBits, StopBit, Xonoff);
#ifndef WIN32
#if 1
	tcgetattr(fd, &oldtio);
	serial_set_speed(fd, BaudRate);
	serial_set_parity(fd, DataBits, StopBit, 'N', 0, Xonoff);
#else
	//3. set serial baudrate
	//save old serial settings
	tcgetattr(fd, &oldtio);
	cfmakeraw(&newtio);
	//apply new settings
	cfsetospeed(&newtio, GetBaudRate(BaudRate));
	cfsetispeed(&newtio, GetBaudRate(BaudRate));
	tcsetattr(fd, TCSANOW, &newtio);
#endif
#endif

	memset((void*)devBuf, 0, sizeof(devBuf));
	memset((void*)consoleBuf, 0, sizeof(consoleBuf));
	nDevLen = 0;
	nConsoleLen = 0;

	//write(fd, "AT\r\n", 4);	
	//sleep(1);
	//write(fd, "ATI\r\n", 5);	
	
	while(1){
		FD_ZERO(&fdset);
		FD_SET(fd, &fdset);
		FD_SET(STDIN_FILENO, &fdset);

		if(select(fd+1, &fdset, NULL, NULL, NULL)<0) break;

		if(FD_ISSET(STDIN_FILENO, &fdset)){ //read console
			//printf("get some data from console..\n");
			int i = read(STDIN_FILENO, consoleBuf+nConsoleLen, sizeof(consoleBuf)-nConsoleLen);
			if(i<0){
				printf("Cannot read console: read %d\n", i);
				break;
			}
			nConsoleLen += i;	

			if(strncasecmp(consoleBuf, "quitit", 6)==0) break;
			if(strncasecmp(consoleBuf, "ctrl+z", 6)==0){
				printf(">>ctrl+z ");
				consoleBuf[0] = 0x1A;
				write(fd, consoleBuf, 1);
				memset(consoleBuf, 0, sizeof(consoleBuf));
				nConsoleLen = 0;
			}

			//scan for \n\r
			for(i=0; i<nConsoleLen; i++){
				if(consoleBuf[i]=='\r' || consoleBuf[i]=='\n'){
					consoleBuf[i] = '\0';
					strcat(consoleBuf, "\r\n");
					write(fd, consoleBuf, i+2);
					printf(">>> %.*s", i, consoleBuf);
					memset(consoleBuf, 0, sizeof(consoleBuf));
					nConsoleLen = 0;				
				}
			}
			printf("\n");

		}

		if(FD_ISSET(fd, &fdset)){ //read device
			//printf("get some data from %s..\n", lpszDev);
			int i = read(fd, devBuf+nDevLen, sizeof(devBuf)-nDevLen);
			if(i<0){
				printf("Cannot read device: read %d\n", i);
				break;
			} else if (i == 0) {
				printf("Device is shutdown\n");
				break;
			}
			nDevLen += i;			

			printf("<(%d)< %s\n", nDevLen, devBuf);
			memset(devBuf, 0, sizeof(devBuf));
			nDevLen = 0;					
			continue;
/*
			//scan for \r\n
			for(i=0; i<nDevLen; i++){
				if(devBuf[i]=='\r' || devBuf[i]=='\n'){
					i++;
					printf("<<< %.*s", i, devBuf);
					fflush(stdout);
					memcpy(devBuf, devBuf+i, sizeof(devBuf)-i);
					//memset(devBuf, 0, sizeof(devBuf));
					nDevLen -= i;					
				}
			}
			if(nDevLen) printf("<<< %s\n", devBuf);
			memset(devBuf, 0, sizeof(devBuf));
			nDevLen = 0;
*/
		}

	}

#ifndef WIN32
	//restore old settings
	tcsetattr(fd, TCSANOW, &oldtio);
#endif
	
	//cleanup
	close(fd);

	printf("exit\n");

	return 0;
}

//////////////////////////////////////////////////////////////////////////
//      for redirect module debug com
//////////////////////////////////////////////////////////////////////////
int comredirect_main(int argc, char* argv[])
{
#ifndef WIN32
	int comfd, usbfd;
	char *comDev = "/dev/ttyS0";
	char *usbDev = "/dev/ttyUSB1";
	char comname[16]="/dev/", usbname[16]="/dev/";
	char usbbuf[1024], combuf[1024];
	struct termios comoldtio, usboldtio;
	int ncomLen=0, nusbLen=0;
	fd_set fdset;

	/*evdo cne680 450m: ttyUSB1
	 *evdo cne660 450m: ttyUSB0
	 *evdo mc2716: ttyUSB3
	 *wcdma em770w: ttyUSB1
	 */
	if(argc>1) usbDev=argv[1];
	if(argc>2) comDev=argv[2];

	if(*usbDev != '/') {
		strcat(usbname, usbDev);
		usbDev = usbname;
	}
	if(*comDev != '/') {
		strcat(comname, comDev);
		comDev = comname;
	}
	printf("redirect %s and %s (mc2716-USB1[AT]/USB3,cne660-USB0,other-USB1)\n", usbDev, comDev);
	//system("service wan1 stop");
	comfd = open(comDev, O_RDWR);
	tcgetattr(comfd, &comoldtio);
	serial_set_speed(comfd, 115200);
	serial_set_parity(comfd, 8, 1, 'N', 0, 0);

	usbfd = open(usbDev, O_RDWR);
	tcgetattr(usbfd, &usboldtio);
	serial_set_speed(usbfd, 115200);
	serial_set_parity(usbfd, 8, 1, 'N', 0, 0);

	memset((void*)combuf, 0, sizeof(combuf));
	memset((void*)usbbuf, 0, sizeof(usbbuf));

	while(1) {
		FD_ZERO(&fdset);
		FD_SET(comfd, &fdset);
		FD_SET(usbfd, &fdset);

		if(select(usbfd+1, &fdset, NULL, NULL, NULL)<0) break;

		if(FD_ISSET(comfd, &fdset)){ //read com
			int i = read(comfd, combuf+ncomLen, sizeof(combuf)-ncomLen);
			if(i<0){
				printf("Cannot read console: read %d\n", i);
				break;
			}
			ncomLen += i;

			//printf("get some data from ttyS4,len:%d..\n", i);
			write(usbfd, combuf, i);//write usb com
			memset(combuf, 0, sizeof(combuf));
			ncomLen = 0;
		}

		if(FD_ISSET(usbfd, &fdset)){ //read usb com
			int i = read(usbfd, usbbuf+nusbLen, sizeof(usbbuf)-nusbLen);
			if(i<0){
				printf("Cannot read console: read %d\n", i);
				break;
			}
			ncomLen += i;

			//printf("get some data from ttyUSB1,len:%d..\n", i);
			write(comfd, usbbuf, i);//write com
			memset(usbbuf, 0, sizeof(usbbuf));
			ncomLen = 0;
		}
	}

	tcsetattr(comfd, TCSANOW, &comoldtio);
	tcsetattr(usbfd, TCSANOW, &usboldtio);

	close(comfd);
	close(usbfd);
#endif

	return 0;
}
