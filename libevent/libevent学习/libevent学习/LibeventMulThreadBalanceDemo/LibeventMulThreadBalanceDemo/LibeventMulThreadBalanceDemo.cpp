// LibeventMulThreadBalanceDemo.cpp : 定义控制台应用程序的入口点。
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

const int g_nThreadNum = 10;   //开启的线程数量

typedef struct Libevent_Thread
{
	DWORD did;                //子线程ID
	struct event_base *base;  //子线程base
	struct event event;       //子线程event
	evutil_socket_t read_fd;  //读管道描述符
	evutil_socket_t write_fd; //写管道描述符
} LIBEVENT_THREAD;

typedef struct Dispatcher_Thread
{
	DWORD did;                //主线程ID
	struct event_base *base;  //主线程base
} DISPATCHER_THREAD;

LIBEVENT_THREAD *g_pThreads = new LIBEVENT_THREAD[g_nThreadNum];

DISPATCHER_THREAD g_DispatcherThread;

int g_nlastThread = 0;

typedef struct PictureInfo
{
	char szFileName[260];
	long nFileSize;
} PICTUREINFO;

//读缓冲去回调函数
void read_cb(struct bufferevent *bev, void *arg)
{
	BufferManager* bm = (BufferManager*)arg;

	//第一次接收
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
		printf("收到的字节数: %d ,nThreadID = %d\n",bm->nReceiveTotal,GetCurrentThreadId());
		bm->iniParam();
	}
}

//写缓冲区回调函数
void write_cb(struct bufferevent *bev, void *arg)
{
	printf("成功写数据给客户端,写缓冲区回调函数被回调.\n");
}

//事件回调函数
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

	//注销事件导致事件循环退出，这样子线程也将退出
	bufferevent_free(bev);

	if (pThis)
	{
		delete pThis;
		pThis = NULL;
	}

	printf("bufferevent 资源已经被释放.\n");
}

//开启子线程的事件循环
unsigned __stdcall Work_Thread( void* pArguments )
{
	LIBEVENT_THREAD *pThis = (LIBEVENT_THREAD*)pArguments;
	
	event_base_dispatch(pThis->base);

	_endthreadex(0);
	return 1;
}

int nNum = 0;

//管道读回调函数
void pipe_process(int fd, short which, void *arg)
{
	LIBEVENT_THREAD* pThis = (LIBEVENT_THREAD*)arg;

	//获取管道的读取描述符
	int readfd = pThis->read_fd;
	evutil_socket_t evsock;
	recv(readfd, (char*)&evsock, sizeof(evutil_socket_t), 0);

	//为新的连接关联事件
	struct bufferevent* bev;
	bev = bufferevent_socket_new(pThis->base, evsock, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);

	BufferManager *bm = new BufferManager;

	//给bufferevent缓冲区设置回调
	bufferevent_setcb(bev, 
		read_cb,
		write_cb,
		event_cb,
		NULL);

	//启动bufferevent的读缓冲区，读缓冲区默认是disable的.
	bufferevent_enable(bev, EV_READ);

	printf("第 %d 个连接，线程ID = %d\n",nNum++,GetCurrentThreadId());
}

//主线程监听器回调函数
void cb_listener(struct evconnlistener* listener, 
				evutil_socket_t fd, 
			    struct sockaddr *addr, 
				int len, 
				void *ptr)
{
	//printf("new client connect.\n");

	//采用负载均衡算法为当前连接选择子线程
	int nCurThread = g_nlastThread % g_nThreadNum;
	g_nlastThread = nCurThread + 1;
	int sendfd = g_pThreads[nCurThread].write_fd;

	//将fd传给子线程
	send(sendfd, (char*)&fd, sizeof(evutil_socket_t), 0);
}

int _tmain(int argc, _TCHAR* argv[])
{
	//初始化网络库
#ifdef WIN32
	evthread_use_windows_threads();

	WSADATA wsa_data;
	WSAStartup(0x0201, &wsa_data);
#endif

	//为每个子线程的事件绑定管道的读写事件，子线程通过管道与主线程进行通信
	int nRet(0);
	for (int i = 0;i < g_nThreadNum;i++)
	{
		evutil_socket_t fds[2];
		if(evutil_socketpair(AF_INET, SOCK_STREAM, 0, fds) < 0) 
		{
			printf("create socketpair error,g_nThreadNum = %d\n",g_nThreadNum);
			return false;
		}

		//设置成无阻塞的socket
		evutil_make_socket_nonblocking(fds[0]);
		evutil_make_socket_nonblocking(fds[1]);

		g_pThreads[i].read_fd = fds[0];
		g_pThreads[i].write_fd = fds[1];

		//创建子线程的base
		g_pThreads[i].base = event_base_new();
		if (g_pThreads[i].base == NULL)
		{
			printf("event_base_new error,g_nThreadNum = %d\n",g_nThreadNum);
			return 0;
		}

		//将文件描述符和事件进行绑定,并加入到base中
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

	//创建子线程，并启动子线程的事件循环，在有注册事件(管道的读事件)的情况下循环不会退出
	for (int i = 0;i < g_nThreadNum;i++)
	{
		 _beginthreadex( NULL, 0, &Work_Thread, (void*)&g_pThreads[i], 0, (unsigned int*)&g_pThreads[i].did );
	}

	//初始化服务器地址结构
	struct sockaddr_in sSerAddr;
	memset(&sSerAddr, 0, sizeof(sSerAddr));
	sSerAddr.sin_family = AF_INET;
	sSerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sSerAddr.sin_port = htons(8888);

	//创建主线程base
	g_DispatcherThread.base = event_base_new();
	if (g_DispatcherThread.base == NULL)
	{
		printf("g_DispatcherThread.base create error.\n");
		return 0;
	}

	//创建监听器
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

	//开启主线程的事件循环
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

