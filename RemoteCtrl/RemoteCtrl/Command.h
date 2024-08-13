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
#pragma warning(disable : 4996)  // 包括一些：fopen;  sprintf; strcpy; strstr

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
               TRACE("执行命令失败：%d, ret=%d\r\n",status, ret);
            }
        }
        else {
			MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户failed"), MB_OK | MB_ICONERROR);
        }           
    }
protected:
	typedef int(CCommand::* CMDFUNC)(std::list<CPacket>&, CPacket&);   // 成员函数指针
	std::map<int, CMDFUNC> m_mapFunction; // 从命令号到功能的映射
	CLockinfoDialog dlg;  // 非模态，要声明为全局变量
	unsigned int threadid;
protected:
    int MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket)  // 1-->A盘； 2->B盘；3->C盘；...26->Z盘；
    {
        TRACE("%s(%d): Enter MakeDriverInfo() \r\n", __FILE__, __LINE__);
        std::string result;
        for (int i = 1; i <= 26; i++) {
            if (_chdrive(i) == 0)  // _chdrive(),更改当前的驱动器号,成功返回0
            {
                if (result.size() > 0)
                    result += ',';
                result += ('A' + i - 1);
            }
        }
        lstPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));

        //CPacket pack(1, (BYTE*)result.c_str(), result.size());  // 实现这个重载；打包用的
        //CTool::Dump((BYTE*)pack.Data(), pack.Size());
        //CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) {
        std::string strPath=inPacket.strData;
        // CServerSocket::getInstance()->GetFilePath(strPath);  // 解耦
        if (_chdir(strPath.c_str()) != 0)  // 切换到指定路径
        {
            FILEINFO finfo;
            finfo.HasNext = FALSE; // 直接把HasNext设为FALSE，并返回
            lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
            OutputDebugString(_T("没有权限访问目录！"));
            return -2;
        }
        _finddata_t fdata;
        int hfind = 0;
        if ((hfind = _findfirst("*", &fdata)) == -1) {
            FILEINFO finfo;
            finfo.HasNext = FALSE;
            lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
            OutputDebugString(_T("没有找到任何文件/失败！"));
            return -3;
        }
        int Count = 0;
        do {
            FILEINFO finfo; // 默认构造函数的HasNext=TRUE; IsInvalid=FALSE;
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
        // ShellExecuteA()这个就相当于双击这个文件，如果是.exe就运行，文件就打开....
        ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL); // UNICODE嘛？感觉应该是ShellExecuteW
        lstPacket.push_back(CPacket(3, NULL, 0));
        return 0;
    }
	
    int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
        std::string strPath=inPacket.strData;
        long long data = 0;  // 8字节;  对面先接收8字节的data，如果data是0就表示失败；不为0表示是文件的大小。
        FILE* pFile = fopen(strPath.c_str(), "rb"); // 因不知道文件是什么，要以二进制
        if (pFile == NULL) {
            lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
            return -1;
        }
        fseek(pFile, 0, SEEK_END);
        // data = ftell(pFile);
        data = _ftelli64(pFile);
        lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
        fseek(pFile, 0, SEEK_SET);  // 记得要fseek回来

        char buffer[1024] = ""; // 建议不要太大，1K就好
        size_t rlen = 0;
        do {
            rlen = fread(buffer, 1, 1024, pFile);
            lstPacket.push_back(CPacket(4, (BYTE*)buffer, rlen));  // 一开始写成了：lstPacket.push_back(CPacket(4, (BYTE*)&data, rlen));
        } while (rlen >= 1024);
        lstPacket.push_back(CPacket(4, NULL, 0));
        fclose(pFile); // 记得关闭
        return 0;
    }

    int MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket) {
        MOUSEEV mouse;
        memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV));
        DWORD nFlags;  // 使用标志位；避免在switch中嵌套switch或者if-else
        switch (mouse.nButton) {
        case 0:  // 左键
            nFlags = 1;
            break;
        case 1:  // 右键
            nFlags = 2;
            break;
        case 2:  // 中键
            nFlags = 4;
            break;
        case 3:  // 没有按键(鼠标移动)，也就没有对应的mouse.nAction
            nFlags = 8;
            break;
        default:
            nFlags = 8;
            break;
        }
        if (nFlags != 8) SetCursorPos(mouse.ptXY.x, mouse.ptXY.y); // SetCursorPos()用于设置鼠标的位置
        switch (mouse.nAction) {
        case 0:  // 单击
            nFlags |= 0x10;
            break;
        case 1:  // 双击
            nFlags |= 0x20;
            break;
        case 2:  // 按下
            nFlags |= 0x40;
            break;
        case 3:  // 放开
            nFlags |= 0x80;
            break;
        case 4:  // 鼠标移动
            break;
        default:
            break;
        }
        TRACE("nFlages:%08X x:%d, y:%d\r\n", nFlags, mouse.ptXY.x, mouse.ptXY.y);
        switch (nFlags)
        {
        case 0x21:  // 左键双击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());  // 这种写法，case 0x21:后面没有break，如果执行到，会把后续的也执行了；
        case 0x11:  // 左键单击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x41:  // 左键按下
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x81:  // 左键放开
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x22:  // 右键双击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x12:  // 右键单击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x42:  // 右键按下
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x82:  // 右键放开
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x24:  // 中键双击  MOUSEEVENTF_MIDDLEDOWN
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x14:  // 中键单击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x44:  // 中键按下
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x84:  // 中键放开
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x08:  // 单纯的鼠标移动
            mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
            break;
        }
        lstPacket.push_back(CPacket(5, NULL, 0));
        // 最后都要发送回一个包，表示已经接收到这个命令
        return 0;
    }


    int SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket) {
        CImage screen;  // GDI：图形设备接口，使用第一代桌面采集技术；
        // DC:设备上下文
        HDC hScreen = ::GetDC(NULL);  // 获取整个屏幕的设备上下文句柄（HDC）
        // GetDeviceCaps()函数获取屏幕设备上下文的属性;
        int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);  // BITSPIXEL参数表示每个像素的位数
        int nWidth = GetDeviceCaps(hScreen, HORZRES);          // HORZRES参数表示屏幕水平分辨率，即屏幕的宽度
        int nHeight = GetDeviceCaps(hScreen, VERTRES);
        screen.Create(nWidth, nHeight, nBitPerPixel); // 使用CImage对象的Create方法创建一个位图
        BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);  // 使用BitBlt函数将屏幕内容复制到创建的位图中
        // 2560×1440   1920×1080
        ReleaseDC(NULL, hScreen);

        // 保存到内存中去
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
        DWORD tick = GetTickCount64();  // GetTickCount()获取当前时间(毫秒级)
        screen.Save(_T("test.png"), Gdiplus::ImageFormatPNG);
        TRACE("png: %d\r\n", GetTickCount64() - tick);
        tick = GetTickCount64();
        screen.Save(_T("test.jpg"), Gdiplus::ImageFormatJPEG);
        TRACE("jpg: %d\r\n", GetTickCount64() - tick);
        */
        // 倾向选择jpg;  【但这里是保存到文件中了】
        // screen.Save(_T("test.jpg"), Gdiplus::ImageFormatJPEG);  // 对比：两种图片的大小(所消耗网络带宽)、压缩时间(消耗CPU)
        screen.ReleaseDC();
        return 0;
    }

	// 不能在主进程里面进行while()死循环，将无法收到Unlock的命令
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
		dlg.Create(IDD_DIALOG_INFO, NULL);  // 在这里create比较好
		dlg.ShowWindow(SW_SHOW);
		// 遮蔽后台窗口
		CRect rect;
		rect.left = 0;
		rect.top = 0;
		rect.right = GetSystemMetrics(SM_CXFULLSCREEN);  // GetSystemMetrics() 用于获取系统的各种度量信息
		rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
		rect.bottom *= (LONG)1.03;
		// TRACE("right:%d, bottom:%d\r\n", rect.right, rect.bottom);
		dlg.MoveWindow(&rect);  //MoveWindow() 用于移动窗口, 参数指定窗口的新位置和大小

		// 让IDC_STATIC居中显示
		CWnd* pText = dlg.GetDlgItem(IDC_STATIC);  // GetDlgItem() 用于获取对话框中具有指定标识符的控件的指针
		if (pText) {
			CRect rtText;
			pText->GetWindowRect(rtText);
			int nWidth = rtText.Width();
			int nHeight = rtText.Height();
			int x = (rect.right - nWidth) / 2;
			int y = (rect.bottom - nHeight) / 2;
			pText->MoveWindow(x, y, nWidth, nHeight);
		}

		// 窗口置顶
		dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE); // 不要改变大小|不要移动
		// 限制鼠标功能
		ShowCursor(false); // 鼠标在对话框内，就不显示
		// 限制鼠标活动范围
		rect.right = rect.left + 1;
		rect.bottom = rect.top + 1;
		ClipCursor(rect);
		// 隐藏系统任务栏
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);

		// 对话框依赖与消息循环(让这个对话框一直显示)
		MSG msg;  // 定义了一个消息结构体msg，用于接收消息
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);  // 消息转换
			DispatchMessage(&msg);   // 消息分派
			if (msg.message == WM_KEYDOWN)  // 键盘按下
			{
				// TRACE("msg:%08X wparam:%08X lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);
				if (msg.wParam == 0x41) { // 如果按下a键0x41  (Esc键0x1B)
					// TRACE("************Press-A(%d),%s***************\r\n", __LINE__, __FUNCTION__);
					break;
				}
			}
		}
		// 恢复鼠标
		ShowCursor(true);
		ClipCursor(NULL);
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW); // 显示系统任务栏
		dlg.DestroyWindow();
    }

    int LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
    {
        // 为了避免一直要求锁机，避免一直创建线程
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
        // dlg.SendMessage(WM_KEYDOWN, 0x41, 0x01E0001); // 发送消息：按下a键
        // ::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x41, 0x01E0001);  // 这里发送的消息，另外的线程是无法接收到的；
        PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0);  // 只能通过这种方式才能收到

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
        // wcstombs();// wcs宽字节字符集    mbs多字节字符集
        // mbstowcs();
        /*
        // 多字节字符集 转 宽字节字符集
        TCHAR sPath[MAX_PATH] = _T("");
        mbstowcs(sPath, strPath.c_str(), strPath.size());
        */
        DeleteFile((LPCTSTR)strPath.c_str());
        TRACE("%s(%d)%s: [%s]\r\n", __FILE__, __LINE__, __FUNCTION__, (LPCTSTR)strPath.c_str());
        lstPacket.push_back(CPacket(9, NULL, 0));
        return 0;
    }
};

