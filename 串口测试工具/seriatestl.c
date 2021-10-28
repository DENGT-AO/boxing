#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>     /*��׼�����ⶨ��*/
#include <unistd.h>     /*Unix ��׼��������*/
#include <termios.h>    /*PPSIX �ն˿��ƶ���*/
#include "shared.h"
#include <pthread.h>
#include <event2/event.h>

void message(void)
{
	printf("=====================================================================\n");
    printf("Usage: ngrokc\n");
    printf("    -d    Serial device node.\n");
    printf("    -r    Baud rate.\n");
    printf("    -b    Data bits.\n");
    printf("    -v    Valid bit.\n");
    printf("    -s    Stop bits.\n");
    printf("    -h    See help\n");
    printf("=====================================================================\n");
    exit(0);
}


int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
    
    struct termios newtio, oldtio;
    /*��ȡԭ�д�������*/
    if  ( tcgetattr( fd,&newtio)  !=  0) { 
        perror("SetupSerial 1");
        return -1;
    }
    
    //memset(&newtio, 0, sizeof(newtio));
    /*CREAD �����������ݽ��գ�CLOCAL���򿪱�������ģʽ*/
    newtio.c_cflag  |=  CLOCAL | CREAD;

    /*��������λ*/
    newtio.c_cflag &= ~CSIZE;
    switch( nBits )
    {
        case 7:
            newtio.c_cflag |= CS7;
            break;
        case 8:
            newtio.c_cflag |= CS8;
            break;
    }
    /* ������żУ��λ */
    switch( nEvent )
    {
        case 'O':
            newtio.c_cflag |= PARENB;
            newtio.c_cflag |= PARODD;
            newtio.c_iflag |= (INPCK | ISTRIP);
            break;
        case 'E': 
            newtio.c_iflag |= (INPCK | ISTRIP);
            newtio.c_cflag |= PARENB;
            newtio.c_cflag &= ~PARODD;
            break;
        case 'N':  
            newtio.c_cflag &= ~PARENB;
            break;
    }

    /*
     ���ò����� 
     cfsetispeed�������벨����
     cfsetospeed�������������
    */
    switch( nSpeed )
    {
        case 2400:
            cfsetispeed(&newtio, B2400);
            cfsetospeed(&newtio, B2400);
            break;
        case 4800:
            cfsetispeed(&newtio, B4800);
            cfsetospeed(&newtio, B4800);
            break;
        case 9600:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
            break;
        case 115200:
            cfsetispeed(&newtio, B115200);
            cfsetospeed(&newtio, B115200);
            break;
        case 460800:
            cfsetispeed(&newtio, B460800);
            cfsetospeed(&newtio, B460800);
            break;
        default:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
            break;
    }
    /*����ֹͣλ*/
    if( nStop == 1 )/*����ֹͣλ����ֹͣλΪ1�������CSTOPB����ֹͣλΪ2���򼤻�CSTOPB*/
    {
        newtio.c_cflag &=  ~CSTOPB;/*Ĭ��Ϊһλֹͣλ�� */
    }
    else if ( nStop == 2 )
    {
        newtio.c_cflag |=  CSTOPB;
    }
    /*���������ַ��͵ȴ�ʱ�䣬���ڽ����ַ��͵ȴ�ʱ��û���ر��Ҫ��ʱ*/
    newtio.c_cc[VTIME]  = 0;/*�ǹ淶ģʽ��ȡʱ�ĳ�ʱʱ�䣻*/
    newtio.c_cc[VMIN] = 0;/*�ǹ淶ģʽ��ȡʱ����С�ַ���*/
    /*tcflush����ն�δ��ɵ�����/����������ݣ�TCIFLUSH��ʾ������յ������ݣ��Ҳ���ȡ���� */
    tcflush(fd,TCIFLUSH);
    if((tcsetattr(fd,TCSANOW,&newtio))!=0)
    {
        perror("com set error");
        return -1;
    }
//  printf("set done!\n\r");
    return 0;
}

int uart_init(char *dev, int nSpeed, int nBits, char nEvent, int nStop)
{
    int fd = 0;
    int ret = 0;
    
    fd = open(dev , O_RDWR | O_NOCTTY | O_NDELAY, 0);    
    if (fd < 0) 
    {
        printf("open error!\n");        
        return -1;
    }
    ret = set_opt(fd, nSpeed, nBits, nEvent, nStop);
    if (ret == -1)
    {
        perror("set uart error~\n");
        return -1;
    }


    return fd;
}

void * writefunc(void *ptr)
{
    int fd = *(int *)ptr;
    char buf[1024] = {0};
    int len = 0;

    while (1)
    {
        //fgets(buf, 1024, stdin);
        gets(buf);
        len = strlen(buf);
        if ( len >= 0 )
        {
            buf[len] = '\n';
            write(fd, buf, len + 1);
        }
    }

}

void read_cb(evutil_socket_t fd, short event_flag, void *arg)
{
    char buf[1024] = {0};
    int len = read(fd, buf, sizeof(buf-1));
    //printf("read event: %s \n ", what & EV_READ ? "YES":"NO");
    printf("%s\n", buf);
    write(fd, buf, sizeof(buf));
    memset(buf, 0, sizeof(buf));
}

int stestserver_main(int argc, char* argv[])
{
    int c;
    int fd, Baud, nBits, nValid, nStop;
    char device[32] = {0};
    int ret;
    pthread_t writeThread;
    struct event_base *base;
    struct event *readevent;

    printf("aaaaaaaaaaaaaaaaaaaaa\n");

    while ((c = getopt(argc, argv, "d:r:b:v:s:h")) != -1) {
		switch (c) {
		case 'h':
			message();
		case 'd':
			memcpy(device, optarg, sizeof(device));
            printf("%s",device);
			break;
		case 'r':
			Baud = atoi(optarg);
			break;
		case 'b':
			nBits = atoi(optarg);
			break;
		case 'v':
			nValid = atoi(optarg);
			break;
		case 's':
			nStop = atoi(optarg);
			break;
		default:
			printf("ignore unknown arg: -%c %s", c, optarg ? optarg : "");
			break;
		}
	}

    fd = uart_init(device, 115200, 8, 'N', 1);
    if (fd == -1)
    {
        printf("UART INIT ERROR!\n");
        exit(-1);
    }
    
    //����д�߳�
    /*
    writeThread = pthread_create(&writeThread, NULL, writefunc, &fd);
    if(ret != 0)
    {
        printf("create pthread error!\n");
        goto ERR1;
    }*/

    //����event base
    base = event_base_new();
    if (base == NULL)
    {
        perror("create event baseerror\n");
        goto ERR1;
    }

    //�����¼�
    readevent = event_new(base, fd, EV_READ | EV_PERSIST, read_cb, NULL);
    if (readevent == NULL)
    {
        perror("create event error\n");
        goto ERR2;
    }

    //�����¼�
    ret = event_add(readevent, NULL);
    if (ret)
    {
        perror("add event error\n");
        goto ERR2;
    }

    //��ѯ�¼�
    event_base_dispatch(base);

ERR2:
    event_base_free(base);
ERR1:
    close(fd);
    return 0;
}


























#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>
#include <string.h>

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
int stestserver_main(int argc, char* argv[])
{
	int fd;
	char *lpszDev = "/dev/ttyO1";
	char dev[32];
	char devBuf[1024];
	char consoleBuf[1024];
	int nDevLen, nConsoleLen;
	fd_set fdset;
	int BaudRate = 115200, DataBits = 8, StopBit = 1;
	int Xonoff = 0;
    int i = 0;

#ifndef WIN32
	struct termios oldtio;
#endif	

	//strlcpy(dev, nvram_safe_get("console_iface"), sizeof(dev));;

	if(argc>1) lpszDev = argv[1];

	if(argc>2) BaudRate = atoi(argv[2]);

	if(argc>3) DataBits = atoi(argv[3]);
	if(argc>4) StopBit = atoi(argv[4]);
	if(argc>5) Xonoff = atoi(argv[5]);

	//fd = open(lpszDev, O_RDWR|O_NONBLOCK, 0666);
	//fd = open(lpszDev, O_RDWR|O_NONBLOCK);
	fd = open(lpszDev , O_RDWR | O_NOCTTY | O_NDELAY, 0);
	if(fd<0){
		printf("cannot open device: %s\n, err: %d,%s\n", 
			lpszDev, errno, strerror(errno));
		exit(-1);
	}
	

	printf("open device %s for operation(fd:%d), baudrate:%d, databits:%d, stopbit:%d, xonoff:%d\n", lpszDev,fd, BaudRate, DataBits, StopBit, Xonoff);

	tcgetattr(fd, &oldtio);
	serial_set_speed(fd, BaudRate);
	serial_set_parity(fd, DataBits, StopBit, 'N', 0, Xonoff);


	memset((void*)devBuf, 0, sizeof(devBuf));
	memset((void*)consoleBuf, 0, sizeof(consoleBuf));
	nDevLen = 0;
	nConsoleLen = 0;
	
	while(1){
		FD_ZERO(&fdset);
		FD_SET(fd, &fdset);
		FD_SET(STDIN_FILENO, &fdset);

		if(select(fd+1, &fdset, NULL, NULL, NULL)<0) break;

        if(FD_ISSET(fd, &fdset)){ //read device
			//printf("get some data from %s..\n", lpszDev);
            i = read(fd, devBuf+nDevLen, sizeof(devBuf)-nDevLen);
			if(i<0){
				printf("Cannot read device: read %d\n", i);
				break;
			} else if (i == 0) {
				printf("Device is shutdown\n");
				break;
			}
			nDevLen += i;			

			printf("<(%d)< %s\n", nDevLen, devBuf);
		}

		if(FD_ISSET(STDIN_FILENO, &fdset)){ //read console
			//printf("get some data from console..\n");

			if(strncasecmp(devBuf, "quitit", 6)==0) break;
			if(strncasecmp(devBuf, "ctrl+z", 6)==0) {
				write(fd, devBuf, 1);
				memset(devBuf, 0, sizeof(devBuf));
				nConsoleLen = 0;
			}

			//scan for \n\r
			for(i=0; i<nConsoleLen; i++){
				if(consoleBuf[i]=='\r' || consoleBuf[i]=='\n'){
					consoleBuf[i] = '\0';
					strcat(consoleBuf, "\r\n");
					write(fd, consoleBuf, i+2);
			        printf("aaaaaaaaaaa%s\n",  consoleBuf);
					memset(consoleBuf, 0, sizeof(consoleBuf));
					nConsoleLen = 0;				
				}
			}
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
       
