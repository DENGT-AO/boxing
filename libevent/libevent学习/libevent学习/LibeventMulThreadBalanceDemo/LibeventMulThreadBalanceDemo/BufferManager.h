#pragma once
class BufferManager
{
public:
	BufferManager(void);
	~BufferManager(void);

	FILE *f;
	char szImgName[260]; //ͼƬ������
	char buf[1000000];  //���ڽ���ͼ������
	int nFileSize;      //�ļ��ܴ�С
	int nReceiveTotal;  //�����ļ���С

public:
	void iniParam();

};

