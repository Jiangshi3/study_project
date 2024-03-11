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
