#pragma once
#include "base.h"
#include "Socket.h"
class RTPHeader {  //见RTP header定义图
public:
	unsigned short csrccount : 4;  //位域 ，以位为单位来指定其成员所占内存长度
	unsigned short extension : 1;
	unsigned short padding : 1;
	unsigned short version : 2;
	
	unsigned short pytype : 7;
	unsigned short mark : 1;

	unsigned short serial;
	unsigned timestamp;
	unsigned ssrc;
	unsigned csrc[15];
	
	
public:
	RTPHeader();
	RTPHeader(const RTPHeader& header);
	RTPHeader& operator=(const RTPHeader& header);
	operator EBuffer();  // 允许强制转换
};

class RTPFrame
{
public:
	RTPHeader m_head;  //RTP头
	EBuffer m_pyload;  //RTP荷载
	operator EBuffer();
};

class RTPHelper
{
public:
	RTPHelper():timestamp(0), m_udp(false){
		m_udp.Bind(EAddress("0.0.0.0", (short)55000));
		//m_file = fopen("./out.bin", "wb+");
	}
	~RTPHelper(){
	//	fclose(m_file);
	}

	int SendMediaFrame(RTPFrame& rtpframe, EBuffer& frame, const EAddress& client);
private:
	RTPHeader m_head;
	EBuffer m_pyload;
	
private:
	int GetFrameSepSize(EBuffer& frame);
	int SendFrame(const EBuffer& frame, const EAddress& client);
	DWORD timestamp;
	ESocket m_udp;
	//FILE* m_file;
};

