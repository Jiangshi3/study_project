#pragma once
#include <map>
// #include <Windows.h>
#include <mswsock.h>
#include "CThread.h"
#include "CQueue.h"

enum MyOperator{
    ENone,
    EAccept,
    ERecv,
    ESend,
    EError
};

class CServer;
class MyClient;

typedef std::shared_ptr<MyClient> PCLIENT;

template <MyOperator> class AcceptOverlapped; // Ҫ����
template <MyOperator> class RecvOverlapped;
template <MyOperator> class SendOverlapped;
template <MyOperator> class ErrorOverlapped;

typedef AcceptOverlapped<EAccept> AcceptOVERLAPPED;// �����ݴ���� MyOperator ö��ֵ�Ĳ�ͬ��ʵ��������ͬ����
typedef RecvOverlapped<ERecv> RecvOVERLAPPED; 
typedef SendOverlapped<ESend> SendOVERLAPPED;
typedef ErrorOverlapped<EError> ErrorOVERLAPPED;

class MyOverlapped {
public:
	OVERLAPPED m_overlapped;  // �ڵ�һ��
	DWORD m_operator;         // ����  enum MyOperator{};
    std::vector<char> m_buffer;  // ������
    CThreadWorker m_worker;  // ������
    CServer* m_server;       // ����������
	MyClient* m_client;        // ��Ӧ�Ŀͻ��ˣ� ����û����ʹ������ָ��PCLIENT m_client;
	WSABUF m_wsabuffer;  // ���Ǹ����ݽṹ������һ��buffer���Ⱥ�ָ�룻
	virtual ~MyOverlapped() {
		m_buffer.clear();
	}
};

class MyClient: public CThreadFuncBase {
public:
	MyClient();
	~MyClient() {
		m_buffer.clear();
		closesocket(m_sock);
		m_overlapped.reset();
		m_recv.reset();
		m_send.reset();
		m_vecSend.Clear();
	}
	void SetOverlapped(PCLIENT& ptr);
	operator SOCKET() {  // ���ֲ��������أ�Ҳ����ֱ��д����Ա����
		return m_sock;
	}
	operator PVOID() {
		return &m_buffer[0];
	}
	operator LPDWORD() {
		return &m_received;
	}
	operator LPOVERLAPPED();
	LPWSABUF RecvWSABuffer();

	LPWSABUF SendWSABuffer();

	DWORD& flags() {
		return m_flags;
	}

	sockaddr_in* GetLocalAddr() {
		return &m_laddr;
	}
	sockaddr_in* GetRemoteAddr() {
		return &m_raddr;
	}
	size_t GetBufferSize() const{
		return m_buffer.size();
	}
	// Recv()��Send()Ҫ�õ����ǿͻ��˵��׽���m_sock; ���Է���class MyClient�У�
	// CServer���е��׽���������accept��
	int Recv();
	int Send(void* buffer, size_t nSize);  // Ҳ������char*
	int SendData(std::vector<char>& data);
private:
	SOCKET m_sock;
	DWORD m_received;
	DWORD m_flags;
	std::shared_ptr<AcceptOVERLAPPED> m_overlapped;
	std::shared_ptr<RecvOVERLAPPED> m_recv;
	std::shared_ptr<SendOVERLAPPED> m_send;
	std::vector<char> m_buffer;
	size_t m_used;  // �Ѿ�ʹ�õĻ�������С
	sockaddr_in m_laddr; // local addr  ���ص�ַ
	sockaddr_in m_raddr; // remote addr Զ�̵�ַ
	bool m_isbusy;
	CSendQueue<std::vector<char>> m_vecSend;  // �������ݶ��У�    ������������Ǹ�vector<char>����һ��Ҫ���͵�buffer
};

// �����ݴ���� MyOperator ö��ֵ�Ĳ�ͬ��ʵ��������ͬ����
template <MyOperator>
class AcceptOverlapped :public MyOverlapped, CThreadFuncBase {
public:
	AcceptOverlapped();
	int AcceptWorker();
};


template <MyOperator>
class RecvOverlapped :public MyOverlapped, CThreadFuncBase {
public:
	RecvOverlapped();
	int RecvWorker() {
		int ret = m_client->Recv();
		return ret;
	}
};

template <MyOperator>
class SendOverlapped :public MyOverlapped, CThreadFuncBase {
public:
	SendOverlapped();
	/*
	*1 Send���ܲ����������
	*/
	int SendWorker() {
		return -1;
	}
};

template <MyOperator>
class ErrorOverlapped :public MyOverlapped, CThreadFuncBase {
public:
    ErrorOverlapped() :m_operator(EError), m_worker(this, &ErrorOverlapped::ErrorWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}

	int ErrorWorker() {
		//TODO
		return -1;
	}
};


class CServer :
    public CThreadFuncBase
{
public:
    CServer(const std::string& ip = "0.0.0.0", const short port = 9527) :m_pool(10){
        m_hIOCP = INVALID_HANDLE_VALUE;
        m_sock = INVALID_SOCKET;      
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(port);
		m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    }
	~CServer();

	bool StartService();
	bool NewAccept();

private:
	void CreateSocket() {
		m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		int opt = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));  // ���õ�ַ����
	}

	int threadIocp();

private:
    CThreadPool m_pool;
    HANDLE m_hIOCP;
    SOCKET m_sock;
    sockaddr_in m_addr;
    std::map<SOCKET, PCLIENT> m_client; //  typedef std::shared_ptr<MyClient> PCLIENT;
};

