#pragma once

#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#include <list>
#include <map>
#include <mutex>

#define WM_SEND_PACK (WM_USER+1)  // ���Ͱ�����
void Dump(BYTE* pData, size_t nSize);

#pragma pack(push, 1)  // ���Ķ��뷽ʽ�� �������CPacket�ཫ����һ���ֽڵı߽���룻
/*
	#pragma pack(1) ָ���˽ṹ���Ա�Ķ��뷽ʽΪ 1 �ֽڡ�
	����ζ�Žṹ��ĳ�Ա���������У����������ǵ���Ȼ���롣
	�������Խ�ʡ�ڴ�ռ䣬�����ܻ����ӷ��ʳ�Ա�Ŀ�����
*/
class CPacket
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;  // std::string���Ϳ���ֱ��=������std::string�ĸ�ֵ��������أ� �������char*����Ҫ��strcpy()
		sSum = pack.sSum;
		hEvent = pack.hEvent;
	}

	CPacket(WORD nCmd, const BYTE* pData, size_t nSize, HANDLE hEvent)  // ���,���ﴫ���nSize��data�ĳ���
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
		this->hEvent = hEvent;
		// TRACE("CPacket: sHead:%08X, nLength:%d, sCmd:%d, sSum:%d\r\n", sHead, nLength, sCmd, sSum);
	}

	CPacket(const BYTE* pData, size_t& nSize):hEvent(INVALID_HANDLE_VALUE)  // ���ݰ�����������е����ݷ��䵽��Ա������
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
			i += (nLength - 4);
			// TRACE("%s\r\n", strData.c_str() + 12);  // ����FILEINFO�ṹ��Ĵ�С(12)???
		}
		sSum = *(WORD*)(pData + i);
		i += 2;

		// У��
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
			hEvent = pack.hEvent; 
		}
		return *this;
	}
	int Size() {  // ����������Ĵ�С
		return nLength + 4 + 2;
	}
	const char* Data(std::string& strOut) const{
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();  // ����һ��ָ�룬ָ��strOut��
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();  // pData��һ��char*��������Ҫmemcpy()
		*(WORD*)pData = sSum;
		return strOut.c_str();  // ��󷵻�strOut��ָ��
	}

public:
	WORD        sHead;   // ��ͷ �̶�λFEEF                       2�ֽ�
	DWORD       nLength; // ���� (�ӿ������ʼ������У�����)   4�ֽ�
	WORD        sCmd;    // �������� (���Ƕ���)                   2�ֽ�
	std::string strData; // ������                             
	WORD        sSum;    // ��У��                                2�ֽ�
	// std::string strOut;  // ������������
	HANDLE hEvent;
};
#pragma pack(pop)


typedef struct file_info {
	file_info()  // �ṹ������Ҳ���Թ��캯��
	{
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;       // �Ƿ���Ч    
	BOOL IsDirectory;     // �Ƿ�ΪĿ¼  0��1��
	BOOL HasNext;         // �Ƿ��к��� 
	char szFileName[256]; // �ļ���
}FILEINFO, * PFILEINFO;

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
		// ��̬��Ա����û��thisָ�룬ֻ�ܷ��ʾ�̬��Ա������m_instanceΪ��̬��Ա����
		if (m_instance == NULL)  // ��������ʱ���̰߳�ȫ�ģ�����Ҫmutex����Ϊ����main֮ǰִ�еġ�
		{
			m_instance = new CClientSocket();
		}
		return m_instance;
	}
	bool InitSocket();


#define BUFFER_SIZE 2048000  // ͼƬ�Ƚϴ���������󻺳�������һֱ������
	int DealCommand() {
		if (m_sock == -1) return -1;
		// char* buffer = new char[BUFFER_SIZE];
		char* buffer = m_buffer.data();  // .data() ��Ա��������ָ�������е�һ��Ԫ�ص�ָ��
		// memset(buffer, 0, BUFFER_SIZE);  // ������������ջ�����������
		static size_t index = 0;  // �����indexҲ���������0
		while (true) {
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);  // �������Ĵ�������������
			// recv()����Ϊ0��ʱ�򻹲��ܽ�������ʱ��������������
			if (((int)len <= 0) && ((int)index <= 0))   // ���һ�����Ϊ�ղ��˳�   TODO:recv()����������int��size_t��unsigned int���ͣ�
			{
				return -1;
			}
			// Dump((BYTE*)buffer, index);
			index += len;  // !!!
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);  // ����ڶ�������len��������ã���ı���ֵ;(���Ժ����len��ֵ��һ������֮ǰ��ֵ)
			if (len > 0)   // �����len��ʾ���ǣ��õ���һ�������ݰ��ĳ���
			{
				index -= len;
				memmove(buffer, buffer + len, index);   // �������Ĵ�������������
				return m_packet.sCmd;  // ������һ������ָ��
			}
			// ���len==0 ��ʾ��������û��һ�������ݰ������ü���whileѭ��
		}
		return -1;
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
	void UpdateAddress(int nIP, int nPort) {
		if ((m_nIP != nIP) || (m_nPort != nPort)) {
			m_nIP = nIP;
			m_nPort = nPort;
		}
	}

	bool SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClose = true);


private:
	typedef void(CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	std::map<UINT, MSGFUNC> m_mapFunc;
	HANDLE m_hThread;
	std::mutex m_lock;
	bool m_bAutoClose;
	std::list<CPacket> m_lstSend;
	std::map<HANDLE, std::list<CPacket>&> m_mapAck;
	std::map<HANDLE, bool> m_mapAutoClose;
	int m_nIP;
	int m_nPort;
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet;
	CClientSocket() :m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_bAutoClose(true), m_hThread(INVALID_HANDLE_VALUE){
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE);
		memset(m_buffer.data(), 0, BUFFER_SIZE);
	}
	CClientSocket(const CClientSocket& ss) {  // ��ֹ��������
		m_hThread = INVALID_HANDLE_VALUE;
		m_bAutoClose = ss.m_bAutoClose;
		m_sock = ss.m_sock;
		m_nIP = ss.m_nIP;
		m_nPort = ss.m_nPort;
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
	CClientSocket& operator=(const CClientSocket& ss) {}   // ��ֹ������ֵ
	~CClientSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		WSACleanup();
	}
	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		}
		return TRUE;
	}

	bool Send(const CPacket& packet) // �޸��ˣ�������const��packet.Data()����ı��Աֵ�����Ǵ�����һ������
	{
		TRACE("Send sCmd=%d\r\n", packet.sCmd);
		if (m_sock == -1) return false;
		std::string strOut;  // ����packet.Data(strOut)�õ�����������������
		packet.Data(strOut);
		return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
		//std::string strOut;
		// ������ջ˳���Ǵ�������,��ֻ���ڻ�ȡ�ڶ���������ʱ��Ÿı���str��ֵ�� ��������strOut.size()��Զ��Ϊ0��
		//return send(m_sock, packet.Data(strOut), strOut.size(), 0) > 0;   // �����ԣ�����д������
	}

	bool Send(const char* msg, int nSize) {
		if (m_sock == -1) return false;
		return send(m_sock, msg, nSize, 0) > 0;
	}

	void SendPack(UINT nMsg, WPARAM pParam/*��������ֵ*/, LPARAM lParam/*�������ĳ���*/);


	static void threadEntry(void* arg);
	void threadFunc();
	void threadFunc2();


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

