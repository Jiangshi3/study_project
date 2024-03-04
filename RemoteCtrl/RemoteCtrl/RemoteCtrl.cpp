// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
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
    // CServerSocket::getInstance()->SendMsg(pack);
    return 0;
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
            /*
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
                int ret = pServer->DealCommand();
                // TODO:
            }
            */
            int nCmd = 1;
            switch (nCmd) {
            case 1: // 查看磁盘分区
                MakeDriverInfo();
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
