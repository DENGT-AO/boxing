#pragma once
#include "winsock2.h"
#include "event2/listener.h"
#include "event2/thread.h"
#include "event2/bufferevent.h"
#include "event.h"
#include "event2/thread.h"
#include <process.h>
#include <list>
using std::list;

class ZZCServer;
typedef struct _SUB_THREAD Sub_Thread;

#define BufferLen 4906

typedef struct _CLIENT_ITEM
{
	Sub_Thread* pSubThread;
	struct bufferevent* bev;
	evutil_socket_t fd;        //�ļ�������
	char *in_buf;
	FILE *f;
	char szImgName[260];    //ͼƬ������
	int nFileSize;          //�ļ��ܴ�С
	int nReceiveTotal;      //�����ļ���С

	_CLIENT_ITEM()
	{
		bev = NULL;
		fd = NULL;
		nFileSize = 0;
		nReceiveTotal = 0;
		in_buf = NULL;
		in_buf = new char[BufferLen];
		memset(szImgName,0,sizeof(szImgName));
	}

	~_CLIENT_ITEM()
	{
		if (in_buf)
		{
			delete[] in_buf;
			in_buf = NULL;
		}
	}

	inline void clearparam()
	{
		if (f)
		{
			fclose(f);
			f = NULL;
		}

		nFileSize = 0;
		nReceiveTotal = 0;
		fd = NULL;
		memset(szImgName,0,sizeof(szImgName));
	}

}Client_Item;

struct _SUB_THREAD
{
	struct event_base *pbase;
	struct event event; //�¼�
	evutil_socket_t read_fd;  //���ܵ�������
	evutil_socket_t write_fd; //д�ܵ�������
	HANDLE hThread;
	list<Client_Item*> itemlist;
	ZZCServer* pServer;

	inline bool PopItem(evutil_socket_t est)
	{
		list<Client_Item*>::iterator it;
		for (it = itemlist.begin();it != itemlist.end();it++)
		{
			if ((*it)->fd == est)
			{
				itemlist.erase(it);
				return true;
			}
		}
		return false;
	}

	_SUB_THREAD()
	{
		hThread = INVALID_HANDLE_VALUE;
		pbase = NULL;
	}

};

class ZZCServer
{
public:
	ZZCServer(void);
	~ZZCServer(void);

private:
	short m_sPort;                       //�����˿�
	short m_sWorkThreadNum;              //���߳���Ŀ
	unsigned int m_uItemNum;             //һ�����߳��еĿͻ�����Ŀ
	int m_nReadTimeout;                  //��ȡ��ʱʱ��
	int m_nWriteTimeout;                 //д�볬ʱʱ��
	struct evconnlistener *m_pListener;  //���̼߳�����
	struct event_base *m_pBase;          //���߳�base
	HANDLE m_hThread;                    //���߳̾��
	Sub_Thread* m_pSubThread;
	int m_nCurrentThread;

public:
	bool Run(short port = 8888, short workthreadnum = 10, unsigned int itemnum = 10, int readtimeout = 60, int writetimeout = 60);
	bool Stop();
	bool RunMainThreadListen();
	bool RunSubThreadCb();

private:
	static unsigned __stdcall MainThreadEventDispatch(void* param);
	static unsigned __stdcall SubThreadEventDispatch(void* param);
	static void pipe_process(int fd, short which, void *arg);
	static void Listen_cb(struct evconnlistener *listener, evutil_socket_t fd,struct sockaddr *sa, int len, void *param);
	static void Read_cb(struct bufferevent *bev, void *param);
	static void Error_cb(struct bufferevent *bev, short error, void *param);
};

