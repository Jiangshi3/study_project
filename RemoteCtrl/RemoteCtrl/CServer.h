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

template <MyOperator> class AcceptOverlapped; // 要声明
template <MyOperator> class RecvOverlapped;
template <MyOperator> class SendOverlapped;
template <MyOperator> class ErrorOverlapped;

typedef AcceptOverlapped<EAccept> AcceptOVERLAPPED;// 将根据传入的 MyOperator 枚举值的不同而实例化出不同的类
typedef RecvOverlapped<ERecv> RecvOVERLAPPED; 
typedef SendOverlapped<ESend> SendOVERLAPPED;
typedef ErrorOverlapped<EError> ErrorOVERLAPPED;

class MyOverlapped {
public:
	OVERLAPPED m_overlapped;  // 在第一个
	DWORD m_operator;         // 操作  enum MyOperator{};
    std::vector<char> m_buffer;  // 缓冲区
    CThreadWorker m_worker;  // 处理函数
    CServer* m_server;       // 服务器对象
	MyClient* m_client;        // 对应的客户端； 这里没有再使用智能指针PCLIENT m_client;
	WSABUF m_wsabuffer;  // 这是个数据结构，包含一个buffer长度和指针；
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
	operator SOCKET() {  // 这种操作符重载，也可以直接写个成员函数
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
	// Recv()和Send()要用到的是客户端的套接字m_sock; 所以放在class MyClient中；
	// CServer类中的套接字是用来accept的
	int Recv();
	int Send(void* buffer, size_t nSize);  // 也可以用char*
	int SendData(std::vector<char>& data);
private:
	SOCKET m_sock;
	DWORD m_received;
	DWORD m_flags;
	std::shared_ptr<AcceptOVERLAPPED> m_overlapped;
	std::shared_ptr<RecvOVERLAPPED> m_recv;
	std::shared_ptr<SendOVERLAPPED> m_send;
	std::vector<char> m_buffer;
	size_t m_used;  // 已经使用的缓冲区大小
	sockaddr_in m_laddr; // local addr  本地地址
	sockaddr_in m_raddr; // remote addr 远程地址
	bool m_isbusy;
	CSendQueue<std::vector<char>> m_vecSend;  // 发送数据队列；    这个队列里面是个vector<char>，是一个要发送的buffer
};

// 将根据传入的 MyOperator 枚举值的不同而实例化出不同的类
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
	*1 Send可能不会立即完成
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
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));  // 设置地址重用
	}

	int threadIocp();

private:
    CThreadPool m_pool;
    HANDLE m_hIOCP;
    SOCKET m_sock;
    sockaddr_in m_addr;
    std::map<SOCKET, PCLIENT> m_client; //  typedef std::shared_ptr<MyClient> PCLIENT;
};

