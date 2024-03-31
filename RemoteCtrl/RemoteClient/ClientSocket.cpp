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

CClientSocket::CClientSocket() :
	m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), 
	m_bAutoClose(true), m_hThread(INVALID_HANDLE_VALUE) {
	if (InitSockEnv() == FALSE) {
		MessageBox(NULL, _T("无法初始化套接字环境"), _T("初始化错误"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);
	if (WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT) {
		TRACE("网络消息处理线程启动失败！\r\n");
	}
	CloseHandle(m_eventInvoke);
	
	m_buffer.resize(BUFFER_SIZE);
	memset(m_buffer.data(), 0, BUFFER_SIZE);

	struct {
		UINT message;
		MSGFUNC func;
	}funcs[] = {
		{WM_SEND_PACK, &CClientSocket::SendPack},
		// {WM_SEND_PACK, },
		{0, NULL}
	};
	for (int i = 0; funcs[i].message != 0; i++) {
		if (m_mapFunc.insert(std::pair<UINT, MSGFUNC>(funcs[i].message, funcs[i].func)).second == false)
			TRACE("插入失败，消息值：%d，函数值：%08X, 序号：%d\r\n", funcs[i].message, funcs[i].func, i);
	}
}

CClientSocket::CClientSocket(const CClientSocket& ss) {  // 禁止拷贝构造
	m_hThread = INVALID_HANDLE_VALUE;
	m_bAutoClose = ss.m_bAutoClose;
	m_sock = ss.m_sock;
	m_nIP = ss.m_nIP;
	m_nPort = ss.m_nPort;
	std::map<UINT, MSGFUNC>::const_iterator it = ss.m_mapFunc.begin();
	for (; it != ss.m_mapFunc.end(); it++) {
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(it->first, it->second));
	}
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
	// TRACE("nIP：%08x  nPort:%d\r\n", m_nIP, m_nPort);
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


unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc2();  // 改为threadFunc2()
	_endthreadex(0);
	return 0;
}


// 是通过事件机制来通知； hEvent、std::map<HANDLE, std::list<CPacket>&>、 std::list<CPacket>;
/*
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
*/

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClose, WPARAM wParam) {
	UINT nMode = isAutoClose ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam);
	bool ret = PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)pData, (LPARAM)hWnd);  // 记得delete
	if (ret == false) {  // 如果没有PostThreadMessage成功，那么new出来的就不会被接收到，只能在这里delete；
		delete pData;
	}
	return ret;
}

/*
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
*/

void CClientSocket::threadFunc2()
{
	SetEvent(m_eventInvoke);
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		TRACE("Get Message:%08X \r\n", msg.message);
		std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
		if (it != m_mapFunc.end()) {
			(this->*(it->second))(msg.message, msg.wParam, msg.lParam);   // 执行 CClientSocket::SendPack()
			// (this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}
	}
}
// TODO:定义一个消息的数据结构（数据和数据长度，模式[接到一个应答包就关闭还是要接收多个才关闭]）；
// TODO:回调消息的数据结构(要知道是哪个窗口的句柄HWND)
// wParam参数是PACKET_DATA结构体消息； lParam参数是HWND类型的，HWND表示窗口句柄；
void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam) {
	// 如果调用SendPack()传入的WPARAM wParam是局部变量，隐藏的问题；  就先拷贝过来，然后释放掉原先的；
	PACKET_DATA data = *(PACKET_DATA*)wParam;  // 用到了自定义结构体里写的拷贝构造函数； 然后data是一个栈中的局部变量，自动销毁；
	delete (PACKET_DATA*)wParam;  // 使用局部变量拷贝出来后，当场销毁，防止遗忘delete；
	HWND hWnd = (HWND)lParam;
	if (InitSocket() == true) {		
		int ret = send(m_sock, data.strData.c_str(), data.strData.size(), 0);
		if (ret > 0) {
			size_t index = 0;
			std::string strBuffer;    // 局部变量，自动回收
			strBuffer.resize(BUFFER_SIZE);  // resize后不要轻易对string进行赋值，因为会改变赋给的内存；
			char* pBuffer = (char*)strBuffer.c_str();  // 拿到string的指针方便使用
			while (m_sock != INVALID_SOCKET) {
				int length = recv(m_sock, pBuffer +index, BUFFER_SIZE-index, 0);
				if ((length > 0) || (index > 0)) {
					index += (size_t)length;
					size_t nLen = index;
					CPacket pack((BYTE*)pBuffer, nLen);
					if (nLen > 0) { // 解到包
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);  // 在接收的地方要记得delete
						if (data.nMode & CSM_AUTOCLOSE) { // 如果是自动关闭模式						
							CloseSocket();
							return;
						}						
					}
					index -= nLen;
					memmove((void*)pBuffer, pBuffer + nLen, index);  // 老师写错了memmove((void*)pBuffer, pBuffer + index, nLen); 
				}
				else { // TODO 对方关闭了套接字，或者网络设备异常
					CloseSocket();
					::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, 1);
				}
			}			
		}
		else{
			CloseSocket();
			// TODO 网络终止处理
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1);
		}
	}
	else {
		// TODO 错误处理;  InitSocket()=false;
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2);
	}
}
