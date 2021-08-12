// Libeventclient.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "winsock2.h"
#include "event2/listener.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"

typedef struct PictureInfo
{
	char szFileName[260];
	long nFileSize;
} PICTUREINFO;


void read_cb(struct bufferevent *bev, void *ctx)
{
}
 
void write_cb(struct bufferevent *bev, void *ctx)
{
}
 
void event_cb(struct bufferevent *bev, short what, void *ctx)
{
	if (what & BEV_EVENT_EOF) //遇到文件结束指示
	{
		printf("Connection closed.\n");
	}
	else if (what & BEV_EVENT_ERROR) //操作发生错误
	{
		char sz[100] = { 0 };
		strerror_s(sz, 100, errno);
		printf("Got an error on the connection: %s\n",sz);
	}
	else if (what & BEV_EVENT_TIMEOUT) //超时
	{
		printf("Connection timeout.\n");
	}
	else if (what & BEV_EVENT_CONNECTED) //连接已经完成
	{
		printf("connect succeed.\n");

		//发送一张图片进行测试

		WIN32_FIND_DATA FileInfo;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		DWORD FileSize = 0;                   //文件大小

		char buf[10001] = {0};
		char *pbuf = NULL;
		ZeroMemory(&FileInfo,sizeof(WIN32_FIND_DATA));

		//获取文件大小
		hFind = FindFirstFile("test.png",&FileInfo); 
		if(hFind != INVALID_HANDLE_VALUE) 
		{
			FileSize = FileInfo.nFileSizeLow  + sizeof(PICTUREINFO);
		}
		FindClose(hFind);

		strcpy_s(((PICTUREINFO*)buf)->szFileName,"test.png");
		((PICTUREINFO*)buf)->nFileSize = FileSize;

		//第一次发送10000，带文件头
		FILE *f = NULL;
		fopen_s(&f,"test.png","rb");
		fread(buf + sizeof(PICTUREINFO),4906 - sizeof(PICTUREINFO),1,f);
		bufferevent_write(bev,buf,4906);
		FileSize -= 4906;

		while (FileSize >= 4906)
		{
			fread(buf,4906,1,f);
			bufferevent_write(bev,buf,4906);

			FileSize -= 4906;
		}

		if (FileSize > 0)
		{
			fread(buf,FileSize,1,f);
			bufferevent_write(bev,buf,FileSize);
		
			FileSize -= FileSize;
		}

		if (f)
		{
			fclose(f);
			f = NULL;
		}
		return;
	}
	/* None of the other events can happen here, since we haven't enabled
	* timeouts */
	bufferevent_free(bev);
}

int _tmain(int argc, _TCHAR* argv[])
{
	//初始化网络库
#ifdef WIN32
	WSADATA wsa_data;
	WSAStartup(0x0201, &wsa_data);
#endif

	//创建base
	struct event_base *base;
	base = event_base_new();

	SOCKET fd = socket(AF_INET, SOCK_STREAM, 0);
	struct bufferevent *bev = {NULL};
	bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

	struct sockaddr_in addr = { 0 };
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//addr.sin_addr.s_addr = htonl(0x7f000001);
	//inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
	addr.sin_port = htons(8888);

	//设置回调函数
	bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);

	//连接服务器
	bufferevent_socket_connect(bev, (struct sockaddr*)&addr, sizeof(addr));

	bufferevent_enable(bev, EV_READ | EV_WRITE);

	//创建bufferevent,并将通信的fd放到bufferevent中
	
	

	event_base_dispatch(base);

	event_base_free(base);
	WSACleanup();

	return 0;
}

