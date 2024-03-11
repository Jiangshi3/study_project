#pragma once
#include "pch.h"
#include "framework.h"
void Dump(BYTE* pData, size_t nSize);
/*
typedef unsigned long       DWORD;  // 4�ֽ�
typedef int                 BOOL;   
typedef unsigned char       BYTE;  
typedef unsigned short      WORD;   // 2�ֽ�
typedef unsigned long long  size_t; // 8�ֽ�
*/


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
			if (*(WORD*)(pData+i) == 0xFEFF) {
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
			i += (nLength - 4);  // �Լ�д����-2,û��debug����  :( 
		}
		sSum = *(WORD*)(pData + i);
		i += 2;

		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			// �� strData[j] ת��Ϊ BYTE ���Ͳ����а�λ�������ȷ��ֻȡ�ַ��ĵ�8λ,������κβ���Ҫ�ĸ�λӰ�졣
			sum += BYTE(strData[j]) & 0xFF;   //ɶ���⣿ 
		}
		if (sum == sSum) {  // У��
			nSize = i;
			return;
		}
		nSize = 0;
	}
	~CPacket() {}
	CPacket& operator=(const CPacket& pack){
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


typedef struct file_info {
	file_info()  // �ṹ������Ҳ���Թ��캯��
	{
		IsInvaild = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvaild;       // �Ƿ���Ч    
	BOOL IsDirectory;     // �Ƿ�ΪĿ¼  0��1��
	BOOL HasNext;         // �Ƿ��к��� 
	char szFileName[256]; // �ļ���
}FILEINFO, * PFILEINFO;

typedef struct MouseEvent{
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction; // 0��ʾ������1��ʾ˫����2��ʾ���£�3��ʾ�ſ���4��������
	WORD nButton; // 0��ʾ�����1��ʾ�Ҽ���2��ʾ�м���3û�а���
	POINT ptXY;   // ����
}MOUSEEV,*PMOUSEEV;


class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		// ��̬��Ա����û��thisָ�룬ֻ�ܷ�λ��̬��Ա������m_instanceΪ��̬��Ա����
		if (m_instance == NULL)  // ��������ʱ���̰߳�ȫ�ģ�����Ҫmutex����Ϊ����main֮ǰִ�еġ�
		{
			m_instance = new CServerSocket(); 
		}
		return m_instance;
	}
	bool InitSocket() {
		if (m_servSock == -1) return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(9527);
		// ��
		if (bind(m_servSock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) return false;
		if (listen(m_servSock, 1) == -1) return false;
		return true;
	}

	bool AcceptClient() {
		TRACE("enter AcceptClient()\r\n");
		sockaddr_in clnt_addr;
		int clnt_len = sizeof(clnt_addr);
		m_clntSock = accept(m_servSock, (struct sockaddr*)&clnt_addr, &clnt_len);
		TRACE("m_clntSock=%d\r\n", m_clntSock);
		if (m_clntSock == -1)return false;
		return true;
	}

#define BUFFER_SIZE 4096
	int DealCommand() {
		if (m_clntSock == -1) return -1;
		char* buffer = new char[BUFFER_SIZE];  
		if (buffer == NULL) {
			TRACE("�ڴ治��\r\n");
		}
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true) {
			size_t len = recv(m_clntSock, buffer+index, BUFFER_SIZE-index, 0);  // �������Ĵ�������������
			if (len <= 0) {
				delete[] buffer;
				return -1;
			}
			index += len;  // !!!
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);  // ����ڶ�������len��������ã���ı���ֵ;(���Ժ����len��ֵ��һ������֮ǰ��ֵ)
			if (len > 0)   // �����len��ʾ���ǣ��õ���һ�������ݰ��ĳ���
			{
				index -= len; 
				memmove(buffer, buffer + len, BUFFER_SIZE - len);   // �������Ĵ�������������
				delete[] buffer;
				return m_packet.sCmd;  // ������һ������ָ��
			}
			// ���len==0 ��ʾ��������û��һ�������ݰ������ü���whileѭ��
		}
		delete[] buffer;
		return -1;
	}

	CPacket& GetPacket() {
		return m_packet;
	}

	bool Send(const char* msg, int nSize) {
		if (m_clntSock == -1) return false;
		return send(m_clntSock, msg, nSize, 0) > 0;
	}
	bool Send(CPacket& packet) // ���ﲻ��ʹ��const�ˣ���Ϊpacket.Data()��ı��Աֵ
	{
		if (m_clntSock == -1) return false;
		Dump((BYTE*)packet.Data(), packet.Size());
		return send(m_clntSock, packet.Data(), packet.Size(), 0) > 0; 
	}
	bool GetFilePath(std::string& strPath) {
		// if((m_packet.sCmd >= 2)&&(m_packet.sCmd <= 4))
		if ((m_packet.sCmd == 2) || (m_packet.sCmd == 3) || (m_packet.sCmd == 4) || (m_packet.sCmd == 9)) 
		{
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

	void CloseClient() {
		closesocket(m_clntSock);
		m_clntSock = INVALID_SOCKET;
	}

private:
	SOCKET m_servSock;
	SOCKET m_clntSock;
	CPacket m_packet;
	CServerSocket() {
		m_servSock = INVALID_SOCKET; // -1
		m_clntSock = INVALID_SOCKET;
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_servSock = socket(PF_INET, SOCK_STREAM, 0);
	}
	CServerSocket(const CServerSocket&) {}              // ��ֹ��������
	CServerSocket& operator=(const CServerSocket&) {}   // ��ֹ������ֵ
	~CServerSocket() {
		closesocket(m_servSock);
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
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}

	class CHelper{
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releseInstance();
		}
	};

	static CServerSocket* m_instance;
	static CHelper m_helper;
};

// extern CServerSocket server;
// extern CServerSocket* pServer;
