// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"

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


int main()
{
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
    return 0;
}
