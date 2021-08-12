// LibeventMulThreadBalanceDemo.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "winsock2.h"
#include <process.h> 
#include <io.h>
#include <fcntl.h>
#include "event2/listener.h"
#include "event2/bufferevent.h"
#include "event2/thread.h"
#include "event.h"
#include "BufferManager.h"
#include "Windows.h"

const int g_nThreadNum = 10;   //�������߳�����

typedef struct Libevent_Thread
{
	DWORD did;                //���߳�ID
	struct event_base *base;  //���߳�base
	struct event event;       //���߳�event
	evutil_socket_t read_fd;  //���ܵ�������
	evutil_socket_t write_fd; //д�ܵ�������
} LIBEVENT_THREAD;

typedef struct Dispatcher_Thread
{
	DWORD did;                //���߳�ID
	struct event_base *base;  //���߳�base
} DISPATCHER_THREAD;

LIBEVENT_THREAD *g_pThreads = new LIBEVENT_THREAD[g_nThreadNum];

DISPATCHER_THREAD g_DispatcherThread;

int g_nlastThread = 0;

typedef struct PictureInfo
{
	char szFileName[260];
	long nFileSize;
} PICTUREINFO;

//������ȥ�ص�����
void read_cb(struct bufferevent *bev, void *arg)
{
	BufferManager* bm = (BufferManager*)arg;

	//��һ�ν���
	if (bm->nFileSize == 0)
	{
		int nReceived = bufferevent_read(bev, bm->buf,10000);
		bm->nReceiveTotal += nReceived;

		if (nReceived >= sizeof(PICTUREINFO))
		{
			bm->nFileSize = ((PICTUREINFO*)bm->buf)->nFileSize;
			strcpy_s(bm->szImgName,sizeof(bm->szImgName),((PICTUREINFO*)bm->buf)->szFileName);

			bm->f = NULL;
			fopen_s(&bm->f,bm->szImgName,"wb");
			if (fwrite(bm->buf+sizeof(PICTUREINFO),bm->nReceiveTotal - sizeof(PICTUREINFO),1,bm->f) < 1){
				// write error
			}
		}
	}
	else if ((bm->nFileSize - bm->nReceiveTotal) >= 10000)
	{
		int nReceived = bufferevent_read(bev, bm->buf,10000);
		bm->nReceiveTotal += nReceived;

		if (fwrite(bm->buf,nReceived,1,bm->f) < 1){
			// write error
		}
	}
	else if((bm->nFileSize - bm->nReceiveTotal) >= 0)
	{
		int nReceived = bufferevent_read(bev, bm->buf, bm->nFileSize-bm->nReceiveTotal);
		bm->nReceiveTotal += nReceived;

		if (fwrite(bm->buf,nReceived,1,bm->f) < 1){
			// write error
		}
	}

	if (bm->nReceiveTotal == bm->nFileSize)
	{
		printf("�յ����ֽ���: %d ,nThreadID = %d\n",bm->nReceiveTotal,GetCurrentThreadId());
		bm->iniParam();
	}
}

//д�������ص�����
void write_cb(struct bufferevent *bev, void *arg)
{
	printf("�ɹ�д���ݸ��ͻ���,д�������ص��������ص�.\n");
}

//�¼��ص�����
void event_cb(struct bufferevent *bev,short events, void *arg)
{
	BufferManager* pThis = (BufferManager*)arg;

	if (events & BEV_EVENT_EOF)
	{
		printf("connection close.\n");
	} 
	else if(events & BEV_EVENT_ERROR)
	{
		printf("some other error.\n");
	}

	//ע���¼������¼�ѭ���˳����������߳�Ҳ���˳�
	bufferevent_free(bev);

	if (pThis)
	{
		delete pThis;
		pThis = NULL;
	}

	printf("bufferevent ��Դ�Ѿ����ͷ�.\n");
}

//�������̵߳��¼�ѭ��
unsigned __stdcall Work_Thread( void* pArguments )
{
	LIBEVENT_THREAD *pThis = (LIBEVENT_THREAD*)pArguments;
	
	event_base_dispatch(pThis->base);

	_endthreadex(0);
	return 1;
}

int nNum = 0;

//�ܵ����ص�����
void pipe_process(int fd, short which, void *arg)
{
	LIBEVENT_THREAD* pThis = (LIBEVENT_THREAD*)arg;

	//��ȡ�ܵ��Ķ�ȡ������
	int readfd = pThis->read_fd;
	evutil_socket_t evsock;
	recv(readfd, (char*)&evsock, sizeof(evutil_socket_t), 0);

	//Ϊ�µ����ӹ����¼�
	struct bufferevent* bev;
	bev = bufferevent_socket_new(pThis->base, evsock, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);

	BufferManager *bm = new BufferManager;

	//��bufferevent���������ûص�
	bufferevent_setcb(bev, 
		read_cb,
		write_cb,
		event_cb,
		NULL);

	//����bufferevent�Ķ�����������������Ĭ����disable��.
	bufferevent_enable(bev, EV_READ);

	printf("�� %d �����ӣ��߳�ID = %d\n",nNum++,GetCurrentThreadId());
}

//���̼߳������ص�����
void cb_listener(struct evconnlistener* listener, 
				evutil_socket_t fd, 
			    struct sockaddr *addr, 
				int len, 
				void *ptr)
{
	//printf("new client connect.\n");

	//���ø��ؾ����㷨Ϊ��ǰ����ѡ�����߳�
	int nCurThread = g_nlastThread % g_nThreadNum;
	g_nlastThread = nCurThread + 1;
	int sendfd = g_pThreads[nCurThread].write_fd;

	//��fd�������߳�
	send(sendfd, (char*)&fd, sizeof(evutil_socket_t), 0);
}

int _tmain(int argc, _TCHAR* argv[])
{
	//��ʼ�������
#ifdef WIN32
	evthread_use_windows_threads();

	WSADATA wsa_data;
	WSAStartup(0x0201, &wsa_data);
#endif

	//Ϊÿ�����̵߳��¼��󶨹ܵ��Ķ�д�¼������߳�ͨ���ܵ������߳̽���ͨ��
	int nRet(0);
	for (int i = 0;i < g_nThreadNum;i++)
	{
		evutil_socket_t fds[2];
		if(evutil_socketpair(AF_INET, SOCK_STREAM, 0, fds) < 0) 
		{
			printf("create socketpair error,g_nThreadNum = %d\n",g_nThreadNum);
			return false;
		}

		//���ó���������socket
		evutil_make_socket_nonblocking(fds[0]);
		evutil_make_socket_nonblocking(fds[1]);

		g_pThreads[i].read_fd = fds[0];
		g_pThreads[i].write_fd = fds[1];

		//�������̵߳�base
		g_pThreads[i].base = event_base_new();
		if (g_pThreads[i].base == NULL)
		{
			printf("event_base_new error,g_nThreadNum = %d\n",g_nThreadNum);
			return 0;
		}

		//���ļ����������¼����а�,�����뵽base��
		event_set(&g_pThreads[i].event,
			      g_pThreads[i].read_fd,
				  EV_READ | EV_PERSIST,
				  pipe_process,
				  &g_pThreads[i]);

		nRet = event_base_set(g_pThreads[i].base, &g_pThreads[i].event);
		nRet = event_add(&g_pThreads[i].event,NULL);
		if (nRet == -1)
		{
			printf("event_add error,g_nThreadNum = %d\n",g_nThreadNum);
			return 0;
		}
	}

	//�������̣߳����������̵߳��¼�ѭ��������ע���¼�(�ܵ��Ķ��¼�)�������ѭ�������˳�
	for (int i = 0;i < g_nThreadNum;i++)
	{
		 _beginthreadex( NULL, 0, &Work_Thread, (void*)&g_pThreads[i], 0, (unsigned int*)&g_pThreads[i].did );
	}

	//��ʼ����������ַ�ṹ
	struct sockaddr_in sSerAddr;
	memset(&sSerAddr, 0, sizeof(sSerAddr));
	sSerAddr.sin_family = AF_INET;
	sSerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sSerAddr.sin_port = htons(8888);

	//�������߳�base
	g_DispatcherThread.base = event_base_new();
	if (g_DispatcherThread.base == NULL)
	{
		printf("g_DispatcherThread.base create error.\n");
		return 0;
	}

	//����������
	struct evconnlistener *listener;
	listener = evconnlistener_new_bind(g_DispatcherThread.base, 
		cb_listener, 
		g_DispatcherThread.base, 
		LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, 
		-1, 
		(struct sockaddr*)&sSerAddr, 
		sizeof(sSerAddr));

	if (listener == NULL)
	{
		printf("evconnlistener_new_bind error.\n");
		return 0;
	}

	//�������̵߳��¼�ѭ��
	event_base_dispatch(g_DispatcherThread.base);

	for (int i = 0;i < g_nThreadNum;i++)
	{
		event_base_free(g_pThreads[i].base);
	}

	if (g_pThreads)
	{
		delete []g_pThreads;
		g_pThreads = NULL;
	}

	evconnlistener_free(listener);
	event_base_free(g_DispatcherThread.base);
	WSACleanup();

	return 0;
}

