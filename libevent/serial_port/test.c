#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <event2/event.h>

//串口操作的头文件
#include     <stdio.h>      /*标准输入输出定义*/
#include     <stdlib.h>     /*标准函数库定义*/
#include     <unistd.h>     /*Unix 标准函数定义*/
#include     <sys/types.h>  
#include     <sys/stat.h>   
#include     <fcntl.h>      /*文件控制定义*/
#include     <termios.h>    /*PPSIX 终端控制定义*/
#include     <errno.h>      /*错误号定义*/

#include    <pthread.h>

//串口名称
#define UART_DEV "/dev/ttyUSB0"

//编译方法：gcc -o test test.c -levent -lpthread

//对操作的处理函数
void read_cb(evutil_socket_t fd, short what, void *arg)
{
    //读管道
    char buf[1024] = {0};
    int len = read(fd, buf, sizeof(buf-1));
    //printf("read event: %s \n ", what & EV_READ ? "YES":"NO");
    printf("%s",buf);
}


int set_opt(int fd)
{    
    struct termios new_cfg;
    tcgetattr(fd, &new_cfg);    
    
    new_cfg.c_cflag |= (CLOCAL | CREAD);    
 
    cfsetispeed(&new_cfg, B115200);//设置波特率    
    cfsetospeed(&new_cfg, B115200);    
 
    new_cfg.c_cflag &= ~CSIZE;  //设置串口数据位，停止位和效验位  
    new_cfg.c_cflag |= CS8;    
    new_cfg.c_cflag &= ~PARENB;    
    new_cfg.c_cflag &= ~CSTOPB;    
    new_cfg.c_cc[VTIME] = 0;    
    new_cfg.c_cc[VMIN] = 0;  
    
    //只是串口传输数据，而不需要串口来处理，那么使用原始模式(Raw Mode)方式来通讯
    new_cfg.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
    new_cfg.c_oflag  &= ~OPOST;   /*Output*/  
 
    tcflush(fd,TCIFLUSH);    
    tcsetattr(fd, TCSANOW, &new_cfg);    
 
    return 0;
}

int uart_init(void )
{
    int fd=0;    
    fd = open(UART_DEV , O_RDWR | O_NOCTTY | O_NDELAY, 0);    
    if (fd < 0) 
    {        
        printf("open error!\n");        
        return -1;
    }    
    set_opt(fd);    
    return  fd;
}

void * writeThreadFun(void * arg )
{
	int fd = *(int *)arg;
	char buf[1024] = {0};
	int len = 0;
	
	while(1)
	{
		gets(buf);
		len = strlen(buf);
		if(len >= 0)//这里是操作嵌入式板子的shell，每次都要发送一个回车，其他通讯根据实际情况来确定是否需要加回车
		{
			buf[len] = '\n';
			write(fd, buf ,len+1);
	    }
	}
}

int main(int argc, const char* argv[])
{   
    //open file
    int fd = uart_init();
    if(fd < 0)
    {
        printf("open error");
        exit(1);
    }
    
    //创建写线程
    pthread_t writeThread;
    int ret = pthread_create(&writeThread, NULL, writeThreadFun, &fd);
    
    //创建一个event_base
    struct event_base* base = NULL;
    base = event_base_new();
    
    //创建事件
    struct event* ev = NULL;
    ev = event_new(base, fd, EV_READ| EV_PERSIST, read_cb, NULL);
    
    //添加事件  为NULL表示不会超时
    event_add(ev, NULL);
    
    //事件循环
    event_base_dispatch(base);
    
    //释放资源
    event_free(ev);
    event_base_free(base);
    close(fd);//关闭串口
}