#pragma once

#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>

#pragma pack(push, 1)  // ���Ķ��뷽ʽ�� �������CPacket�ཫ����һ���ֽڵı߽���룻
class CPacket
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}

	CPacket(WORD nCmd, const BYTE* pData, size_t nSize)  // ���,���ﴫ���nSize��data�ĳ���
	{
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		sSum = 0;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
			for (size_t i = 0; i < nSize; i++) {
				sSum += (BYTE)pData[i] & 0xFF;
			}
		}
		else {
			strData.clear();
		}
	}

	CPacket(const BYTE* pData, size_t& nSize)  // ���ݰ�����������е����ݷ��䵽��Ա������
	{
		size_t i = 0;
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2; // 2�ֽ�
				break;
			}
		}
		// 4�ֽ�nLength; 2�ֽ�sCmd��2�ֽ�sSum;
		if (i + 4 + 2 + 2 > nSize) { // �����ݲ�ȫ�����߰�ͷδ��ȫ�����յ�
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i);
		i += 4;
		if (nLength + i > nSize) {  // ��û����ȫ���յ��� �ͷ��أ�����ʧ��
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i);
		i += 2;
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2); // ע��nLength������������������ݡ���У��ĳ���
			memcpy((void*)strData.c_str(), (pData + i), (nLength - 2 - 2));  // ��ȡ����
			i += (nLength - 2);
		}
		sSum = *(WORD*)(pData + i);
		i += 2;

		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			// �� strData[j] ת��Ϊ BYTE ���Ͳ����а�λ�������ȷ��ֻȡ�ַ��ĵ�8λ,������κβ���Ҫ�ĸ�λӰ�졣
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) {  // У��
			nSize = i;
			return;
		}
		nSize = 0;
	}
	~CPacket() {}
	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) {
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}
	int Size() {  // ����������Ĵ�С
		return nLength + 4 + 2;
	}
	const char* Data() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();  // ����һ��ָ�룬ָ��strOut��
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();  // ��󷵻�strOut��ָ��
	}

public:
	WORD        sHead;   // ��ͷ �̶�λFEEF                       2�ֽ�
	DWORD       nLength; // ���� (�ӿ������ʼ������У�����)   4�ֽ�
	WORD        sCmd;    // �������� (���Ƕ���)                   2�ֽ�
	std::string strData; // ������                             
	WORD        sSum;    // ��У��                                2�ֽ�
	std::string strOut;  // ������������
};
#pragma pack(pop)


typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction; // 0��ʾ������1��ʾ˫����2��ʾ���£�3��ʾ�ſ���4��������
	WORD nButton; // 0��ʾ�����1��ʾ�Ҽ���2��ʾ�м���3û�а���
	POINT ptXY;   // ����
}MOUSEEV, * PMOUSEEV;


// ����д��.cpp�ļ��ڣ���Ϊ��д����������ļ�#include "ClientSocket.h"ʱ���ҵĻᱨ��ȷ��ֻ������һ��
std::string GetErrorInfo(int wsaErrCode);  

class CClientSocket
{
public:
	static CClientSocket* getInstance() {
		// ��̬��Ա����û��thisָ�룬ֻ�ܷ�λ��̬��Ա������m_instanceΪ��̬��Ա����
		if (m_instance == NULL)  // ��������ʱ���̰߳�ȫ�ģ�����Ҫmutex����Ϊ����main֮ǰִ�еġ�
		{
			m_instance = new CClientSocket();
		}
		return m_instance;
	}
	bool InitSocket(const std::string strIpAddress) {
		if (m_sock != INVALID_SOCKET) CloseSocket();
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
		if (m_sock == -1) return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = inet_addr(strIpAddress.c_str());
		serv_addr.sin_port = htons(9527);
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
		return true;
	}

#define BUFFER_SIZE 4096
	int DealCommand() {
		if (m_sock == -1) return -1;
		// char* buffer = new char[BUFFER_SIZE];
		char* buffer = m_buffer.data();  // .data() ��Ա��������ָ�������е�һ��Ԫ�ص�ָ��
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true) {
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);  // �������Ĵ�������������
			if (len <= 0) {
				return -1;
			}
			index += len;  // !!!
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);  // ����ڶ�������len��������ã���ı���ֵ;(���Ժ����len��ֵ��һ������֮ǰ��ֵ)
			if (len > 0)   // �����len��ʾ���ǣ��õ���һ�������ݰ��ĳ���
			{
				index -= len;
				memmove(buffer, buffer + len, BUFFER_SIZE - len);   // �������Ĵ�������������
				return m_packet.sCmd;  // ������һ������ָ��
			}
			// ���len==0 ��ʾ��������û��һ�������ݰ������ü���whileѭ��
		}
		return -1;
	}

	bool Send(const char* msg, int nSize) {
		if (m_sock == -1) return false;
		return send(m_sock, msg, nSize, 0) > 0;
	}
	bool Send(CPacket& packet) // ���ﲻ��ʹ��const�ˣ���Ϊpacket.Data()��ı��Աֵ
	{
		if (m_sock == -1) return false;
		return send(m_sock, packet.Data(), packet.Size(), 0) > 0;
	}
	bool GetFilePath(std::string& strPath) {
		// if((m_packet.sCmd >= 2)&&(m_packet.sCmd <= 4))
		if ((m_packet.sCmd == 2) || (m_packet.sCmd == 3) || (m_packet.sCmd == 4)) {
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}
	bool GetMouseEvent(MOUSEEV& mouse)  // ��꣺�ƶ����������һ���˫������Ҫһ���ṹ�壻
	{
		if (m_packet.sCmd == 5) {
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true;
		}
		return false;
	}

	CPacket& GetPacket() {
		return m_packet;
	}

	void CloseSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}

private:
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet;
	CClientSocket() {
		m_sock = INVALID_SOCKET; // -1
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE);
	}
	CClientSocket(const CClientSocket&) {}              // ��ֹ��������
	CClientSocket& operator=(const CClientSocket&) {}   // ��ֹ������ֵ
	~CClientSocket() {
		closesocket(m_sock);
		WSACleanup();
	}
	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		}
		return TRUE;
	}
private:
	static void releseInstance() {
		if (m_instance != NULL) {
			CClientSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}

	class CHelper {
	public:
		CHelper() {
			CClientSocket::getInstance();
		}
		~CHelper() {
			CClientSocket::releseInstance();
		}
	};

	static CClientSocket* m_instance;
	static CHelper m_helper;
};

