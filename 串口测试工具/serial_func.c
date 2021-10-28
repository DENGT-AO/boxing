/*
*/
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

////////////////////////////////////////////////////////////////////////////////////////////
//		serial device IO
////////////////////////////////////////////////////////////////////////////////////////////
unsigned int serial_get_baudrate(unsigned int baudRate)
{
	unsigned int BaudR = 0;

	switch(baudRate) {
	case 460800:
		BaudR=B460800;
		break;
	case 230400:
		BaudR=B230400;
		break;
	case 115200:
		BaudR=B115200;
		break;
	case 57600:
		BaudR=B57600;
		break;
	case 38400:
		BaudR=B38400;
		break;
	case 19200:
		BaudR=B19200;
		break;
	case 9600:
		BaudR=B9600;
		break;
	case 4800:
		BaudR=B4800;
		break;
	case 2400:
		BaudR=B2400;
		break;
	case 1200:
		BaudR=B1200;
		break;
	case 600:
		BaudR=B600;
		break;
	case 300:
		BaudR=B300;
		break;
	case 110:
		BaudR=B110;
		break;

	default:
		BaudR=B19200;
	}

	return BaudR;
}

int serial_set_speed(int fd, int speed)
{ 
	int   status;
	struct termios   Opt;

	tcgetattr(fd, &Opt);

	cfsetispeed(&Opt, serial_get_baudrate(speed));
	cfsetospeed(&Opt, serial_get_baudrate(speed));
	status = tcsetattr(fd, TCSANOW, &Opt);

	if  (status != 0){
		printf("set serial baudrate failed! error=(%d,%s)\n", 
			errno, strerror(errno));
		return -1;
	}

	tcflush(fd,TCIOFLUSH);

	return 0;
}

int serial_set_parity(int fd,int databits,int stopbits,int parity, int crtscts, int xonoff)
{ 
	struct termios options;

	if( tcgetattr( fd,&options)  !=  0) {
		printf("set serial parity failed! Cannot get attribute! error=(%d,%s)\n", 
			errno, strerror(errno));
		return -1;
	}

	//cfmakeraw(&options);
	options.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
			|INLCR|IGNCR|ICRNL|IXON);
	if(xonoff) options.c_iflag |= IXON;
	options.c_oflag = OPOST;
	options.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
	options.c_cflag &= ~(CSIZE|PARENB);
	//options.c_cflag |= CS8;

	options.c_cflag &= ~CSIZE;

	switch (databits) { 
	case 5:
		options.c_cflag |= CS5;
		break;
	case 6:
		options.c_cflag |= CS6;
		break;
	case 7:
		options.c_cflag |= CS7;
		//options.c_cflag &= ~CS8;
		break;
	case 8:
		options.c_cflag |= CS8;
		//options.c_cflag &= ~CS7;
		break;
	default:
		printf("Bad data bit : %d\n", databits);
		options.c_cflag |= CS8;
	}

	switch (parity) { 
	case 'n':
	case 'N':
		options.c_cflag &= ~PARENB;   /* Clear parity enable */
		options.c_iflag &= ~INPCK;     /* disable parity checking */
		break;
	case 'o':
	case 'O':
		options.c_cflag |= (PARODD | PARENB);  /* odd */ 
		options.c_iflag |= INPCK;             /* Enable parity checking */
		break;
	case 'e':
	case 'E':
		options.c_cflag |= PARENB;     /* Enable parity */
		options.c_cflag &= ~PARODD;   /* even */  
		options.c_iflag |= INPCK;       /* Enable parity checking */
		break;
	case 'S':
	case 's':  /*as no parity*/
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;   //FIXME: why clear stop bit?
		options.c_iflag &= ~INPCK;     /* disable parity checking */
		break;
	default:
		printf("Bad parity : %c\n", parity);
		options.c_cflag &= ~PARENB;   /* Clear parity enable */
		options.c_iflag &= ~INPCK;     /* disable parity checking */
	}

	switch (stopbits) { 
	case 1:
		options.c_cflag &= ~CSTOPB;
		break;
	case 2:
		options.c_cflag |= CSTOPB;
		break;
	default:
		printf("Bad stop bit : %c\n", stopbits);
		options.c_cflag &= ~CSTOPB;
	}

	options.c_cc[VTIME] = 0; // 15 seconds
	options.c_cc[VMIN] = 1;

	if(crtscts) options.c_cflag |= CRTSCTS; //enable hw flow control
	else options.c_cflag &= ~CRTSCTS; //disable hw flow control

	options.c_cflag |= CLOCAL;

	tcflush(fd,TCIFLUSH); /* Update the options and do it NOW */

	if (tcsetattr(fd,TCSANOW,&options) != 0){
		printf("set serial parity failed! Cannot set attribute! error=(%d,%s)\n", 
			errno, strerror(errno));
		return -5;
	}

	return 0;
}

int uart_init(char *dev, int nSpeed, int nBits, char nEvent, int nStop)
{
    int fd = 0;
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
