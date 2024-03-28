#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::m_instance = NULL;  // 静态成员函数显式初始化
// 声明：static CHelper m_helper;
CClientSocket::CHelper CClientSocket::m_helper;   // 实现
CClientSocket* pServer = CClientSocket::getInstance();


// 在.cpp里面实现
std::string GetErrorInfo(int wsaErrCode) {
	std::string ret;
	LPVOID lpMsgBuf = NULL;  // void*
	// 调用 FormatMessage 函数，该函数用于将系统错误代码转换为相应的可读错误信息。
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);  // 释放通过 FormatMessage 函数分配的消息缓冲区，以防止内存泄漏。
	return ret;
}


void Dump(BYTE* pData, size_t nSize) {
	std::string strOut;
	for (size_t i = 0; i < nSize; i++) {
		char buf[8] = "";
		if (i > 0 && (i % 16 == 0)) strOut += "\n";
		snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
		strOut += buf;
	}
	strOut += '\n';
	OutputDebugStringA(strOut.c_str());
}


bool CClientSocket::InitSocket()
{
	if (m_sock != INVALID_SOCKET) CloseSocket();
	m_sock = socket(PF_INET, SOCK_STREAM, 0);
	TRACE("InitSocket client m_sock：%d\r\n", m_sock);
	if (m_sock == -1) return false;
	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(m_nIP);
	serv_addr.sin_port = htons(m_nPort);
	// TRACE("serv nIP：%08x  nPort:%d\r\n", m_nIP, m_nPort);
	if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
		AfxMessageBox("指定的ip地址不存在");
		return false;
	}
	int ret = connect(m_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	if (ret == -1) {
		AfxMessageBox("连接失败");
		TRACE("连接失败：%d %s\r\n", WSAGetLastError(), GetErrorInfo(WSAGetLastError()).c_str());
		return false;
	}
	TRACE("socket init done!\r\n");
	return true;
}


void CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc();
	_endthread();
}

void CClientSocket::threadFunc()
{
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index = 0;  // static???
	InitSocket();
	while (m_sock != INVALID_SOCKET) {
		if (m_lstSend.size() > 0) {	
			InitSocket();
			TRACE("m_lstSend size:%d\r\n", m_lstSend.size());
			m_lock.lock();
			CPacket& head = m_lstSend.front();
			m_lock.unlock();
			if (Send(head) == false) {
				TRACE("发包失败！\r\n");				
				continue;
			}
			std::map<HANDLE, std::list<CPacket>&>::iterator it;
			m_lock.lock();
			it = m_mapAck.find(head.hEvent);
			m_lock.unlock();
			if (it != m_mapAck.end()) {
				std::map<HANDLE, bool>::iterator it0;
				it0 = m_mapAutoClose.find(head.hEvent);
				do {
					int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
					TRACE("recv:%d  index:%d\r\n", length, index);
					if ((length > 0) || (index > 0)) {
						index += length;
						size_t size = (size_t)index;
						CPacket pack((BYTE*)pBuffer, size);
						if (size > 0) {  // TODO 对于文件夹信息获取，文件信息获取可能产生问题
							// TODO通知对应的事件
							pack.hEvent = head.hEvent;
							it->second.push_back(pack);
							index -= size;
							memmove(pBuffer, pBuffer + size, index);
							TRACE("SetEvent:%d   it0->second:%d\r\n", pack.sCmd, it0->second);
							if (it0->second == true) {
								SetEvent(head.hEvent);
								break;  // 逻辑？？
							}
						}
					}
					else if ((length <= 0) && (index <= 0)) {
						CloseSocket();
						SetEvent(head.hEvent); // 等到服务端关闭命令之后，再通知事情完成
						// TRACE("SetEvent:%d   it0->second:%d\r\n", head.sCmd, it0->second); // 在erase(it0)之前，查看it0->second
						if (it0 != m_mapAutoClose.end()) {
							TRACE("SetEvent:%d   it0->second:%d\r\n", head.sCmd, it0->second);
							//m_lock.lock();
							//m_mapAutoClose.erase(it0);
							//m_lock.unlock();
						}
						else {
							TRACE("异常情况，没有对应的pair\r\n");
						}
						break;
					}
				} while (it0->second == false);
			}			
			m_lock.lock();
			m_mapAutoClose.erase(head.hEvent);
			m_lstSend.pop_front(); // 对于文件传输可能会有多个CPacket，如果pop出，后续会拿不到HANDLE
			m_lock.unlock();
			if (InitSocket() == false) {
				InitSocket();
			}			
		}
		Sleep(1);
	}
	CloseSocket();
}

bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClose) {
	if (m_sock == INVALID_SOCKET && m_hThread==INVALID_HANDLE_VALUE) {
		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);
		TRACE("start thread\r\n");
	}
	m_lock.lock();
	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(pack.hEvent, lstPacks));
	m_mapAutoClose.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClose));
	m_lstSend.push_back(pack);
	m_lock.unlock();
	TRACE("cmd:%d  hEvent:%08X  threadid:%d\r\n", pack.sCmd, pack.hEvent, GetCurrentThreadId());
	WaitForSingleObject(pack.hEvent, INFINITE); // -1 无限等待
	TRACE("cmd:%d  hEvent:%08X  threadid:%d\r\n", pack.sCmd, pack.hEvent, GetCurrentThreadId());
	std::map<HANDLE, std::list<CPacket>&>::iterator it;
	m_lock.lock();
	it = m_mapAck.find(pack.hEvent);
	m_lock.unlock();
	if (it != m_mapAck.end()) {
		m_lock.lock();
		m_mapAck.erase(it);
		m_lock.unlock();
		return true;
	}
	return false;
}

void CClientSocket::threadFunc2()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
		if (it != m_mapFunc.end()) {
			(this->*(it->second))(msg.message, msg.wParam, msg.lParam);
			// (this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}
	}
}


void CClientSocket::SendPack(UINT nMsg, WPARAM pParam, LPARAM lParam) {
	// TODO:定义一个消息的数据结构（数据和数据长度，模式[接到一个应答包就关闭还是要接收多个]）；
	// TODO:回调消息的数据结构(要知道是哪个窗口的句柄HWND；要知道回调什么消息MESSAGE)
	if (InitSocket() == true) {
		int ret = send(m_sock, (char*)pParam, (int)lParam, 0);
		if (ret > 0) {
		}
		else{
			CloseSocket();
			// TODO 网络终止处理
		}
	}
	else {
		// TODO 错误处理
	}
}
