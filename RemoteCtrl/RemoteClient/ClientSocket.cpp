#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::m_instance = NULL;  // ��̬��Ա������ʽ��ʼ��
// ������static CHelper m_helper;
CClientSocket::CHelper CClientSocket::m_helper;   // ʵ��
CClientSocket* pServer = CClientSocket::getInstance();


// ��.cpp����ʵ��
std::string GetErrorInfo(int wsaErrCode) {
	std::string ret;
	LPVOID lpMsgBuf = NULL;  // void*
	// ���� FormatMessage �������ú������ڽ�ϵͳ�������ת��Ϊ��Ӧ�Ŀɶ�������Ϣ��
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
	LocalFree(lpMsgBuf);  // �ͷ�ͨ�� FormatMessage �����������Ϣ���������Է�ֹ�ڴ�й©��
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
		MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);
	if (WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT) {
		TRACE("������Ϣ�����߳�����ʧ�ܣ�\r\n");
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
			TRACE("����ʧ�ܣ���Ϣֵ��%d������ֵ��%08X, ��ţ�%d\r\n", funcs[i].message, funcs[i].func, i);
	}
}

CClientSocket::CClientSocket(const CClientSocket& ss) {  // ��ֹ��������
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
	TRACE("InitSocket client m_sock��%d\r\n", m_sock);
	if (m_sock == -1) return false;
	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(m_nIP);
	serv_addr.sin_port = htons(m_nPort);
	// TRACE("nIP��%08x  nPort:%d\r\n", m_nIP, m_nPort);
	if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
		AfxMessageBox("ָ����ip��ַ������");
		return false;
	}
	int ret = connect(m_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	if (ret == -1) {
		AfxMessageBox("����ʧ��");
		TRACE("����ʧ�ܣ�%d %s\r\n", WSAGetLastError(), GetErrorInfo(WSAGetLastError()).c_str());
		return false;
	}
	TRACE("socket init done!\r\n");
	return true;
}


unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc2();  // ��ΪthreadFunc2()
	_endthreadex(0);
	return 0;
}


// ��ͨ���¼�������֪ͨ�� hEvent��std::map<HANDLE, std::list<CPacket>&>�� std::list<CPacket>;
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
				TRACE("����ʧ�ܣ�\r\n");				
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
						if (size > 0) {  // TODO �����ļ�����Ϣ��ȡ���ļ���Ϣ��ȡ���ܲ�������
							// TODO֪ͨ��Ӧ���¼�
							pack.hEvent = head.hEvent;
							it->second.push_back(pack);
							index -= size;
							memmove(pBuffer, pBuffer + size, index);
							TRACE("SetEvent:%d   it0->second:%d\r\n", pack.sCmd, it0->second);
							if (it0->second == true) {
								SetEvent(head.hEvent);
								break;  // �߼�����
							}
						}
					}
					else if ((length <= 0) && (index <= 0)) {
						CloseSocket();
						SetEvent(head.hEvent); // �ȵ�����˹ر�����֮����֪ͨ�������
						// TRACE("SetEvent:%d   it0->second:%d\r\n", head.sCmd, it0->second); // ��erase(it0)֮ǰ���鿴it0->second
						if (it0 != m_mapAutoClose.end()) {
							TRACE("SetEvent:%d   it0->second:%d\r\n", head.sCmd, it0->second);
							//m_lock.lock();
							//m_mapAutoClose.erase(it0);
							//m_lock.unlock();
						}
						else {
							TRACE("�쳣�����û�ж�Ӧ��pair\r\n");
						}
						break;
					}
				} while (it0->second == false);
			}			
			m_lock.lock();
			m_mapAutoClose.erase(head.hEvent);
			m_lstSend.pop_front(); // �����ļ�������ܻ��ж��CPacket�����pop�����������ò���HANDLE
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
	bool ret = PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)pData, (LPARAM)hWnd);  // �ǵ�delete
	if (ret == false) {  // ���û��PostThreadMessage�ɹ�����ônew�����ľͲ��ᱻ���յ���ֻ��������delete��
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
	WaitForSingleObject(pack.hEvent, INFINITE); // -1 ���޵ȴ�
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
			(this->*(it->second))(msg.message, msg.wParam, msg.lParam);   // ִ�� CClientSocket::SendPack()
			// (this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}
	}
}
// TODO:����һ����Ϣ�����ݽṹ�����ݺ����ݳ��ȣ�ģʽ[�ӵ�һ��Ӧ����͹رջ���Ҫ���ն���Źر�]����
// TODO:�ص���Ϣ�����ݽṹ(Ҫ֪�����ĸ����ڵľ��HWND)
// wParam������PACKET_DATA�ṹ����Ϣ�� lParam������HWND���͵ģ�HWND��ʾ���ھ����
void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam) {
	// �������SendPack()�����WPARAM wParam�Ǿֲ����������ص����⣻  ���ȿ���������Ȼ���ͷŵ�ԭ�ȵģ�
	PACKET_DATA data = *(PACKET_DATA*)wParam;  // �õ����Զ���ṹ����д�Ŀ������캯���� Ȼ��data��һ��ջ�еľֲ��������Զ����٣�
	delete (PACKET_DATA*)wParam;  // ʹ�þֲ��������������󣬵������٣���ֹ����delete��
	HWND hWnd = (HWND)lParam;
	if (InitSocket() == true) {		
		int ret = send(m_sock, data.strData.c_str(), data.strData.size(), 0);
		if (ret > 0) {
			size_t index = 0;
			std::string strBuffer;    // �ֲ��������Զ�����
			strBuffer.resize(BUFFER_SIZE);  // resize��Ҫ���׶�string���и�ֵ����Ϊ��ı丳�����ڴ棻
			char* pBuffer = (char*)strBuffer.c_str();  // �õ�string��ָ�뷽��ʹ��
			while (m_sock != INVALID_SOCKET) {
				int length = recv(m_sock, pBuffer +index, BUFFER_SIZE-index, 0);
				if ((length > 0) || (index > 0)) {
					index += (size_t)length;
					size_t nLen = index;
					CPacket pack((BYTE*)pBuffer, nLen);
					if (nLen > 0) { // �⵽��
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);  // �ڽ��յĵط�Ҫ�ǵ�delete
						if (data.nMode & CSM_AUTOCLOSE) { // ������Զ��ر�ģʽ						
							CloseSocket();
							return;
						}						
					}
					index -= nLen;
					memmove((void*)pBuffer, pBuffer + nLen, index);  // ��ʦд����memmove((void*)pBuffer, pBuffer + index, nLen); 
				}
				else { // TODO �Է��ر����׽��֣����������豸�쳣
					CloseSocket();
					::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, 1);
				}
			}			
		}
		else{
			CloseSocket();
			// TODO ������ֹ����
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1);
		}
	}
	else {
		// TODO ������;  InitSocket()=false;
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2);
	}
}
