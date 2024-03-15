#pragma once
#include "pch.h"
#include "framework.h"
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

