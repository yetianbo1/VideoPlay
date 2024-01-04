#pragma once
#include "base.h"
class MideaFile
{
public:
	MideaFile();
	~MideaFile();
	int Open(const EBuffer& path, int nType = 96);
	//���buffer��sizeΪ0�����ʾû��֡��
	EBuffer ReadOneFrame();
	void Close();
	//���ú�ReadOneFrame�ֻ���ֵ����
	void Reset();
private:
	//����-1��������ʧ��
	long FindH264Head(int& headsize);
	EBuffer ReadH264Frame();
private:
	long m_size;
	FILE* m_file;
	int m_type; //96 ��ʾH264
};

