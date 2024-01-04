#pragma once
#include <WinSock2.h>
#include <share.h>
#include <memory>

#include "base.h"
#pragma comment(lib,"ws2_32.lib")  //ÍøÂçAPIº¯Êý

class Socket
{
public:
	Socket(bool bIsTcp = true) {
		m_sock = INVALID_SOCKET;
		if (bIsTcp) {
			m_sock = socket(PF_INET, SOCK_STREAM, 0);
		}
		else { // UDP
			m_sock = socket(PF_INET, SOCK_DGRAM, 0);
		}
	}
	Socket(SOCKET s) {
		m_sock = s;
	}
	void Close() {
		if (m_sock != INVALID_SOCKET) {
			SOCKET tmp = m_sock;
			m_sock = INVALID_SOCKET;
			closesocket(tmp);
		}
	}
	operator SOCKET() {
		return m_sock;
	}

	~Socket() {
		Close();
	}

private:
	SOCKET m_sock;
};

class EAddress
{
public:
	EAddress() {
		m_port = -1;
		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.sin_family = AF_INET;
	}
	EAddress(const std::string& ip, short port) {
		m_ip = ip;
		m_port = port;
		m_addr.sin_port = htons(port);
		m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
	}
	EAddress(const EAddress& addr) {
		m_ip = addr.m_ip;
		m_port = addr.m_port;
		memcpy(&m_addr, &addr.m_addr, sizeof(sockaddr_in));
	}
	EAddress& operator=(const EAddress& addr) {
		if (this != &addr) {
			m_ip = addr.m_ip;
			m_port = addr.m_port;
			memcpy(&m_addr, &addr.m_addr, sizeof(sockaddr_in));
		}
		return *this;
	}
	EAddress& operator=(short port) {
		m_port = (unsigned short)port;
		m_addr.sin_port = htons(port);
		return *this;
	}

	~EAddress(){}
	void  Update(const std::string& ip, short port) {
		m_ip = ip;
		m_port = port;
		m_addr.sin_port = htons(port);
		m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
	}
	operator const sockaddr* () const {
		return (sockaddr*)&m_addr;
	}
	operator sockaddr* () {
		return (sockaddr*)&m_addr;
	}
	operator sockaddr_in* () {
		return &m_addr;
	}
	int size() const {
		return sizeof(sockaddr_in);
	}
	const std::string Ip() const {
		return m_ip;
	}
	unsigned short Port() const {
		return m_port;
	}
	void Fresh() {
		m_ip = inet_ntoa(m_addr.sin_addr);
	}

private:
	std::string m_ip;
	unsigned short m_port;
	sockaddr_in m_addr;
};

class ESocket
{
public:
	ESocket(bool isTcp = true):
		m_socket(new Socket(isTcp)), 
		m_isTCP(isTcp)
	{}
	ESocket(const ESocket& sock):
		m_socket(sock.m_socket), 
		m_isTCP(sock.m_isTCP)
	{}
	ESocket(SOCKET sock,bool isTcp):
		m_socket(new Socket(sock)),
		m_isTCP(isTcp)
	{}
	ESocket& operator=(const ESocket& sock) {
		if(this!=&sock) m_socket = sock.m_socket;
		return *this;
	}
	~ESocket() {
		m_socket.reset();
	}
	operator SOCKET() const{
		return *m_socket;
	}
	int Bind(const EAddress& addr) {
		if (m_socket == nullptr) {
			m_socket.reset(new Socket(m_isTCP));
		}
		return bind(*m_socket, addr, addr.size());
	}
	int Listen(int backlog = 5) {
		return listen(*m_socket, backlog);
	}
	ESocket Accept(EAddress& addr) {
		int len = addr.size();
		if (m_socket == nullptr) return ESocket(INVALID_SOCKET,true);
		SOCKET server = *m_socket;
		if (server == INVALID_SOCKET) return ESocket(INVALID_SOCKET,true);
		SOCKET s = accept(server, addr, &len);
		return ESocket(s, m_isTCP);
	}
	int Connect(const EAddress& addr) {
		return connect(*m_socket, addr, addr.size());
	}
	int Recv(EBuffer& buffer) {
		return recv(*m_socket, buffer, buffer.size(),0);
	}
	int Send(const EBuffer& buffer) {
		printf("send(%d):[%s]\r\n", buffer.size(), (char*)buffer);
		int index = 0;
		char* pData = buffer;
		while (index < (int)buffer.size()) {
			int ret = send(*m_socket, pData + index, buffer.size() - index, 0);
			if (ret < 0) return ret;
			if (ret == 0) break;
			index += ret;
		}
		return index;
	}

	void Close() {
		m_socket.reset();
	}

private:
	std::shared_ptr<Socket> m_socket;
	bool m_isTCP;
};

class SocketIniter
{
public:
	SocketIniter() {
		WSADATA wsa;
		WSAStartup(MAKEWORD(2, 2), &wsa);
	}
	~SocketIniter() {
		WSACleanup();
	}
};