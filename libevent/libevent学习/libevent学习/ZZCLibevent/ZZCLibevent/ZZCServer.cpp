#include "StdAfx.h"
#include "ZZCServer.h"

typedef struct PictureInfo
{
	char szFileName[260];
	long nFileSize;
} PICTUREINFO;

ZZCServer::ZZCServer(void):m_hThread(INVALID_HANDLE_VALUE),m_sWorkThreadNum(10),m_uItemNum(10)
	,m_sPort(8888),m_nReadTimeout(60),m_nWriteTimeout(60),m_nCurrentThread(0)
{
	WSADATA WSAData;
	WSAStartup(0x0201, &WSAData);

	evthread_use_windows_threads();
}


ZZCServer::~ZZCServer(void)
{
	WSACleanup();
}

void ZZCServer::Read_cb(struct bufferevent *bev, void *param)
{
	Client_Item* pThis = (Client_Item*)param;

	if (pThis->nFileSize == 0)
	{
		int nReceived = bufferevent_read(bev, pThis->in_buf,BufferLen);
		pThis->nReceiveTotal += nReceived;

		if (nReceived >= sizeof(PICTUREINFO))
		{
			pThis->nFileSize = ((PICTUREINFO*)pThis->in_buf)->nFileSize;
			strcpy_s(pThis->szImgName,sizeof(pThis->szImgName),((PICTUREINFO*)pThis->in_buf)->szFileName);

			pThis->f = NULL;
			fopen_s(&pThis->f,pThis->szImgName,"wb");
			if (fwrite(pThis->in_buf+sizeof(PICTUREINFO),pThis->nReceiveTotal - sizeof(PICTUREINFO),1,pThis->f) < 1){
				pThis->clearparam();
			}
		}
	}
	else if ((pThis->nFileSize - pThis->nReceiveTotal) >= BufferLen)
	{
		int nReceived = bufferevent_read(bev, pThis->in_buf,BufferLen);
		pThis->nReceiveTotal += nReceived;

		if (fwrite(pThis->in_buf,nReceived,1,pThis->f) < 1){
			pThis->clearparam();
		}
	}
	else if((pThis->nFileSize - pThis->nReceiveTotal) >= 0)
	{
		int nReceived = bufferevent_read(bev, pThis->in_buf, pThis->nFileSize-pThis->nReceiveTotal);
		pThis->nReceiveTotal += nReceived;

		if (fwrite(pThis->in_buf,nReceived,1,pThis->f) < 1){
			pThis->clearparam();
		}
	}

	if (pThis->nReceiveTotal == pThis->nFileSize)
	{
		printf("收到的字节数: %d ,nThreadID = %d\n",pThis->nReceiveTotal,GetCurrentThreadId());
		pThis->clearparam();
	}
}

void ZZCServer::Error_cb(struct bufferevent *bev, short error, void *param)
{
	Client_Item* pThis = (Client_Item*)param;

	int nFinished(0);

	if (error & BEV_EVENT_TIMEOUT) 
	{
		nFinished = 1;
	} 
	else if (error & BEV_EVENT_EOF)
	{
		nFinished = 1;
	} 
	else if (error & BEV_EVENT_ERROR) 
	{
		nFinished = 1;
	}

	if (nFinished == 1)
	{
		bufferevent_free(bev);
		pThis->pSubThread->PopItem(pThis->fd);

		if (pThis)
		{
			delete pThis;
			pThis = NULL;
		}

		printf("%d 线程中的客户端退出\n",GetCurrentThreadId());
	}
}

unsigned __stdcall ZZCServer::SubThreadEventDispatch(void* param)
{
	Sub_Thread* pThis = (Sub_Thread*)param;

	event_base_dispatch(pThis->pbase);

	_endthreadex(0);
	return 1;
}

int nNum(0);
void ZZCServer::pipe_process(int fd, short which, void *arg)
{
	Sub_Thread* pThis = (Sub_Thread*)arg;

	int readfd = pThis->read_fd;
	evutil_socket_t evsock;
	recv(readfd, (char*)&evsock, sizeof(evutil_socket_t), 0);

	Client_Item* itemtemp = new Client_Item;
	itemtemp->fd = evsock;
	itemtemp->pSubThread = pThis;
	itemtemp->bev = bufferevent_socket_new(pThis->pbase, evsock, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
	evutil_make_socket_nonblocking(evsock);
	pThis->itemlist.push_back(itemtemp);

	bufferevent_setcb(itemtemp->bev, 
		Read_cb,
		NULL,
		Error_cb,
		itemtemp);

	/*struct timeval delayWriteTimeout;
	delayWriteTimeout.tv_sec = pThis->pServer->m_nWriteTimeout;
	delayWriteTimeout.tv_usec = 0;
	struct timeval delayReadTimeout;
	delayReadTimeout.tv_sec = pThis->pServer->m_nReadTimeout;
	delayReadTimeout.tv_usec = 0;
	bufferevent_set_timeouts(itemtemp->bev,&delayReadTimeout,&delayWriteTimeout);*/
	bufferevent_setwatermark(itemtemp->bev, EV_READ, 0, BufferLen);
	bufferevent_enable(itemtemp->bev, EV_READ);

	printf("第 %d 个连接，线程ID = %d\n",++nNum,GetCurrentThreadId());
}

bool ZZCServer::RunSubThreadCb()
{
	m_pSubThread = new Sub_Thread[m_sWorkThreadNum];

	for (int i = 0;i < m_sWorkThreadNum;i++)
	{
		m_pSubThread[i].pbase = event_base_new();

		if (m_pSubThread[i].pbase == NULL)
		{
			if (m_pSubThread)
			{
				delete[] m_pSubThread;
				m_pSubThread = NULL;
			}

			return false;
		}

		evutil_socket_t fds[2];
		if(evutil_socketpair(AF_INET, SOCK_STREAM, 0, fds) < 0) 
		{
			return false;
		}

		evutil_make_socket_nonblocking(fds[0]);
		evutil_make_socket_nonblocking(fds[1]);

		m_pSubThread[i].read_fd  = fds[0];
		m_pSubThread[i].write_fd = fds[1];

		event_set(&m_pSubThread[i].event,
			m_pSubThread[i].read_fd,
			EV_READ | EV_PERSIST,
			pipe_process,
			&m_pSubThread[i]);

		int nRet(0);
		nRet = event_base_set(m_pSubThread[i].pbase, &m_pSubThread[i].event);
		nRet = event_add(&m_pSubThread[i].event,NULL);
		if (nRet == -1)
		{
			return 0;
		}

		m_pSubThread[i].pServer = this;

		m_pSubThread[i].hThread = (HANDLE)_beginthreadex( NULL, 0, &SubThreadEventDispatch, (void*)&m_pSubThread[i], 0, NULL);
		if (m_pSubThread[i].hThread == INVALID_HANDLE_VALUE)
		{
			if (m_pSubThread)
			{
				delete[] m_pSubThread;
				m_pSubThread = NULL;
			}
			return false;
		}
	}

	return true;
}

unsigned __stdcall ZZCServer::MainThreadEventDispatch(void* param)
{
	ZZCServer* pThis = (ZZCServer*)param;

	event_base_dispatch(pThis->m_pBase);

	_endthreadex(0);
	return 1;
}

int n = 0;
void ZZCServer::Listen_cb(struct evconnlistener *listener, 
	           evutil_socket_t fd,
               struct sockaddr *sa, 
			   int len, 
			   void *param)
{
	ZZCServer* pThis = (ZZCServer*)param;

	int nCurrentThread = pThis->m_nCurrentThread % pThis->m_sWorkThreadNum;

	if (++pThis->m_nCurrentThread >= pThis->m_sWorkThreadNum)
	{
		pThis->m_nCurrentThread = 0;
	}

	int sendfd = pThis->m_pSubThread[nCurrentThread].write_fd;
	send(sendfd, (char*)&fd, sizeof(evutil_socket_t), 0);
	
	printf("a new client %d ,thread = %d\n",++n,GetCurrentThreadId());
}

bool ZZCServer::RunMainThreadListen()
{
	if((m_pBase = event_base_new()) == NULL)
	{
		return false;
	}

	struct sockaddr_in sSerAddr;
	memset(&sSerAddr, 0, sizeof(sSerAddr));
	sSerAddr.sin_family = AF_INET;
	sSerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sSerAddr.sin_port = htons(m_sPort);

	m_pListener = evconnlistener_new_bind(m_pBase, 
										  Listen_cb, 
										  (void*)this, 
										  LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, 
										  -1, 
										  (struct sockaddr*)&sSerAddr, 
										  sizeof(sSerAddr));

	if (m_pListener == NULL)
	{
		return false;
	}

	m_hThread = (HANDLE)_beginthreadex( NULL, 0, &MainThreadEventDispatch, (void*)this, 0, NULL);

	if (m_hThread == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	return true;
}

bool ZZCServer::Run(short port, 
	                short workthreadnum, 
					unsigned int itemnum, 
					int readtimeout, 
					int writetimeout)
{
	m_sPort = port;
	m_sWorkThreadNum = workthreadnum;
	m_uItemNum = itemnum;
	m_nReadTimeout = readtimeout;
	m_nWriteTimeout = writetimeout;

	if(!RunMainThreadListen())
		return false;

	if (!RunSubThreadCb())
		return false;

	return true;
}

bool ZZCServer::Stop()
{
	struct timeval delay = { 1, 0 };
	event_base_loopexit(m_pBase, &delay);
	WaitForSingleObject(m_hThread,INFINITE);

	for (int i = 0;i < m_sWorkThreadNum;i++)
	{
		event_base_loopexit(m_pSubThread[i].pbase, &delay);
		WaitForSingleObject(m_pSubThread[i].hThread,INFINITE);
	}

	for (int i = 0;i < m_sWorkThreadNum;i++)
	{
		list<Client_Item*>::iterator it;
		for (it = m_pSubThread[i].itemlist.begin();it != m_pSubThread[i].itemlist.end();it++)
		{
			if ((*it)->bev)
			{
				bufferevent_free((*it)->bev);
			}

			if ((*it))
			{
				delete (*it);
				(*it) = NULL;
			}
		}

		m_pSubThread[i].itemlist.clear();

		event_base_free(m_pSubThread[i].pbase);
	}

	if (m_pSubThread)
	{
		delete[] m_pSubThread;
		m_pSubThread = NULL;
	}

	evconnlistener_free(m_pListener);
	event_base_free(m_pBase);

	return true;
}