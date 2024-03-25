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

void CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc();
	// _endthread();
}

void CClientSocket::threadFunc()
{
	if (InitSocket() == false) {
		return;
	}
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index = 0;  // static???
	while (m_sock != INVALID_SOCKET) {
		if (m_lstSend.size() > 0) {
			CPacket& head = m_lstSend.front();
			if (Send(head) == false) {
				TRACE("����ʧ�ܣ�\r\n");
				continue;
			}
			auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>>(head.hEvent, std::list<CPacket>()));
			int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
			if (length > 0 || index > 0) {
				index += length;
				size_t size = (size_t)index;
				CPacket pack((BYTE*)pBuffer, size);
				if (size > 0) {  // TODO �����ļ�����Ϣ��ȡ���ļ���Ϣ��ȡ���ܲ�������
					// TODO֪ͨ��Ӧ���¼�
					pack.hEvent = head.hEvent;
					pr.first->second.push_back(pack);
					SetEvent(head.hEvent);
				}
			}
			else if (length <= 0 && index <= 0) {
				CloseSocket();
			}
			m_lstSend.pop_front(); // �����ļ�������ܻ��ж��CPacket�����pop�����������ò���HANDLE
		}		
	}
}
