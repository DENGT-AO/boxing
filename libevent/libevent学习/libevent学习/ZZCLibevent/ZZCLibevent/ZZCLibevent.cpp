// ZZCLibevent.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "ZZCServer.h"


int _tmain(int argc, _TCHAR* argv[])
{
	ZZCServer ms;
	ms.Run(8888,3,5,30,30);
	getchar();
	ms.Stop();

	return 0;
}

