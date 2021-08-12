#pragma once
class BufferManager
{
public:
	BufferManager(void);
	~BufferManager(void);

	FILE *f;
	char szImgName[260]; //图片的名称
	char buf[1000000];  //用于接收图像数据
	int nFileSize;      //文件总大小
	int nReceiveTotal;  //接收文件大小

public:
	void iniParam();

};

