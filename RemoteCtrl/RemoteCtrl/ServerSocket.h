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
		// ��̬��Ա����û��thisָ�룬ֻ�ܷ�λ��̬��Ա������m_instanceΪ��̬��Ա����
		if (m_instance == NULL)  // ��������ʱ���̰߳�ȫ�ģ�����Ҫmutex����Ϊ����main֮ǰִ�еġ�
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
		// ��
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
			TRACE("�ڴ治��\r\n");
		}
		memset(buffer, 0, BUFFER_SIZE);  // TODO
		size_t index = 0;
		while (true) {
			size_t len = recv(m_clntSock, buffer+index, BUFFER_SIZE-index, 0);  // �������Ĵ�������������
			if (len <= 0) {
				delete[] buffer;
				return -1;
			}
			index += len;  // !!!
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);  // ����ڶ�������len��������ã���ı���ֵ;(���Ժ����len��ֵ��һ������֮ǰ��ֵ)
			if (len > 0)   // �����len��ʾ���ǣ��õ���һ�������ݰ��ĳ���
			{
				index -= len; 
				memmove(buffer, buffer + len, index);   // �������Ĵ�������������
				delete[] buffer;
				return m_packet.sCmd;  // ������һ������ָ��
			}
			// ���len==0 ��ʾ��������û��һ�������ݰ������ü���whileѭ��
		}
		delete[] buffer;
		return -1;
	}


	bool Send(const char* msg, int nSize) {
		if (m_clntSock == -1) return false;
		return send(m_clntSock, msg, nSize, 0) > 0;
	}
	bool Send(CPacket& packet) // ���ﲻ��ʹ��const�ˣ���Ϊpacket.Data()��ı��Աֵ
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
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_servSock = socket(PF_INET, SOCK_STREAM, 0);
	}
	CServerSocket(const CServerSocket& ss) {}              // ��ֹ��������
	CServerSocket& operator=(const CServerSocket& ss) {}   // ��ֹ������ֵ
	~CServerSocket() {
		closesocket(m_servSock);
		WSACleanup();
 	} 
	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(2, 0), &data) != 0) { // ����ʹ�ø�һ��İ汾
			return FALSE;
		}
		return TRUE;
	}
private:
	static void releseInstance() {
		if (m_instance != NULL) {  // �����Ա��
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp; // ���������ĵ�ʱ��Ƚ϶ࣻ �Ȱ�m_instance�ÿգ�
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
