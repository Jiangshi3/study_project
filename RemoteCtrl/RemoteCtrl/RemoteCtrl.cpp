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
    // CServerSocket::getInstance()->Send(pack);
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
                int ret = pServer->DealCommand(); // 返回的是一个操作指令；
                // TODO:
            }
            */
            int nCmd = 1;
            switch (nCmd) {
            case 1: // 查看磁盘分区
                MakeDriverInfo();
                break;
            case 2: // 查看指定目录下的文件
                MakeDirectoryInfo();
                break;
            case 3:  // 运行文件
                RunFile();
                break;
            case 4:
                DownloadFile();
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
