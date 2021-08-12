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

#include <event2/event.h>

//串口名称
#define UART_DEV "/dev/ttyUSB0"

//编译方法：gcc -o test test.c -levent -lpthread
/*
        #include <termios.h>
        struct termios{
            tcflag_t  c_iflag;  //输入模式标志
            tcflag_t  c_oflag;  //输出模式标志
            tcflag_t  c_cflag;  //控制选项
            tcflag_t  c_lflag;  //行选项
            cc_t      c_cc[NCCS]; //控制字符
        }
        每个选项都是16位数,每一位都有其含义
        .c_iflag：输入模式标志，控制终端输入方式
                键 值             说 明
                IGNBRK          忽略BREAK键输入
                BRKINT          如果设置了IGNBRK，BREAK键的输入将被忽略，如果设置了BRKINT ，将产生SIGINT中断
                IGNPAR          忽略奇偶校验错误
                PARMRK          标识奇偶校验错误
                INPCK           允许输入奇偶校验
                ISTRIP          去除字符的第8个比特
                INLCR           将输入的NL（换行）转换成CR（回车）
                IGNCR           忽略输入的回车
                ICRNL           将输入的回车转化成换行（如果IGNCR未设置的情况下）
                IUCLC           将输入的大写字符转换成小写字符（非POSIX）
                IXON            允许输入时对XON/XOFF流进行控制
                IXANY           输入任何字符将重启停止的输出
                IXOFF           允许输入时对XON/XOFF流进行控制
                IMAXBEL         当输入队列满的时候开始响铃，Linux在使用该参数而是认为该参数总是已经设置
        .c_oflag：输出模式标志，控制终端输出方式
                键 值             说 明
                OPOST           处理后输出
                OLCUC           将输入的小写字符转换成大写字符（非POSIX）
                ONLCR           将输入的NL（换行）转换成CR（回车）及NL（换行）
                OCRNL           将输入的CR（回车）转换成NL（换行）
                ONOCR           第一行不输出回车符
                ONLRET          不输出回车符
                OFILL           发送填充字符以延迟终端输出
                OFDEL           以ASCII码的DEL作为填充字符，如果未设置该参数，填充字符将是NUL（'\0'）（非POSIX）
                NLDLY           换行输出延时，可以取NL0（不延迟）或NL1（延迟0.1s）
                CRDLY           回车延迟，取值范围为：CR0、CR1、CR2和 CR3
                TABDLY          水平制表符输出延迟，取值范围为：TAB0、TAB1、TAB2和TAB3
                BSDLY           空格输出延迟，可以取BS0或BS1
                VTDLY           垂直制表符输出延迟，可以取VT0或VT1
                FFDLY           换页延迟，可以取FF0或FF1
        .c_cflag：控制模式标志，指定终端硬件控制信息
                键 值             说 明
                CBAUD           波特率（4+1位）（非POSIX）
                CBAUDEX         附加波特率（1位）（非POSIX）
                CSIZE           字符长度，取值范围为CS5、CS6、CS7或CS8
                CSTOPB          设置两个停止位
                CREAD           使用接收器
                PARENB          使用奇偶校验
                PARODD          对输入使用奇偶校验，对输出使用偶校验
                HUPCL           关闭设备时挂起
                CLOCAL          忽略调制解调器线路状态
                CRTSCTS         使用RTS/CTS流控制
        .c_lflag：本地模式标志，控制终端编辑功能
                 键 值             说 明
                ISIG            当输入INTR、QUIT、SUSP或DSUSP时，产生相应的信号
                ICANON          使用标准输入模式
                XCASE           在ICANON和XCASE同时设置的情况下，终端只使用大写。如果只设置了XCASE，则输入字符将被转换为小写字符，除非字符使用了转义字符（非POSIX，且Linux不支持该参数）
                ECHO            显示输入字符
                ECHOE           如果ICANON同时设置，ERASE将删除输入的字符，WERASE将删除输入的单词
                ECHOK           如果ICANON同时设置，KILL将删除当前行
                ECHONL          如果ICANON同时设置，即使ECHO没有设置依然显示换行符
                ECHOPRT         如果ECHO和ICANON同时设置，将删除打印出的字符（非POSIX）
                TOSTOP          向后台输出发送SIGTTOU信号
        .c_cc[NCCS]：控制字符，用于保存终端驱动程序中的特殊字符
            只有在本地模式标志c_lflag中设置了IEXITEN时，POSIX没有定义的控制字符才能在Linux中使用。每个控制字符都对应一个按键组合（^C、^H等）。
            VMIN和VTIME这两个控制字符除外，它们不对应控制符。这两个控制字符只在原始模式下才有效。
                键 值             说 明
                c_cc[VMIN]      原始模式（非标准模式）读的最小字符数
                c_cc[VTIME]     原始模式（非标准模式）读时的延时，以十分之一秒为单位

                c_cc[VINTR]     默认对应的控制符是^C，作用是清空输入和输出队列的数据并且向tty设备的前台进程组中的每一个程序发送一个SIGINT信号，对SIGINT信号没有定义处理程序的进程会马上退出。
                c_cc[VQUIT]     默认对应的控制符是^/，作用是清空输入和输出队列的数据并向tty设备的前台进程组中的每一个程序发送一个SIGQUIT信号，对SIGQUIT 信号没有定义处理程序的进程会马上退出。
                c_cc[verase]    默认对应的控制符是^H或^?，作用是在标准模式下，删除本行前一个字符，该字符在原始模式下没有作用。
                c_cc[VKILL]     默认对应的控制符是^U，在标准模式下，删除整行字符，该字符在原始模式下没有作用。
                c_cc[VEOF]      默认对应的控制符是^D，在标准模式下，使用read()返回0，标志一个文件结束。
                c_cc[VSTOP]     默认对应的控制字符是^S，作用是使用tty设备暂停输出直到接收到VSTART控制字符。或者，如果设备了IXANY，则等收到任何字符就开始输出。
                c_cc[VSTART]    默认对应的控制字符是^Q，作用是重新开始被暂停的tty设备的输出。
                c_cc[VSUSP]     默认对应的控制字符是^Z，使当前的前台进程接收到一个SIGTSTP信号。
                c_cc[VEOL]
                c_cc[VEOL2]     在标准模式下，这两个下标在行的末尾加上一个换行符（'/n'），标志一个行的结束，从而使用缓冲区中的数据被发送，并开始新的一行。POSIX中没有定义VEOL2。
                c_cc[VREPRINT]  默认对应的控制符是^R，在标准模式下，如果设置了本地模式标志ECHO，使用VERPRINT对应的控制符和换行符在本地显示，并且重新打印当前缓冲区中的字符。POSIX中没有定义VERPRINT。
                c_cc[VWERASE]   默认对应的控制字符是^W，在标准模式下，删除缓冲区末端的所有空格符，然后删除与之相邻的非空格符，从而起到在一行中删除前一个单词的效果。 POSIX中没有定义VWERASE。
                c_cc[VLNEXT]    默认对应的控制符是^V，作用是让下一个字符原封不动地进入缓冲区。如果要让^V字符进入缓冲区，需要按两下^V。POSIX中没有定义 VLNEXT。
            
        
*/

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
    printf("%s", buf);
}

int main(int argc, char **argv)
{
    int ret = 0;
    int fd = 0;
    pthread_t writeThread;
    struct event_base *base;
    struct event *readevent;
    
    
    //设置串口属性包括波特率、校验位、停止位等
    fd = uart_init(UART_DEV, 115200, 8, 'N', 1);
    if (fd == -1)
    {
        printf("UART INIT ERROR!\n");
        exit(-1);
    }

    //创建写线程
    writeThread = pthread_create(&writeThread, NULL, writefunc, &fd);
    if(ret != 0)
    {
        printf("create pthread error!\n");
        goto ERR1;
    }

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
    close(fd);
    return 0;
}