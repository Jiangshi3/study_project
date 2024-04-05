// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"
#include <conio.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
// 可以通过代码设置入口
// #pragma comment(linker, "/subsystem:windows /entry:WinMainCRTStartup")  // 窗口   启动后会调用WinMain
// #pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")     // 窗口   启动后会调用main（就没有窗口）
// #pragma comment(linker, "/subsystem:console /entry:mainCRTStartup")     // 控制台   
// #pragma comment(linker, "/subsystem:console /entry:WinMainCRTStartup")  // 控制台  （这里启动后又会有窗口）

// git branch -main
// 唯一的应用程序对象

// #define INVOKE_PATH _T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe")
#define INVOKE_PATH _T("C:\\Users\\J\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe")

CWinApp theApp;
using namespace std;


// 业务和通用
BOOL ChooseAutoInvoke(const CString& strPath) {
    // CString strPath =CString(_T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe"));  // 把本应该是system32修改为SysWOW64
    if (PathFileExists(strPath)) {
        return true;
    }
    CString strInfo = _T("该程序只允许用于合法的用途！\n");
    strInfo += _T("继续运行该程序，将使得这台机器处于被监控状态！\n");
    strInfo += _T("如果你不希望这样，请按“取消”按钮退出程序。\n");
    strInfo += _T("按下“是”按钮，该程序将被复制到你的机器上，并随系统启动而自动运行！\n");
    strInfo += _T("按下“否”按钮，程序只运行一次，不会在系统内留下任何东西！\n");
    int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES) {
        // CTool::WriteRegisterTable(strPath);
		if (!CTool::WriteStartupDir(strPath)) {
			MessageBox(NULL, _T("复制文件失败\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
    }
    else if (ret == IDCANCEL) {
		return false;
    }
    return true;
}

void func(void* arg) {
	std::string* pStr = (std::string*)arg;
	if (pStr != NULL) {
		printf("pop from list:%s\r\n", pStr->c_str());
		// delete pStr;
	}
	else {
		printf("list is empty,no data!\r\n");
	}
}

enum {
	IocpListEmpty,
	IocpListPush,
	IocpListPop
};

typedef struct IocpParam {
	IocpParam() {
		nOperator = -1;
	}
	IocpParam(int op, const char* pData, _beginthread_proc_type cb = NULL) {
		nOperator = op;
		strData = pData;
		cbFunc = cb;
	}
	int nOperator; // 操作
	std::string strData; // 数据
	_beginthread_proc_type cbFunc; // 回调函数
}IOCP_PARAM;


void threadmain(HANDLE hIOCP)
{
	std::list<std::string> lstString;
	DWORD dwTransferred = 0;
	ULONG_PTR CompletionKey = 0;
	OVERLAPPED* pOverlapped = NULL;
	int count = 0, count0 = 0, total = 0;
	while (GetQueuedCompletionStatus(hIOCP, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE)) {
		if ((dwTransferred == 0) || (CompletionKey == NULL)) {
			printf("thread is prepare to exit!\r\n");
			break;
		}
		IOCP_PARAM* pParam = (IOCP_PARAM*)CompletionKey;  // 这里就类似于以前的WPARAM
		if (pParam->nOperator == IocpListPush) {
			lstString.push_back(pParam->strData);
			count++;
		}
		else if (pParam->nOperator == IocpListPop) {
			std::string str;
			if (lstString.size() > 0) {
				str = lstString.front();
				lstString.pop_front();
			}
			if (pParam->cbFunc) {  // 回调函数处理
				pParam->cbFunc(&str);
			}
			count0++;
		}
		else if (pParam->nOperator == IocpListEmpty) {
			lstString.clear();
		}
		delete pParam;
		printf("delete pParam total:%d\r\n", ++total);
	}
	// lstString.clear();
	printf("thread exit; count:%d; count0:%d\r\n", count, count0);
}

void threadQuereEntry(HANDLE hIOCP)
{
	threadmain(hIOCP);
	_endthread();   // 代码到此为止，会导致本地对象无法调用析构，从而使得内存发生泄漏
}


int main()
{
	if (!CTool::Init()) return 1;

	printf("press any key to exit...\r\n");
	HANDLE hIOCP = INVALID_HANDLE_VALUE;  // IOCP: I/O Completion Port
	hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);  // 最后一个参数：NumberOfConcurrentThreads（这个参数是有讲究的）
	if (hIOCP == INVALID_HANDLE_VALUE || (hIOCP == NULL)) {
		printf("create iocp failed! %d\r\n", GetLastError());
		return 1;
	}
	HANDLE hThread = (HANDLE)_beginthread(threadQuereEntry, 0, hIOCP);
	
	ULONGLONG tick = GetTickCount64();
	ULONGLONG tick0 = GetTickCount64();
	int count = 0, count0 = 0;
	while (_kbhit() == 0) {  // 完成端口；把请求与实现 分离了
		if (GetTickCount64() - tick0 > 1300) {
			PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM), (ULONG_PTR)new IOCP_PARAM(IocpListPop, "hello world", func), NULL);
			tick0 = GetTickCount64();
			count0++;
		}
		if (GetTickCount64() - tick > 2000) {
			PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM), (ULONG_PTR)new IOCP_PARAM(IocpListPush, "hello world"), NULL);
			tick = GetTickCount64();
			count++;
		}
		Sleep(1);
	}

	if (hIOCP != NULL) {
		PostQueuedCompletionStatus(hIOCP, 0, NULL, NULL);
		WaitForSingleObject(hThread, INFINITE);
	}
	CloseHandle(hIOCP);
	printf("exit done! count:%d; count0:%d\r\n", count, count0);
	exit(0);
	

	/*
	if (CTool::IsAdmin()) {
        if (!CTool::Init()) return 1;
		if (ChooseAutoInvoke(INVOKE_PATH)) {
			CCommand cmd;
			int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);
			switch (ret) {
			case -1:
				MessageBox(NULL, _T("网络初始化异常，请检查网络状态"), _T("网络初始化failed"), MB_OK | MB_ICONERROR);
				break;
			case -2:
				MessageBox(NULL, _T("多次无法正常接入用户，结束程序"), _T("接入用户failed"), MB_OK | MB_ICONERROR);
				break;
			}
		}
	}
	else {
        if (CTool::RunAsAdmin() == false) {
            CTool::ShowError();
			return 1;
        }
	}
	*/
    return 0;
}
