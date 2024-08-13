#pragma once
#include <map>
#include "Packet.h"
#include <direct.h>  // _chdrive();
#include <atlimage.h>
#include <stdio.h>
#include <io.h>
#include <list>
#include "Resource.h"
#include "Tool.h"
#include "LockinfoDialog.h"
#pragma warning(disable : 4996)  // ����һЩ��fopen;  sprintf; strcpy; strstr

class CCommand
{
public:
	CCommand();
	~CCommand() {}
	int ExcuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket);
    static void RunCommand(void* arg, int status, std::list<CPacket>&lstPacket, CPacket& inPacket) {
        CCommand* thiz = (CCommand*)arg;
        if (status > 0) {
            int ret = thiz->ExcuteCommand(status, lstPacket, inPacket);
            if (ret != 0) {
               TRACE("ִ������ʧ�ܣ�%d, ret=%d\r\n",status, ret);
            }
        }
        else {
			MessageBox(NULL, _T("�޷����������û����Զ�����"), _T("�����û�failed"), MB_OK | MB_ICONERROR);
        }           
    }
protected:
	typedef int(CCommand::* CMDFUNC)(std::list<CPacket>&, CPacket&);   // ��Ա����ָ��
	std::map<int, CMDFUNC> m_mapFunction; // ������ŵ����ܵ�ӳ��
	CLockinfoDialog dlg;  // ��ģ̬��Ҫ����Ϊȫ�ֱ���
	unsigned int threadid;
protected:
    int MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket)  // 1-->A�̣� 2->B�̣�3->C�̣�...26->Z�̣�
    {
        TRACE("%s(%d): Enter MakeDriverInfo() \r\n", __FILE__, __LINE__);
        std::string result;
        for (int i = 1; i <= 26; i++) {
            if (_chdrive(i) == 0)  // _chdrive(),���ĵ�ǰ����������,�ɹ�����0
            {
                if (result.size() > 0)
                    result += ',';
                result += ('A' + i - 1);
            }
        }
        lstPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));

        //CPacket pack(1, (BYTE*)result.c_str(), result.size());  // ʵ��������أ�����õ�
        //CTool::Dump((BYTE*)pack.Data(), pack.Size());
        //CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) {
        std::string strPath=inPacket.strData;
        // CServerSocket::getInstance()->GetFilePath(strPath);  // ����
        if (_chdir(strPath.c_str()) != 0)  // �л���ָ��·��
        {
            FILEINFO finfo;
            finfo.HasNext = FALSE; // ֱ�Ӱ�HasNext��ΪFALSE��������
            lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
            OutputDebugString(_T("û��Ȩ�޷���Ŀ¼��"));
            return -2;
        }
        _finddata_t fdata;
        int hfind = 0;
        if ((hfind = _findfirst("*", &fdata)) == -1) {
            FILEINFO finfo;
            finfo.HasNext = FALSE;
            lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
            OutputDebugString(_T("û���ҵ��κ��ļ�/ʧ�ܣ�"));
            return -3;
        }
        int Count = 0;
        do {
            FILEINFO finfo; // Ĭ�Ϲ��캯����HasNext=TRUE; IsInvalid=FALSE;
            finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
            memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
            TRACE("%s\r\n", finfo.szFileName);
            lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
            Count++;
        } while (!_findnext(hfind, &fdata));
        TRACE(" Server Count: %d\r\n", ++Count);
        FILEINFO finfo;
        finfo.HasNext = FALSE;
        lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
        _findclose(hfind);  // IADD
        return 0;
    }

    int RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
        std::string strPath=inPacket.strData;
        // ShellExecuteA()������൱��˫������ļ��������.exe�����У��ļ��ʹ�....
        ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL); // UNICODE��о�Ӧ����ShellExecuteW
        lstPacket.push_back(CPacket(3, NULL, 0));
        return 0;
    }
	
    int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
        std::string strPath=inPacket.strData;
        long long data = 0;  // 8�ֽ�;  �����Ƚ���8�ֽڵ�data�����data��0�ͱ�ʾʧ�ܣ���Ϊ0��ʾ���ļ��Ĵ�С��
        FILE* pFile = fopen(strPath.c_str(), "rb"); // ��֪���ļ���ʲô��Ҫ�Զ�����
        if (pFile == NULL) {
            lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
            return -1;
        }
        fseek(pFile, 0, SEEK_END);
        // data = ftell(pFile);
        data = _ftelli64(pFile);
        lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
        fseek(pFile, 0, SEEK_SET);  // �ǵ�Ҫfseek����

        char buffer[1024] = ""; // ���鲻Ҫ̫��1K�ͺ�
        size_t rlen = 0;
        do {
            rlen = fread(buffer, 1, 1024, pFile);
            lstPacket.push_back(CPacket(4, (BYTE*)buffer, rlen));  // һ��ʼд���ˣ�lstPacket.push_back(CPacket(4, (BYTE*)&data, rlen));
        } while (rlen >= 1024);
        lstPacket.push_back(CPacket(4, NULL, 0));
        fclose(pFile); // �ǵùر�
        return 0;
    }

    int MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket) {
        MOUSEEV mouse;
        memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV));
        DWORD nFlags;  // ʹ�ñ�־λ��������switch��Ƕ��switch����if-else
        switch (mouse.nButton) {
        case 0:  // ���
            nFlags = 1;
            break;
        case 1:  // �Ҽ�
            nFlags = 2;
            break;
        case 2:  // �м�
            nFlags = 4;
            break;
        case 3:  // û�а���(����ƶ�)��Ҳ��û�ж�Ӧ��mouse.nAction
            nFlags = 8;
            break;
        default:
            nFlags = 8;
            break;
        }
        if (nFlags != 8) SetCursorPos(mouse.ptXY.x, mouse.ptXY.y); // SetCursorPos()������������λ��
        switch (mouse.nAction) {
        case 0:  // ����
            nFlags |= 0x10;
            break;
        case 1:  // ˫��
            nFlags |= 0x20;
            break;
        case 2:  // ����
            nFlags |= 0x40;
            break;
        case 3:  // �ſ�
            nFlags |= 0x80;
            break;
        case 4:  // ����ƶ�
            break;
        default:
            break;
        }
        TRACE("nFlages:%08X x:%d, y:%d\r\n", nFlags, mouse.ptXY.x, mouse.ptXY.y);
        switch (nFlags)
        {
        case 0x21:  // ���˫��
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());  // ����д����case 0x21:����û��break�����ִ�е�����Ѻ�����Ҳִ���ˣ�
        case 0x11:  // �������
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x41:  // �������
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x81:  // ����ſ�
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x22:  // �Ҽ�˫��
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x12:  // �Ҽ�����
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x42:  // �Ҽ�����
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x82:  // �Ҽ��ſ�
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x24:  // �м�˫��  MOUSEEVENTF_MIDDLEDOWN
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x14:  // �м�����
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x44:  // �м�����
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x84:  // �м��ſ�
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x08:  // ����������ƶ�
            mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
            break;
        }
        lstPacket.push_back(CPacket(5, NULL, 0));
        // ���Ҫ���ͻ�һ��������ʾ�Ѿ����յ��������
        return 0;
    }


    int SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket) {
        CImage screen;  // GDI��ͼ���豸�ӿڣ�ʹ�õ�һ������ɼ�������
        // DC:�豸������
        HDC hScreen = ::GetDC(NULL);  // ��ȡ������Ļ���豸�����ľ����HDC��
        // GetDeviceCaps()������ȡ��Ļ�豸�����ĵ�����;
        int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);  // BITSPIXEL������ʾÿ�����ص�λ��
        int nWidth = GetDeviceCaps(hScreen, HORZRES);          // HORZRES������ʾ��Ļˮƽ�ֱ��ʣ�����Ļ�Ŀ��
        int nHeight = GetDeviceCaps(hScreen, VERTRES);
        screen.Create(nWidth, nHeight, nBitPerPixel); // ʹ��CImage�����Create��������һ��λͼ
        BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);  // ʹ��BitBlt��������Ļ���ݸ��Ƶ�������λͼ��
        // 2560��1440   1920��1080
        ReleaseDC(NULL, hScreen);

        // ���浽�ڴ���ȥ
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
        if (hMem == NULL) return -1;
        IStream* pStream = NULL;
        HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
        if (ret == S_OK) {
            screen.Save(pStream, Gdiplus::ImageFormatJPEG);
            LARGE_INTEGER bg = { 0 };
            pStream->Seek(bg, STREAM_SEEK_SET, NULL);
            PBYTE pData = (PBYTE)GlobalLock(hMem);
            SIZE_T nSize = GlobalSize(hMem);
            lstPacket.push_back(CPacket(6, pData, nSize));
            GlobalUnlock(hMem);
        }
        pStream->Release();
        GlobalFree(hMem);
        /*
        DWORD tick = GetTickCount64();  // GetTickCount()��ȡ��ǰʱ��(���뼶)
        screen.Save(_T("test.png"), Gdiplus::ImageFormatPNG);
        TRACE("png: %d\r\n", GetTickCount64() - tick);
        tick = GetTickCount64();
        screen.Save(_T("test.jpg"), Gdiplus::ImageFormatJPEG);
        TRACE("jpg: %d\r\n", GetTickCount64() - tick);
        */
        // ����ѡ��jpg;  ���������Ǳ��浽�ļ����ˡ�
        // screen.Save(_T("test.jpg"), Gdiplus::ImageFormatJPEG);  // �Աȣ�����ͼƬ�Ĵ�С(�������������)��ѹ��ʱ��(����CPU)
        screen.ReleaseDC();
        return 0;
    }

	// �������������������while()��ѭ�������޷��յ�Unlock������
	static unsigned __stdcall threadLockDlg(void* arg)
	{
        CCommand* thiz = (CCommand*)arg;
        thiz->threadLockDlgMain();
		_endthreadex(0);
		return 0;
	}

    void threadLockDlgMain()
    {
		TRACE("%s(%d):%d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
		dlg.Create(IDD_DIALOG_INFO, NULL);  // ������create�ȽϺ�
		dlg.ShowWindow(SW_SHOW);
		// �ڱκ�̨����
		CRect rect;
		rect.left = 0;
		rect.top = 0;
		rect.right = GetSystemMetrics(SM_CXFULLSCREEN);  // GetSystemMetrics() ���ڻ�ȡϵͳ�ĸ��ֶ�����Ϣ
		rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
		rect.bottom *= (LONG)1.03;
		// TRACE("right:%d, bottom:%d\r\n", rect.right, rect.bottom);
		dlg.MoveWindow(&rect);  //MoveWindow() �����ƶ�����, ����ָ�����ڵ���λ�úʹ�С

		// ��IDC_STATIC������ʾ
		CWnd* pText = dlg.GetDlgItem(IDC_STATIC);  // GetDlgItem() ���ڻ�ȡ�Ի����о���ָ����ʶ���Ŀؼ���ָ��
		if (pText) {
			CRect rtText;
			pText->GetWindowRect(rtText);
			int nWidth = rtText.Width();
			int nHeight = rtText.Height();
			int x = (rect.right - nWidth) / 2;
			int y = (rect.bottom - nHeight) / 2;
			pText->MoveWindow(x, y, nWidth, nHeight);
		}

		// �����ö�
		dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE); // ��Ҫ�ı��С|��Ҫ�ƶ�
		// ������깦��
		ShowCursor(false); // ����ڶԻ����ڣ��Ͳ���ʾ
		// ���������Χ
		rect.right = rect.left + 1;
		rect.bottom = rect.top + 1;
		ClipCursor(rect);
		// ����ϵͳ������
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);

		// �Ի�����������Ϣѭ��(������Ի���һֱ��ʾ)
		MSG msg;  // ������һ����Ϣ�ṹ��msg�����ڽ�����Ϣ
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);  // ��Ϣת��
			DispatchMessage(&msg);   // ��Ϣ����
			if (msg.message == WM_KEYDOWN)  // ���̰���
			{
				// TRACE("msg:%08X wparam:%08X lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);
				if (msg.wParam == 0x41) { // �������a��0x41  (Esc��0x1B)
					// TRACE("************Press-A(%d),%s***************\r\n", __LINE__, __FUNCTION__);
					break;
				}
			}
		}
		// �ָ����
		ShowCursor(true);
		ClipCursor(NULL);
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW); // ��ʾϵͳ������
		dlg.DestroyWindow();
    }

    int LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
    {
        // Ϊ�˱���һֱҪ������������һֱ�����߳�
        if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {
            // _beginthread(threadLockDlg, 0, NULL);
            _beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid);  
            TRACE("threadid=%d\r\n", threadid);
        }
        lstPacket.push_back(CPacket(7, NULL, 0));
        return 0;
    }

    int UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
    {
        // dlg.SendMessage(WM_KEYDOWN, 0x41, 0x01E0001); // ������Ϣ������a��
        // ::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x41, 0x01E0001);  // ���﷢�͵���Ϣ��������߳����޷����յ��ģ�
        PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0);  // ֻ��ͨ�����ַ�ʽ�����յ�

        lstPacket.push_back(CPacket(8, NULL, 0));
        return 0;
    }

    int TextConnect(std::list<CPacket>& lstPacket, CPacket& inPacket) {
        lstPacket.push_back(CPacket(1981, NULL, 0));
        return 0;
    }

    int DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
    {
        //TODO:
        std::string strPath=inPacket.strData;
        // wcstombs();// wcs���ֽ��ַ���    mbs���ֽ��ַ���
        // mbstowcs();
        /*
        // ���ֽ��ַ��� ת ���ֽ��ַ���
        TCHAR sPath[MAX_PATH] = _T("");
        mbstowcs(sPath, strPath.c_str(), strPath.size());
        */
        DeleteFile((LPCTSTR)strPath.c_str());
        TRACE("%s(%d)%s: [%s]\r\n", __FILE__, __LINE__, __FUNCTION__, (LPCTSTR)strPath.c_str());
        lstPacket.push_back(CPacket(9, NULL, 0));
        return 0;
    }
};

