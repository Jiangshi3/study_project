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