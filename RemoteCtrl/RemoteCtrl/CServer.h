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
template <MyOperator> class AcceptOverlapped;
typedef AcceptOverlapped<EAccept> AcceptOVERLAPPED;

class MyOverlapped {
public:
	OVERLAPPED m_overlapped;  // 在第一个
    DWORD m_operator;         // 操作  enum MyOperator{};
    std::vector<char> m_buffer;  // 缓冲区
    CThreadWorker m_worker;  // 处理函数
    CServer* m_server;       // 服务器对象
};

class MyClient {
public:
	MyClient();
	~MyClient() {
		closesocket(m_sock);
	}
	void SetOverlapped(PCLIENT& ptr);
	operator SOCKET() {
		return m_sock;
	}
	operator PVOID() {
		return &m_buffer[0];
	}
	operator LPDWORD() {
		return &m_received;
	}
	operator LPOVERLAPPED();
	sockaddr_in* GetLocalAddr() {
		return &m_laddr;
	}
	sockaddr_in* GetRemoteAddr() {
		return &m_raddr;
	}
private:
	SOCKET m_sock;
	DWORD m_received;
	std::shared_ptr<AcceptOVERLAPPED> m_overlapped;
	std::vector<char> m_buffer;
	sockaddr_in m_laddr; // local addr  本地地址
	sockaddr_in m_raddr; // remote addr 远程地址
	bool m_isbusy;
};

// 将根据传入的 MyOperator 枚举值的不同而实例化出不同的类
template <MyOperator>
class AcceptOverlapped :public MyOverlapped, CThreadFuncBase {
public:
	AcceptOverlapped();
	int AcceptWorker();
    PCLIENT m_client;
};


template <MyOperator>
class RecvOverlapped :public MyOverlapped, CThreadFuncBase {
public:
    RecvOverlapped() :m_operator(ERecv), m_worker(this, &RecvOverlapped::RecvWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024*256);
	}

	int RecvWorker() {
		//TODO
	}
};

template <MyOperator>
class SendOverlapped :public MyOverlapped, CThreadFuncBase {
public:
    SendOverlapped() :m_operator(ESend), m_worker(this, &SendOverlapped::SendWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024*256);
	}

	int SendWorker() {
		//TODO
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
	}
};

// typedef AcceptOverlapped<EAccept> AcceptOVERLAPPED;  // 将根据传入的 MyOperator 枚举值的不同而实例化出不同的类
typedef AcceptOverlapped<ERecv> RecvOVERLAPPED;  // 将根据传入的 MyOperator 枚举值的不同而实例化出不同的类
typedef AcceptOverlapped<ESend> SendOVERLAPPED;
typedef AcceptOverlapped<EError> ErrorOVERLAPPED;


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
    ~CServer(){}

    bool StartService() {
		CreateSocket();		
		if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return false;
		}
		if (listen(m_sock, 3) == -1) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return false;
		}
		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
		if (m_hIOCP == NULL) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
		CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);
		m_pool.Invoke();
		m_pool.DispatchWorker(CThreadWorker(this, (FUNCTYPE)&CServer::threadIocp));  // 这里封装后，把这个在线程池中分发给线程后，在线程中具体执行的函数就是CServer::threadIocp();
		// m_pool.DispatchWorker(CThreadWorker(this, (FUNCTYPE)&CServer::threadIocp));
		// m_pool.DispatchWorker(CThreadWorker(this, (FUNCTYPE)&CServer::threadIocp));
		
		if (!NewAccept()) {
			return false;
		}
		return true;
    }

	bool NewAccept() {
		PCLIENT pClient(new MyClient());
		pClient->SetOverlapped(pClient);
		m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient)); // 此时第一个参数类型就是SOCKET，通过操作符重载*pClient返回的就是类的成员变量SOCKET sock；
		if (!AcceptEx(m_sock, *pClient, *pClient, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, *pClient, *pClient)) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
		return true;
	}

private:
	void CreateSocket() {
		m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		int opt = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));  // 设置地址重用
	}

    int threadIocp() {
        DWORD Transferred = 0;
        ULONG_PTR CompletionKey = 0;
        OVERLAPPED* lpOverlapped = NULL;
        if (GetQueuedCompletionStatus(m_hIOCP, &Transferred, &CompletionKey, &lpOverlapped, INFINITE)) {
            if (Transferred > 0 && (CompletionKey != NULL)) {
                MyOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, MyOverlapped, m_overlapped);
                switch (pOverlapped->m_operator) {
                case EAccept:{
					// AcceptOverlapped<EAccept>* pAccept = (AcceptOverlapped<EAccept>*)pOverlapped;
					AcceptOVERLAPPED* pAccept = (AcceptOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pAccept->m_worker);
					break;
				}
				case ERecv: {
					RecvOVERLAPPED* pRecv = (RecvOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pRecv->m_worker);
					break;
				}					
				case ESend: {
					SendOVERLAPPED* pSend = (SendOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pSend->m_worker);
					break;
				}
				case EError: {
					ErrorOVERLAPPED* pError = (ErrorOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pError->m_worker);
					break;
				}
                }
            }
            else {
                return -1;
            }
        }
        return 0;
    }

private:
    CThreadPool m_pool;
    HANDLE m_hIOCP;
    SOCKET m_sock;
    sockaddr_in m_addr;
    std::map<SOCKET, PCLIENT> m_client; //  typedef std::shared_ptr<MyClient> PCLIENT;
};

