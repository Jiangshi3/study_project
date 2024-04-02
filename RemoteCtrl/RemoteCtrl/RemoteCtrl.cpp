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

CWinApp theApp;
using namespace std;

// 开机启动的时候，程序的权限是跟随启动用户的；如果两者权限不一致，则会导致程序启动失败
// 开机启动对环境变量有影响，如果依赖dll(动态库)，则可能启动失败； 
//   【解决：1、复制这些dll到system32下面或者sysWOW64下面； 2、或者使用静态库而不是动态库】
//    system32下面多是64为程序； sysWOW64下面多是32为程序；

void WriteRegisterTable(const CString& strPath) {
    CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
	char sPath[MAX_PATH] = "";
	char sSys[MAX_PATH] = "";
	std::string strExe = "\\RemoteCtrl.exe ";  // 记得要加空格
	GetCurrentDirectoryA(MAX_PATH, sPath);  // 使用的是多字节字符集，而不是Unicode字符集；
	GetSystemDirectoryA(sSys, sizeof(sSys));
	std::string strCmd = "cmd /K mklink " + std::string(sSys) + strExe + std::string(sPath) + strExe;  // 要执行的命令; cmd /K：执行后窗口不消失
	system(strCmd.c_str());  // bug:发现这个命令执行后，没有把文件创建在C:\\windows\\system32\\下面，而是创建在了C:\\windows\\SysWOW64\\下面？？？
	// 设置register 注册表
	HKEY hKey = NULL;
	int ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
	if (ret != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足？\r\n程序启动失败\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
		exit(0);
	}

	ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));  // LPCTSTR： const char*
	if (ret != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足？\r\n程序启动失败\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
		exit(0);
	}
	RegCloseKey(hKey);
}

void WriteStartupDir(const CString& strPath) {
    // CString strPath = _T("C:\\Users\\J\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe");
    CString strCmd = GetCommandLine();
    strCmd.Replace(_T("\""), _T("")); // 把双引号替换为空
    bool ret = CopyFile(strCmd, strPath, FALSE);
    if (ret == false) {
        MessageBox(NULL, _T("复制文件失败\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
        exit(0);
    }
}

void ChooseAutoInvoke() {
    // CString strPath =CString(_T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe"));  // 把本应该是system32修改为SysWOW64
    CString strPath = _T("C:\\Users\\J\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe");
    if (PathFileExists(strPath)) {
        return;
    }
    CString strInfo = _T("该程序只允许用于合法的用途！\n");
    strInfo += _T("继续运行该程序，将使得这台机器处于被监控状态！\n");
    strInfo += _T("如果你不希望这样，请按“取消”按钮退出程序。\n");
    strInfo += _T("按下“是”按钮，该程序将被复制到你的机器上，并随系统启动而自动运行！\n");
    strInfo += _T("按下“否”按钮，程序只运行一次，不会在系统内留下任何东西！\n");
    int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES) {
        WriteRegisterTable(strPath);
        // WriteStartupDir(strPath);
    }
    else if (ret == IDCANCEL) {
        exit(0);
    }
    return;
}

// 获取最近一次系统错误消息，并将其输出到调试器
void ShowError() {  
    LPTSTR lpMessageBuf = NULL;
    // strerror(errno);  // 标准c语言库的
    // FormatMessage()格式化系统错误消息
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL, GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMessageBuf, 0, NULL); 
    OutputDebugString(lpMessageBuf);  // 将格式化后的错误消息输出到调试器，通常用于调试目的。
    LocalFree(lpMessageBuf);
}

// 检查当前进程是否以管理员权限运行
bool IsAdmin() {
    HANDLE hToken = NULL;  // 用于存储当前进程的访问令牌(Token)
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {  // 获取当前进程的访问令牌;
        ShowError();
        return false;
    }
    TOKEN_ELEVATION eve;
    DWORD len = 0;
    if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == FALSE) {
        ShowError();
        MessageBox(NULL, _T("GetTokenInformation失败！"), _T("错误"), 0);
        return false;
    }
    CloseHandle(hToken);
    if (len == sizeof(eve)) {
        return eve.TokenIsElevated;  // 返回令牌的提升状态
    }
    printf("length of tokeninformation is %d\r\n", len);
    MessageBox(NULL, _T("获取令牌提升状态失败！"), _T("错误"), 0);
    return false;
}

void RunAsAdmin() {
    // 获取管理员权限，使用该权限创建进程
    HANDLE hToken = NULL;
    // LOGON32_LOGON_INTERACTIVE
    bool ret = LogonUser(L"Administrator", NULL, NULL, LOGON32_LOGON_BATCH, LOGON32_PROVIDER_DEFAULT, &hToken);  // 以指定用户的身份登录系统
    if (!ret) {
        ShowError();
        MessageBox(NULL, _T("LogonUser登录错误！"), _T("程序错误"), 0);
        exit(0);
    }
    OutputDebugString(L"logon administrator success!\r\n");
    STARTUPINFO si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    TCHAR sPath[MAX_PATH] = _T("");
    GetCurrentDirectory(MAX_PATH, sPath);  // 获取当前进程运行的路径
    CString strCmd = sPath;
    strCmd += _T("\\RemoteCtrl.exe");
    // 创建进程
    // ret = CreateProcessWithTokenW(hToken, LOGON_WITH_PROFILE, NULL, (LPWSTR)(LPCWSTR)strCmd, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi); 
    ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, (LPWSTR)(LPCWSTR)strCmd, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
    CloseHandle(hToken);
    if (!ret) {
        ShowError();
        MessageBox(NULL, _T("创建进程失败"), _T("程序错误"), 0);
        exit(0);
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
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
			if (IsAdmin()) {
                MessageBox(NULL, _T("Admin"), _T("用户状态"), 0);
				OutputDebugString(L"current is run as administrator!\r\n");
            }
			else {
                MessageBox(NULL, _T("普通用户，下一步进行提权"), _T("用户状态"), 0);
				OutputDebugString(L"current is run as normal user!\r\n");
                RunAsAdmin();                
				return nRetCode;
			}

            CCommand cmd;
            ChooseAutoInvoke();
            int count = 0;
            int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);  // cmd是一个命令类对象
            switch (ret) {
            case -1:
				MessageBox(NULL, _T("网络初始化异常，请检查网络状态"), _T("网络初始化failed"), MB_OK | MB_ICONERROR);
				exit(0);
                break;
            case -2:
				MessageBox(NULL, _T("多次无法正常接入用户，结束程序"), _T("接入用户failed"), MB_OK | MB_ICONERROR);
				exit(0);
                break;
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
