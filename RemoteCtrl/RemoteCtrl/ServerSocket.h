#pragma once
#include "pch.h"
#include "framework.h"

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
	bool InitSocket() {
		if (m_servSock == -1) return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(9527);
		// 绑定
		if (bind(m_servSock, (struct sockaddr*)&serv_addr, sizeof(m_servSock)) == -1) return false;
		if (listen(m_servSock, 1) == -1) return false;
		return true;
	}

	bool AcceptClient() {
		sockaddr_in clnt_addr;
		int clnt_len = sizeof(clnt_addr);
		m_clntSock = accept(m_servSock, (struct sockaddr*)&clnt_addr, &clnt_len);
		if (m_clntSock == -1)return false;
		return true;
	}

	int DealCommand() {
		if (m_clntSock == -1) return -1;
		char buffer[1024] = "";
		while (true) {
			int len = recv(m_clntSock, buffer, sizeof(buffer), 0);
			if (len <= 0) {
				return -1;
			}
			// TODO: 处理命令

		}
	}

	bool SendMsg(const char* msg, int nSize) {
		if (m_clntSock == -1) return false;
		return send(m_clntSock, msg, nSize, 0) > 0;
	}

private:
	SOCKET m_servSock;
	SOCKET m_clntSock;
	CServerSocket() {
		m_servSock = INVALID_SOCKET; // -1
		m_clntSock = INVALID_SOCKET;
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境"), _T("初始化错误"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_servSock = socket(PF_INET, SOCK_STREAM, 0);
	}
	CServerSocket(const CServerSocket&) {}              // 禁止拷贝构造
	CServerSocket& operator=(const CServerSocket&) {}   // 禁止拷贝赋值
	~CServerSocket() {
		closesocket(m_servSock);
		WSACleanup();
 	} 
	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		}
		return TRUE;
	}
private:
	

	static void releseInstance() {
		if (m_instance != NULL) {
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
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
