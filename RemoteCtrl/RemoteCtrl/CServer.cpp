#include "pch.h"
#include "CServer.h"
#include "Tool.h"
#pragma warning(disable:4407)

template <MyOperator op>
AcceptOverlapped<op>::AcceptOverlapped()
{
	m_operator = EAccept;
	// m_worker(this, &AcceptOverlapped::AcceptWorker); // 这种不行
	// m_worker = CThreadWorker(this, &AcceptOverlapped::AcceptWorker); // 不行
	m_worker = CThreadWorker(this, (FUNCTYPE) & AcceptOverlapped<op>::AcceptWorker);  // 这里会报警告，代码C4407；
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024);
	m_server = NULL;
}

template <MyOperator op>
int AcceptOverlapped<op>::AcceptWorker() {  // 连接到一个客户端就accept一次
	INT lLength = 0, rLength = 0;
	// AcceptEx();的这段缓冲区，包括了客户端发来的第一组数据、本地的地址信息、客户端的地址信息
	if (m_client->GetBufferSize() > 0) {
		sockaddr* plocal = NULL, * premote = NULL;
		// 获取与已接受套接字关联的本地地址和远程地址;   接收到一个客户端，会把客户端的socket填写上去（已经准备好的）；
		GetAcceptExSockaddrs(*m_client, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)&plocal, &lLength, // 本地地址
			(sockaddr**)&premote, &rLength  // 远程地址
		);
		memcpy(m_client->GetLocalAddr(), plocal, sizeof(sockaddr_in));
		memcpy(m_client->GetRemoteAddr(), premote, sizeof(sockaddr_in));
		// 客户端连接进来后，绑定到iocp，然后投递WSARecv(); （后续的WSARecv()就不是在这里投递了）
		m_server->BindNewSocket(*m_client);
		int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABuffer(), 1, *m_client, &m_client->flags(), (LPWSAOVERLAPPED)m_client->RecvOverlapped(), NULL);
		if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING)) { // 排除掉这个错误：异步操作仍在进行中; WSA_IO_PENDING表示正在处理
			//TODO:报错
			TRACE("ret=%d  error=%d\r\n", ret, WSAGetLastError());
		}
		if (!m_server->NewAccept()) {  // 开启下一轮的AcceptEx
			return -2;
		}
	}
	return -1;
}

template<MyOperator op>
RecvOverlapped<op>::RecvOverlapped() {
	m_operator = ERecv;
	m_worker = CThreadWorker(this, (FUNCTYPE) & RecvOverlapped<op>::RecvWorker);  // 这里不写<op>也没报错； RecvOverlapped::RecvWorker
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}

template<MyOperator op>
SendOverlapped<op>::SendOverlapped() {
	m_operator = ESend;
	m_worker = CThreadWorker(this, (FUNCTYPE) & SendOverlapped<op>::SendWorker);  // 这里不写<op>也没报错
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}



MyClient::MyClient() 
	:m_isbusy(false), m_flags(0), 
	m_overlapped(new AcceptOVERLAPPED()), 
	m_recv(new RecvOVERLAPPED()),
	m_send(new SendOVERLAPPED()),
	m_vecSend(this, (SENDCALLBACK)& MyClient::SendData)  // 设置不停发送数据的回调函数
{
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
	memset(&m_laddr, 0, sizeof(m_laddr));
	memset(&m_raddr, 0, sizeof(m_raddr));
	m_received = 0;
	m_used = 0;
}

void MyClient::SetOverlapped(PCLIENT& ptr) {
	m_overlapped->m_client = ptr.get();
	m_recv->m_client = ptr.get();
	m_send->m_client = ptr.get();
}

MyClient::operator LPOVERLAPPED() {
	return &m_overlapped->m_overlapped;
}

LPWSABUF MyClient::RecvWSABuffer()
{
	return &m_recv->m_wsabuffer;
}

// IDO: 老师写的返回值是：LPWSAOVERLAPPED，两种可以强制类型转换；  在WSARecv();
LPOVERLAPPED MyClient::RecvOverlapped()
{
	return &m_recv->m_overlapped;
}

LPWSABUF MyClient::SendWSABuffer()
{
	return &m_send->m_wsabuffer;
}

LPOVERLAPPED MyClient::SendOverlapped()
{
	return &m_send->m_overlapped;
}
/*
? 这里还需要调用recv吗？不是IOCP已经帮我们拿到数据了吗? 不是应该直接从RecvOverlapped的缓冲区拿数据就好了嘛？ 【如果这样操作，应该是完全的异步了吧】
并且拿到数据后，解析数据，根据包命令去执行Command(); Command()执行结束后把应答包push进安全队列中；
*/
// 听直播讲到，这整个并不是一个完全的异步，是把IOCP当成epoll来用了？？？
int MyClient::Recv()
{
	int ret = recv(m_sock, m_buffer.data() + m_used, m_buffer.size() - m_used, 0);  // ？
	if (ret <= 0)return -1;
	m_used += (size_t)ret;
	// TODO 解析数据
	CTool::Dump((BYTE*)m_buffer.data(), ret);
	return 0;
}

// 把要发送的数据push进发送队列中
int MyClient::Send(void* buffer, size_t nSize)
{
	std::vector<char> data(nSize);
	memcpy(data.data(), buffer, nSize);
	// PushBack()函数里面会new
	if (m_vecSend.PushBack(data)) {
		return 0;
	}
	return -1;
}

// 取安全队列中front的值进行发送； （在CSendQueue中，如果有数据，会不停地执行此函数进行发送；这个函数就是设置的回调函数）
int MyClient::SendData(std::vector<char>& data)
{
	if (m_vecSend.Size() > 0) {
		int ret = WSASend(m_sock, SendWSABuffer(), 1, &m_received, m_flags, &m_send->m_overlapped, NULL);
		if (ret != 0 && (WSAGetLastError() != WSA_IO_PENDING)) {
			CTool::ShowError();
			return -1;
		}
	}
	return 0;
}



CServer::~CServer()
{
	closesocket(m_sock);
	std::map<SOCKET, PCLIENT>::iterator it = m_client.begin();
	for (; it != m_client.end(); it++) {
		it->second.reset(); // .reset();将智能指针置空;  并调用MyClient的析构
	}
	m_client.clear();  // 把std::map置空
	CloseHandle(m_hIOCP);
	m_pool.Stop();
	WSACleanup();
}

bool CServer::StartService()
{
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

	if (!NewAccept()) {  // 投递异步IO请求 AcceptEx(); .....
		return false;
	}
	return true;
}

bool CServer::NewAccept()
{
	PCLIENT pClient(new MyClient()); // 在AcceptEx之前提前创建好socket
	pClient->SetOverlapped(pClient);
	m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient)); // 此时第一个参数类型就是SOCKET，通过操作符重载*pClient返回的就是类的成员变量SOCKET sock；
	// 比所使用的传输协议的最大地址长度多 16 个字节； 第四个参数如果设置为0，则不会等待数据到来，直接返回
	if (!AcceptEx(m_sock, *pClient, *pClient, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, *pClient, *pClient)) {
		TRACE("%d\r\n", WSAGetLastError());  // 错误代码997，是IO_Pending,不能认为这个是错误的
		if (WSAGetLastError() != WSA_IO_PENDING) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
	}
	return true;
}

void CServer::BindNewSocket(SOCKET s)
{
	CreateIoCompletionPort((HANDLE)s, m_hIOCP, (ULONG_PTR)this, 0);
}

void CServer::CreateSocket()
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	int opt = 1;
	setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));  // 设置地址重用
}

int CServer::threadIocp()
{
	DWORD Transferred = 0;
	ULONG_PTR CompletionKey = 0;
	OVERLAPPED* lpOverlapped = NULL;
	if (GetQueuedCompletionStatus(m_hIOCP, &Transferred, &CompletionKey, &lpOverlapped, INFINITE)) {
		if (CompletionKey != NULL) {  // 通过PostQueuedCompletionStatus();来传递CompletionKey=null表示结束工作线程
			// CONTAINNING_RECORD 根据结构体中成员变量的地址，计算出结构体的地址； 
			// 参数：成员变量地址、结构体类型、成员变量名； 
			// 保证OVERLAPPED m_overlapped;在结构体的第一个位置；才能这种操作，找到结构体MyOverlapped的地址
			MyOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, MyOverlapped, m_overlapped);
			pOverlapped->m_server = this;  // 更新m_server
			TRACE("pOverlapped->m_operator:%d\r\n", pOverlapped->m_operator);
			switch (pOverlapped->m_operator) {
			case EAccept: {
				// AcceptOverlapped<EAccept>* pAccept = (AcceptOverlapped<EAccept>*)pOverlapped;
				// 这里是强制类型转换，基类指针转换为派生类指针（派生类中没有额外的成员变量）
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
				m_pool.DispatchWorker(pSend->m_worker); // 这里发送完成后，并不会执行什么；
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

