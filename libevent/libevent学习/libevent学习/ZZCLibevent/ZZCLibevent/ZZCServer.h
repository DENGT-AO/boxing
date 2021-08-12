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
	evutil_socket_t fd;        //文件描述符
	char *in_buf;
	FILE *f;
	char szImgName[260];    //图片的名称
	int nFileSize;          //文件总大小
	int nReceiveTotal;      //接收文件大小

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
	struct event event; //事件
	evutil_socket_t read_fd;  //读管道描述符
	evutil_socket_t write_fd; //写管道描述符
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
	short m_sPort;                       //监听端口
	short m_sWorkThreadNum;              //子线程数目
	unsigned int m_uItemNum;             //一个子线程中的客户端数目
	int m_nReadTimeout;                  //读取超时时间
	int m_nWriteTimeout;                 //写入超时时间
	struct evconnlistener *m_pListener;  //主线程监听器
	struct event_base *m_pBase;          //主线程base
	HANDLE m_hThread;                    //主线程句柄
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

