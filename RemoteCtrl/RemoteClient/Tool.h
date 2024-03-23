#pragma once
#include <Windows.h>
#include <atlimage.h>
#include <string>  // ��C++��׼���е�ͷ�ļ�

class CTool
{
public:
	static void Dump(BYTE* pData, size_t nSize) {
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

	static int Bytes2Image(CImage& image, const std::string& strBuffer) {
		BYTE* pData = (BYTE*)strBuffer.c_str();
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);  // ʹ�� GlobalAlloc() ����һ�����ƶ���ȫ���ڴ�飬��������洢�� hMem ��
		if (hMem == NULL) {
			TRACE(_T("�ڴ治�㣡\r\n"));
			Sleep(1);  // ע������ѭ���ڲ������Ҫ���Sleep()��ֹ����
			return -1;
		}
		IStream* pStream = NULL;
		HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);  // ���� CreateStreamOnHGlobal() ��������ȫ���ڴ��ת��Ϊһ�������󣬲�������洢�� pStream ��
		if (hRet == S_OK) {
			ULONG length = 0;
			pStream->Write(pData, strBuffer.size(), &length);  // ���ֽ�����д������
			LARGE_INTEGER bg = { 0 };  // ��������������ָ��λ��
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);  // ������ָ��λ������Ϊ��ʼλ�ã��Ա������ȡ������
			if ((HBITMAP)image != NULL) image.Destroy();
			image.Load(pStream);  // ��ͼ����������н��������ص� m_image ������
		}
		return hRet;
	}
};

