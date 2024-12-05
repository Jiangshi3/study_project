#pragma once
#include <Windows.h>
#include <atlimage.h>
#include <string>  // 是C++标准库中的头文件

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
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);  // 使用 GlobalAlloc() 分配一个可移动的全局内存块，并将句柄存储在 hMem 中
		if (hMem == NULL) {
			TRACE(_T("内存不足！\r\n"));
			Sleep(1);  // 注意在死循环内部，最好要添加Sleep()防止拉满
			return -1;
		}
		IStream* pStream = NULL;
		HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);  // 调用 CreateStreamOnHGlobal() 函数，将全局内存块转换为一个流对象，并将结果存储在 pStream 中
		if (hRet == S_OK) {
			ULONG length = 0;
			pStream->Write(pData, strBuffer.size(), &length);  // 将字节数据写入流中
			LARGE_INTEGER bg = { 0 };  // 用于在流中设置指针位置
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);  // 将流的指针位置设置为起始位置，以便后续读取操作。
			if ((HBITMAP)image != NULL) image.Destroy();
			image.Load(pStream);  // 将图像从数据流中解析并加载到 m_image 对象中
		}
		return hRet;
	}
};

