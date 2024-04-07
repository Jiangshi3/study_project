#pragma once
#include "pch.h"
#include "framework.h"
#include <list>
#include "Packet.h"

typedef void(*SOCKET_CALLBACK)(void*, int, std::list<CPacket>&, CPacket&);
// typedef void(*SOCKET_CALLBACK)(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket);

class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		// 静态成员函数没有this指针，只能方位静态成员变量；m_instance为静态成员变量
		if (m_instance == NULL)  // 单例；此时是线程安全的，不需要mutex；因为是在main之前执行的。
		{
			m_instance = new CServerSocket(); 
		}
		return m_instance;
	}

	int Run(SOCKET_CALLBACK callback, void* arg, short port = 9527)
	{
		// TODO: socket, bind, listen, accept, read, write, close
		bool ret = InitSocket(port);
		if (ret == false) return -1;
		std::list<CPacket> lstPackets;
		m_callback = callback;
		m_arg = arg;
		int count = 0;
		while (true) {
			if (AcceptClient() == false) {
				if(count>=3){
					return -2;
				}
				count++;
			}
			int ret = DealCommand();
			if (ret > 0) {
				m_callback(m_arg, ret, lstPackets, m_packet);
				while (lstPackets.size() > 0) {
					Send(lstPackets.front());
					lstPackets.pop_front();
				}
			}
			CloseClient();
		}
	}

protected:
	bool InitSocket(short port) {
		if (m_servSock == -1) return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(port);
		// 绑定
		if (bind(m_servSock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) return false;
		if (listen(m_servSock, 1) == -1) return false;
		return true;
	}

	bool AcceptClient() {
		TRACE("enter AcceptClient()\r\n");
		sockaddr_in clnt_addr;
		int clnt_len = sizeof(clnt_addr);
		m_clntSock = accept(m_servSock, (struct sockaddr*)&clnt_addr, &clnt_len);
		TRACE("accept m_clntSock=%d\r\n", m_clntSock);
		if (m_clntSock == -1)return false;
		return true;
	}

#define BUFFER_SIZE 4096
	int DealCommand() {
		if (m_clntSock == -1) return -1;
		char* buffer = new char[BUFFER_SIZE];  
		if (buffer == NULL) {
			TRACE("内存不足\r\n");
		}
		memset(buffer, 0, BUFFER_SIZE);  // TODO
		size_t index = 0;
		while (true) {
			size_t len = recv(m_clntSock, buffer+index, BUFFER_SIZE-index, 0);  // 缓冲区的处理！！！！！！
			if (len <= 0) {
				delete[] buffer;
				return -1;
			}
			index += len;  // !!!
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);  // 这里第二个参数len传入的引用；会改变其值;(所以后面的len的值不一定等于之前的值)
			if (len > 0)   // 这里的len表示的是：用到的一整块数据包的长度
			{
				index -= len; 
				memmove(buffer, buffer + len, index);   // 缓冲区的处理！！！！！！
				delete[] buffer;
				return m_packet.sCmd;  // 并返回一个操作指令
			}
			// 如果len==0 表示缓冲区还没有一整块数据包，就让继续while循环
		}
		delete[] buffer;
		return -1;
	}


	bool Send(const char* msg, int nSize) {
		if (m_clntSock == -1) return false;
		return send(m_clntSock, msg, nSize, 0) > 0;
	}
	bool Send(CPacket& packet) // 这里不能使用const了，因为packet.Data()会改变成员值
	{
		if (m_clntSock == -1) return false;
		// Dump((BYTE*)packet.Data(), packet.Size());
		return send(m_clntSock, packet.Data(), packet.Size(), 0) > 0; 
	}


	void CloseClient() {
		if (m_clntSock != INVALID_SOCKET) {
			closesocket(m_clntSock);
			m_clntSock = INVALID_SOCKET;
		}
	}

private:
	SOCKET m_servSock;
	SOCKET m_clntSock;
	CPacket m_packet;
	SOCKET_CALLBACK m_callback;
	void* m_arg;
	CServerSocket() {
		m_servSock = INVALID_SOCKET; // -1
		m_clntSock = INVALID_SOCKET;
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境"), _T("初始化错误"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_servSock = socket(PF_INET, SOCK_STREAM, 0);
	}
	CServerSocket(const CServerSocket& ss) {}              // 禁止拷贝构造
	CServerSocket& operator=(const CServerSocket& ss) {}   // 禁止拷贝赋值
	~CServerSocket() {
		closesocket(m_servSock);
		WSACleanup();
 	} 
	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(2, 0), &data) != 0) { // 现在使用高一点的版本
			return FALSE;
		}
		return TRUE;
	}
private:
	static void releseInstance() {
		if (m_instance != NULL) {  // 防御性编程
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp; // 这里所消耗的时间比较多； 先把m_instance置空；
		}
	}

	class CHelper{
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releseInstance();
		}
	};

	static CServerSocket* m_instance;
	static CHelper m_helper;
};

// extern CServerSocket server;
// extern CServerSocket* pServer;
