#include "StdAfx.h"
#include "BufferManager.h"
#include "Windows.h"

BufferManager::BufferManager(void):f(NULL)
{
	iniParam();
}


BufferManager::~BufferManager(void)
{
}

void BufferManager::iniParam()
{
	memset(szImgName,0,sizeof(szImgName));
	memset(buf,0,sizeof(buf));
	nFileSize = 0;
	nReceiveTotal = 0;

	if(f)
	{
		fclose(f);
		f = NULL;
	}
}