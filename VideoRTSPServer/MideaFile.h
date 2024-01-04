#pragma once
#include "base.h"
class MideaFile
{
public:
	MideaFile();
	~MideaFile();
	int Open(const EBuffer& path, int nType = 96);
	//如果buffer的size为0，则表示没有帧了
	EBuffer ReadOneFrame();
	void Close();
	//重置后，ReadOneFrame又会有值返回
	void Reset();
private:
	//返回-1表明查找失败
	long FindH264Head(int& headsize);
	EBuffer ReadH264Frame();
private:
	long m_size;
	FILE* m_file;
	int m_type; //96 表示H264
};

