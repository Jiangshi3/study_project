#pragma once
#include "pch.h"
#include "framework.h"
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
			if (*(WORD*)(pData + i) == 0xFEFF) {
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
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;       // 是否无效    
	BOOL IsDirectory;     // 是否为目录  0否，1是
	BOOL HasNext;         // 是否还有后续 
	char szFileName[256]; // 文件名
}FILEINFO, * PFILEINFO;

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction; // 0表示单击，1表示双击，2表示按下，3表示放开，4不作处理
	WORD nButton; // 0表示左键，1表示右键，2表示中键，3没有按键
	POINT ptXY;   // 坐标
}MOUSEEV, * PMOUSEEV;

