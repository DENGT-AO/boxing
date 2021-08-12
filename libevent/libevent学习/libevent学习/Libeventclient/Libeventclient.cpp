// Libeventclient.cpp : �������̨Ӧ�ó������ڵ㡣
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
	if (what & BEV_EVENT_EOF) //�����ļ�����ָʾ
	{
		printf("Connection closed.\n");
	}
	else if (what & BEV_EVENT_ERROR) //������������
	{
		char sz[100] = { 0 };
		strerror_s(sz, 100, errno);
		printf("Got an error on the connection: %s\n",sz);
	}
	else if (what & BEV_EVENT_TIMEOUT) //��ʱ
	{
		printf("Connection timeout.\n");
	}
	else if (what & BEV_EVENT_CONNECTED) //�����Ѿ����
	{
		printf("connect succeed.\n");

		//����һ��ͼƬ���в���

		WIN32_FIND_DATA FileInfo;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		DWORD FileSize = 0;                   //�ļ���С

		char buf[10001] = {0};
		char *pbuf = NULL;
		ZeroMemory(&FileInfo,sizeof(WIN32_FIND_DATA));

		//��ȡ�ļ���С
		hFind = FindFirstFile("test.png",&FileInfo); 
		if(hFind != INVALID_HANDLE_VALUE) 
		{
			FileSize = FileInfo.nFileSizeLow  + sizeof(PICTUREINFO);
		}
		FindClose(hFind);

		strcpy_s(((PICTUREINFO*)buf)->szFileName,"test.png");
		((PICTUREINFO*)buf)->nFileSize = FileSize;

		//��һ�η���10000�����ļ�ͷ
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
	//��ʼ�������
#ifdef WIN32
	WSADATA wsa_data;
	WSAStartup(0x0201, &wsa_data);
#endif

	//����base
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

	//���ûص�����
	bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);

	//���ӷ�����
	bufferevent_socket_connect(bev, (struct sockaddr*)&addr, sizeof(addr));

	bufferevent_enable(bev, EV_READ | EV_WRITE);

	//����bufferevent,����ͨ�ŵ�fd�ŵ�bufferevent��
	
	

	event_base_dispatch(base);

	event_base_free(base);
	WSACleanup();

	return 0;
}

