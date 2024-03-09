﻿// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>  // _chdrive();

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
// 可以通过代码设置入口
// #pragma comment(linker, "/subsystem:windows /entry:WinMainCRTStartup")  // 窗口   启动后会调用WinMain
// #pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")     // 窗口   启动后会调用main（就没有窗口）
// #pragma comment(linker, "/subsystem:console /entry:mainCRTStartup")     // 控制台   
// #pragma comment(linker, "/subsystem:console /entry:WinMainCRTStartup")  // 控制台  （这里启动后又会有窗口）


// git提交测试
// git branch -main

// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

void Dump(BYTE* pData, size_t nSize) {
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


int MakeDriverInfo()  // 1-->A盘； 2->B盘；3->C盘；...26->Z盘；
{
    TRACE("%s(%d): Enter MakeDriverInfo() \r\n", __FILE__,__LINE__);
    std::string result;
    for (int i = 1; i <= 26; i++) {
        if (_chdrive(i) == 0)  // _chdrive(),更改当前的驱动器号,成功返回0
        {  
            if (result.size() > 0) 
                result += ',';
            result += ('A' + i - 1);
        }
    }
    CPacket pack(1, (BYTE*)result.c_str(), result.size());  // 实现这个重载；打包用的
    // Dump((BYTE*)&pack, pack.Size());  // (BYTE*)&pack这种方式是错误的；
    Dump((BYTE*)pack.Data(), pack.Size());  
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

#include <io.h>
#include <list>
typedef struct file_info{
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
}FILEINFO,*PFILEINFO;

int MakeDirectoryInfo() {
    std::string strPath;
    // std::list<FILEINFO> lstFileInfos;
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false)  // 解析后，如果sCmd==2才对，此时的strPath是文件夹路径
    {
        OutputDebugString(_T("当前的命令不是获取文件列表，命令解析错误！"));
        return -1;
    }
    if (_chdir(strPath.c_str()) != 0)  // 切换到指定路径
    {
        FILEINFO finfo;
        finfo.IsInvaild = TRUE;
        finfo.IsDirectory = TRUE;
        finfo.HasNext = FALSE;
        memcpy(finfo.szFileName, strPath.c_str(), strPath.size());
        // lstFileInfos.push_back(finfo);
        CPacket pack(2, (BYTE*) & finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        OutputDebugString(_T("没有权限访问目录！"));
        return -2;
    }
    _finddata_t fdata;
    int hfind = 0;
    if ((hfind = _findfirst("*", &fdata)) == -1) {
		OutputDebugString(_T("没有找到任何文件/失败！"));
		return -3;
    }
    do{
        FILEINFO finfo; // 默认构造函数的HasNext=TRUE; IsInvalid=FALSE;
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
        // lstFileInfos.push_back(finfo);
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstance()->Send(pack);  // 发送消息到控制端
    } while (!_findnext(hfind, &fdata));
    FILEINFO finfo;
    finfo.HasNext = FALSE;
	CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
	CServerSocket::getInstance()->Send(pack);
    return 0;
}

int RunFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    // ShellExecuteA()这个就相当于双击这个文件，如果是.exe就运行，文件就打开....
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL); // UNICODE嘛？感觉应该是ShellExecuteW
	CPacket pack(3, NULL, 0);
	CServerSocket::getInstance()->Send(pack);
    return 0;
}

#pragma warning(disable : 4996)  // 包括一些：fopen;  sprintf; strcpy; strstr
int DownloadFile() {
	std::string strPath;
    long long data = 0;  // 8字节;  对面先接收8字节的data，如果data是0就表示失败；不为0表示是文件的大小。
	CServerSocket::getInstance()->GetFilePath(strPath);
    FILE* pFile = fopen(strPath.c_str(), "rb"); // 因不知道文件是什么，要以二进制
    if (pFile == NULL) {
        // CPacket pack(4, NULL, 0);
        CPacket head(4, (BYTE*)&data, 8);  // head
        CServerSocket::getInstance()->Send(head);
        return -1;
    }
    fseek(pFile, 0, SEEK_END);
    // data = ftell(pFile);
    data = _ftelli64(pFile);
    fseek(pFile, 0, SEEK_SET);  // 记得要fseek回来
    CPacket head(4, (BYTE*)&data, 8);  // head
    CServerSocket::getInstance()->Send(head);

    char buffer[1024] = ""; // 建议不要太大，1K就好
    size_t rlen = 0;
    do {
        rlen = fread(buffer, 1, 1024, pFile);
		CPacket pack(4, (BYTE*)buffer, rlen);
		CServerSocket::getInstance()->Send(pack);
    } while (rlen>=1024);
	CPacket end(4, NULL, 0);
	CServerSocket::getInstance()->Send(end);
    fclose(pFile); // 记得关闭
    return 0;
}

int MouseEvent() {
    MOUSEEV mouse;
    if (CServerSocket::getInstance()->GetMouseEvent(mouse) == true) {
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
		default:
			break;
        }
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
        CPacket pack(4, NULL, 0);
        CServerSocket::getInstance()->Send(pack);  // 最后都要发送回一个包，表示已经接收到这个命令
    }
    else {
        OutputDebugString(_T("获取鼠标操作参数失败！"));
        return -1;
    }
    return 0;
}

#include <atlimage.h>
int SendScreen() {
    CImage screen;  // GDI
    // DC:设备上下文
    HDC hScreen = ::GetDC(NULL);  // 获取整个屏幕的设备上下文句柄（HDC）
    // GetDeviceCaps()函数获取屏幕设备上下文的属性;
    int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);  // BITSPIXEL参数表示每个像素的位数
    int nWidth = GetDeviceCaps(hScreen, HORZRES);          // HORZRES参数表示屏幕水平分辨率，即屏幕的宽度
	int nHeight = GetDeviceCaps(hScreen, VERTRES);       
	screen.Create(nWidth, nHeight, nBitPerPixel); // 使用CImage对象的Create方法创建一个位图
    BitBlt(screen.GetDC(), 0, 0, 2560, 1440, hScreen, 0, 0, SRCCOPY);  // 使用BitBlt函数将屏幕内容复制到创建的位图中
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
        CPacket pack(6, pData, nSize);
        CServerSocket::getInstance()->Send(pack);
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


#include "LockinfoDialog.h"
CLockinfoDialog dlg;  // 非模态，要声明为全局变量
unsigned int threadid = 0;
// 不能在主进程里面进行while()死循环，将无法收到Unlock的命令
// unsigned __stdcall threadLockDlg(void* arg) 
unsigned threadLockDlg(void* arg)
{
    TRACE("%s(%d):%d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
	dlg.Create(IDD_DIALOG_INFO, NULL);  // 在这里create比较好
	dlg.ShowWindow(SW_SHOW);
	// 遮蔽后台窗口
	CRect rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
	rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
	rect.bottom *= 1.03;
	// TRACE("right:%d, bottom:%d\r\n", rect.right, rect.bottom);
	dlg.MoveWindow(&rect);

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
	ShowCursor(true);
	::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW); // 显示系统任务栏
    dlg.DestroyWindow();
    _endthreadex(0);
    return 0;
}

int LockMachine()
{
    // 为了避免一直要求锁机，避免一直创建线程
    if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {
        // _beginthread(threadLockDlg, 0, NULL);
        _beginthreadex(NULL, 0, threadLockDlg, NULL, 0, &threadid);
        TRACE("threadid=%d\r\n", threadid);
    }
	CPacket pack(7, NULL, 0);
	CServerSocket::getInstance()->Send(pack);
    return 0;
}

int UnlockMachine() 
{
    // dlg.SendMessage(WM_KEYDOWN, 0x41, 0x01E0001); // 发送消息：按下a键
    // ::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x41, 0x01E0001);  // 这里发送的消息，另外的线程是无法接收到的；
    PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0);  // 只能通过这种方式才能收到

    CPacket pack(8, NULL, 0);
	CServerSocket::getInstance()->Send(pack);
    return 0;
}

int TextConnect() {
	CPacket pack(1981, NULL, 0);
	CServerSocket::getInstance()->Send(pack);
	return 0;
}


int ExcuteCommand(int nCmd)
{
    int ret = 0;
	switch (nCmd) {
	case 1: // 查看磁盘分区
		ret = MakeDriverInfo();
		break;
	case 2: // 查看指定目录下的文件
        ret = MakeDirectoryInfo();
		break;
	case 3:  // 运行文件
        ret = RunFile();
		break;
	case 4:  // 下载文件
        ret = DownloadFile();
		break;
	case 5:  // 鼠标操作
        ret = MouseEvent();
		break;
	case 6:  // 发送屏幕内容 ---> 发送屏幕截图
        ret = SendScreen();
		break;
	case 7:  // 锁机
        ret = LockMachine();  // dlg是全局的
		// Sleep(50);
		// LockMachine();  // 加了判断，不会重复创建线程
		break;
	case 8:  // 解锁
        ret = UnlockMachine();
		break;
    case 1981:
        ret = TextConnect();
        break;
	}
    return ret;
}


int main()
{
    int nRetCode = 0;
    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            // TODO: socket、bind、listen、accept、read、write
            int count = 0;
            CServerSocket* pServer = CServerSocket::getInstance();  // 单例
			if (pServer->InitSocket() == false) {
				MessageBox(NULL, _T("网络初始化异常，请检查网络状态"), _T("网络初始化failed"), MB_OK | MB_ICONERROR);
				exit(0);
			}
            while(pServer!=NULL){    
                if (pServer->AcceptClient() == false) {
                    pServer = CServerSocket::getInstance();
                    if (count >= 3) {
						MessageBox(NULL, _T("多次无法正常接入用户，结束程序"), _T("接入用户failed"), MB_OK | MB_ICONERROR);
						exit(0);
                    }
					MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户failed"), MB_OK | MB_ICONERROR);
                    count++;
                }
                int ret = pServer->DealCommand(); // 如果正确的话返回的是一个操作指令；
                if (ret != -1)   // TODO  
                {
                    ret = ExcuteCommand(ret);
                    if (ret != 0) // 在处理命令的函数中，正确就返回0
                    {
                        TRACE("执行命令失败：%d, ret=%d\r\n", pServer->GetPacket().sCmd, ret);
                    }
                    // 采用短连接（如果采用长连接就不需要close）
                    pServer->CloseClient();   
                    TRACE("Command has done!\r\n");
                }
            }     
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
