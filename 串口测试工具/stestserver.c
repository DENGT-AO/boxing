#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>     /*标准函数库定义*/
#include <unistd.h>     /*Unix 标准函数定义*/
#include <termios.h>    /*PPSIX 终端控制定义*/
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
    /*获取原有串口配置*/
    if  ( tcgetattr( fd,&newtio)  !=  0) { 
        perror("SetupSerial 1");
        return -1;
    }
    
    //memset(&newtio, 0, sizeof(newtio));
    /*CREAD 开启串行数据接收，CLOCAL并打开本地连接模式*/
    newtio.c_cflag  |=  CLOCAL | CREAD;

    /*设置数据位*/
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
    /* 设置奇偶校验位 */
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
     设置波特率 
     cfsetispeed设置输入波特率
     cfsetospeed设置输出波特率
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
    /*设置停止位*/
    if( nStop == 1 )/*设置停止位；若停止位为1，则清除CSTOPB，若停止位为2，则激活CSTOPB*/
    {
        newtio.c_cflag &=  ~CSTOPB;/*默认为一位停止位； */
    }
    else if ( nStop == 2 )
    {
        newtio.c_cflag |=  CSTOPB;
    }
    /*设置最少字符和等待时间，对于接收字符和等待时间没有特别的要求时*/
    newtio.c_cc[VTIME]  = 0;/*非规范模式读取时的超时时间；*/
    newtio.c_cc[VMIN] = 0;/*非规范模式读取时的最小字符数*/
    /*tcflush清空终端未完成的输入/输出请求及数据；TCIFLUSH表示清空正收到的数据，且不读取出来 */
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
    struct termios oldtio;
    
    fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY, 0);
	if(fd<0){
		printf("cannot open device: %s\n, err: %d,%s\n", 
			dev, errno, strerror(errno));
		exit(-1);
	}

    tcgetattr(fd, &oldtio);
	serial_set_speed(fd, nSpeed);
	serial_set_parity(fd, nBits, nStop, nEvent, 0, 0);

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


char crc_8(char crc, char *message, int len)
{
	int i = 0;
    int crc_sum= 0;

    for (i=0;i<len;i++) {
        crc_sum += message[i];
    }

    return crc_sum & 0xff;
}

int crc_para(char *data)
{
    char id[5] = {0};
    char length[3] = {0};
    char sdata[4] = {0};
    char crc_sum[5] = {0};
    int i = 0;
    char crc_message[32] = {0};
    char crc_ret;


    //S0008082560008D
    for(i=0;i<sizeof(id)-1;i++)
    {
        id[i] = data[1+i];
    }
    id[sizeof(id)-1] = '\0';
    for (i=0;i<strlen(id);i++) {
        if (id[i]!='0') break; //找出前面有几个0
    }   
    strcpy(id,id+i);
    

    for(i=0;i<sizeof(length)-1;i++)
    {
        length[i] = data[5+i];
    }
    length[sizeof(length)-1] = '\0';
    for (i=0;i<strlen(length);i++) {
        if (length[i]!='0') break; //找出前面有几个0
    }
    strcpy(length,length+i);
    

    for(i=0;i<sizeof(sdata)-1;i++)
    {
        sdata[i] = data[7+i];
    }
    sdata[sizeof(sdata)-1] = '\0';
    for (i=0;i<strlen(sdata);i++) {
        if (sdata[i]!='0') break; //找出前面有几个0
    }
    strcpy(sdata,sdata+i);
    
    for(i=0;i<sizeof(crc_sum)-1;i++)
    {
        crc_sum[i] = data[10+i];
    }
    crc_sum[sizeof(crc_sum)-1] = '\0';
    for (i=0;i<strlen(crc_sum);i++) {
        if (crc_sum[i]!='0') break; //找出前面有几个0
    }
    strcpy(crc_sum,crc_sum+i);
    


    //printf("id = %s, length =%s, sdata = %s\n", id, length, sdata);
    //printf("crc_sum = %s\n", crc_sum);
    strncat(crc_message, id, sizeof(crc_message));
    strncat(crc_message, length, sizeof(crc_message));
    strncat(crc_message, sdata, sizeof(crc_message));
    
    crc_ret = crc_8(crc_ret, crc_message, sizeof(crc_message));
    

    if ((int)crc_ret == atoi(crc_sum)) {
        return 0;
    } else {
        printf("data = %s\n", data);
        printf("crc_message = %s\n", crc_message);
        printf("%d", crc_ret);
        return 1;
    } 
}


char* pars_data(char *data, char *ret) 
{
    char *com_buf = NULL;
    static char tmp_buf[1024] = {'\0'};
    char *p = NULL;
    int i = 0;

    com_buf=(char *)malloc(1024*sizeof(char));
    if (!com_buf) {
        printf("malloc failed!exit to retry!\n");
        exit(-1);
    }
    memset(com_buf, 0, sizeof(com_buf));
    

    if (data[0] == 'S') {
        //printf("111111111111111");
        strncpy(tmp_buf, data, sizeof(tmp_buf));
        ret = NULL;
        free(com_buf);
        return ret;
    } else if(data[strlen(data)-1] == 'D') {
        //printf("AAAAAAAAAAAAAAAA");
        strncat(tmp_buf, data, strlen(data));
        strncpy(com_buf, tmp_buf, sizeof(tmp_buf));
        com_buf[strlen(com_buf)] = '\0';
        memset(tmp_buf, 0, sizeof(tmp_buf));
        if (!crc_para(com_buf)) {
            ret = com_buf;
        } else {
            ret = NULL;
        }
        return ret;
    } else if (strchr(data,'S') && strchr(data,'D')) {
        //printf("BBBBBBBBBBBBBBBBBB");
        p = strchr(data,'S');
        //printf("CCCCCC");
        for (i = 0; i < p-data; i++)
        {
            tmp_buf[strlen(tmp_buf)] = data[i];
        }
        //printf("DDD");
        tmp_buf[strlen(tmp_buf)] = '\0';
        //printf("EEE");
        strncpy(com_buf, tmp_buf, sizeof(tmp_buf));
        //printf("FFF");
        memset(tmp_buf, 0, sizeof(tmp_buf));
        //printf("RRR");
        strncpy(tmp_buf, p, strlen(p));
        //printf("%s\n",com_buf);
        //printf("TTT\n");
        if (!crc_para(com_buf)) {
            ret = com_buf;
        } else {
            ret = NULL;
        }
        return ret;
    } else {
        //printf("22222222222222");
        strncat(tmp_buf, data, sizeof(tmp_buf));
        ret = NULL;
        free(com_buf);
        return ret;
    }
}

//void read_cb(evutil_socket_t fd, short event_flag, void *arg)
void read_cb(int fd)
{   

    fd_set fdset;
    char buf[1024] = {'\0'};
    char comp_data[1024] = {'\0'};
    char *ret_data = NULL;
    int len = 0;

    FD_ZERO(&fdset);
	FD_SET(fd, &fdset);
	FD_SET(STDIN_FILENO, &fdset);
    if(select(fd+1, &fdset, NULL, NULL, NULL)<0) exit;


    if(FD_ISSET(fd, &fdset)) {
        len = read(fd, buf, sizeof(buf-1));
        if (len == 0) {
            return;
        }
        //printf("read event: %s \n ", what & EV_READ ? "YES":"NO");
        //printf("buf = %s, len = %d\n", buf, len);
        ret_data = pars_data(buf, ret_data);
        //printf("%s\n", data);
        if (ret_data != NULL) {
            //printf("\n%s\n%d\n",ret_data,strlen(ret_data));
            write(fd, ret_data, strlen(ret_data));
            //write(fd, buf, strlen(buf));
            free(ret_data);
            ret_data = NULL;
        }
        memset(buf, 0, sizeof(buf));
    }
}

int stestserver_main(int argc, char* argv[])
{
    int c;
    int fd, Baud, nBits, nStop;
    char nValid;
    char device[32] = {0};
    int ret;
    pthread_t writeThread;
    struct event_base *base;
    struct event *readevent;

    while ((c = getopt(argc, argv, "d:r:b:v:s:h")) != -1) {
		switch (c) {
		case 'h':
			message();
		case 'd':
			memcpy(device, optarg, sizeof(device));
            //printf("%s",device);
			break;
		case 'r':
			Baud = atoi(optarg);
			break;
		case 'b':
			nBits = atoi(optarg);
			break;
		case 'v':
            //printf("optarg=%s", optarg);
			nValid = optarg[0];
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
    
    printf("open device %s for operation(fd:%d), baudrate:%d, databits:%d, parity:%c, stopbit:%d\n", device, fd, Baud, nBits, nValid, nStop);
    //创建写线程
    /*
    writeThread = pthread_create(&writeThread, NULL, writefunc, &fd);
    if(ret != 0)
    {
        printf("create pthread error!\n");
        goto ERR1;
    }*/

    while (1)
    {
        /* code */
        read_cb(fd);
    }


/*
    //创建event base
    base = event_base_new();
    if (base == NULL)
    {
        perror("create event baseerror\n");
        goto ERR1;
    }

    //创建事件
    readevent = event_new(base, fd, EV_READ | EV_PERSIST, read_cb, NULL);
    if (readevent == NULL)
    {
        perror("create event error\n");
        goto ERR2;
    }

    //挂起事件
    ret = event_add(readevent, NULL);
    if (ret)
    {
        perror("add event error\n");
        goto ERR2;
    }

    //轮询事件
    event_base_dispatch(base);

ERR2:
    event_base_free(base);
ERR1:
*/
    close(fd);
    return 0;
}