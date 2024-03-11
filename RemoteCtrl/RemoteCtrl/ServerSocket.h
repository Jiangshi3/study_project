#pragma once
#include "pch.h"
#include "framework.h"
void Dump(BYTE* pData, size_t nSize);
/*
typedef unsigned long       DWORD;  // 4字节
typedef int                 BOOL;   
typedef unsigned char       BYTE;  
typedef unsigned short      WORD;   // 2字节
typedef unsigned long long  size_t; // 8字节
*/


#pragma pack(push, 1)  // 更改对齐方式， 让下面的CPacket类将按照一个字节的边界对齐；

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

	CPacket(WORD nCmd, const BYTE* pData, size_t nSize)  // 打包,这里传入的nSize是data的长度
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

	CPacket(const BYTE* pData, size_t& nSize)  // 数据包解包，将包中的数据分配到成员变量中
	{
		size_t i = 0;
		for (; i < nSize; i++) {
			if (*(WORD*)(pData+i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2; // 2字节
				break;
			}
		}
		// 4字节nLength; 2字节sCmd；2字节sSum;
		if (i + 4 + 2 + 2 > nSize) { // 包数据不全；或者包头未能全部接收到
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i);
		i += 4;
		if (nLength + i > nSize) {  // 包没有完全接收到， 就返回，解析失败
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i);
		i += 2;
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2); // 注：nLength包括：控制命令、包数据、和校验的长度
			memcpy((void*)strData.c_str(), (pData + i), (nLength - 2 - 2));  // 读取数据
			i += (nLength - 4);  // 自己写成了-2,没有debug出来  :( 
		}
		sSum = *(WORD*)(pData + i);
		i += 2;

		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			// 将 strData[j] 转换为 BYTE 类型并进行按位与操作，确保只取字符的低8位,并清除任何不必要的高位影响。
			sum += BYTE(strData[j]) & 0xFF;   //啥问题？ 
		}
		if (sum == sSum) {  // 校验
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
	int Size() {  // 获得整个包的大小
		return nLength + 4 + 2;
	}
	const char* Data() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();  // 声明一个指针，指向strOut；
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();  // 最后返回strOut的指针
	}

public:
	WORD        sHead;   // 包头 固定位FEEF                       2字节
	DWORD       nLength; // 包长 (从控制命令开始，到和校验结束)   4字节
	WORD        sCmd;    // 控制命令 (考虑对齐)                   2字节
	std::string strData; // 包数据                             
	WORD        sSum;    // 和校验                                2字节
	std::string strOut;  // 整个包的数据
};
#pragma pack(pop)


typedef struct file_info {
	file_info()  // 结构体里面也可以构造函数
	{
		IsInvaild = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvaild;       // 是否无效    
	BOOL IsDirectory;     // 是否为目录  0否，1是
	BOOL HasNext;         // 是否还有后续 
	char szFileName[256]; // 文件名
}FILEINFO, * PFILEINFO;

typedef struct MouseEvent{
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction; // 0表示单击，1表示双击，2表示按下，3表示放开，4不作处理
	WORD nButton; // 0表示左键，1表示右键，2表示中键，3没有按键
	POINT ptXY;   // 坐标
}MOUSEEV,*PMOUSEEV;


class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		// 静态成员函数没有this指针，只能方位静态成员变量；m_instance为静态成员变量
		if (m_instance == NULL)  // 单例；此时是线程安全的，不需要mutex；因为是在main之前执行的。
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
		// 绑定
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
			TRACE("内存不足\r\n");
		}
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true) {
			size_t len = recv(m_clntSock, buffer+index, BUFFER_SIZE-index, 0);  // 缓冲区的处理！！！！！！
			if (len <= 0) {
				delete[] buffer;
				return -1;
			}
			index += len;  // !!!
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);  // 这里第二个参数len传入的引用；会改变其值;(所以后面的len的值不一定等于之前的值)
			if (len > 0)   // 这里的len表示的是：用到的一整块数据包的长度
			{
				index -= len; 
				memmove(buffer, buffer + len, BUFFER_SIZE - len);   // 缓冲区的处理！！！！！！
				delete[] buffer;
				return m_packet.sCmd;  // 并返回一个操作指令
			}
			// 如果len==0 表示缓冲区还没有一整块数据包，就让继续while循环
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
	bool Send(CPacket& packet) // 这里不能使用const了，因为packet.Data()会改变成员值
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
	bool GetMouseEvent(MOUSEEV& mouse)  // 鼠标：移动，单击，右击，双击；需要一个结构体；
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
			MessageBox(NULL, _T("无法初始化套接字环境"), _T("初始化错误"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_servSock = socket(PF_INET, SOCK_STREAM, 0);
	}
	CServerSocket(const CServerSocket&) {}              // 禁止拷贝构造
	CServerSocket& operator=(const CServerSocket&) {}   // 禁止拷贝赋值
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
