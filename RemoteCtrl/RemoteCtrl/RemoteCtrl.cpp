// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"
#include <conio.h>
#include "CQueue.h"
#include <MSWSock.h>
#include "CServer.h"

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


void iocp();

void udp_server();
void udp_client(bool ishost = true);

void initsock() {
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
}
void clearsock() {
	WSACleanup();
}


// int wmain(int argc, TCHAR* argv[]);
// int _tmain(int argc, TCHAR* argv[]);
int main(int argc, char* argv[])
{
	if (!CTool::Init()) return 1;
	initsock();
	if (argc == 1) {
		char wstrDir[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, wstrDir);
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		memset(&si, 0, sizeof(si));
		memset(&pi, 0, sizeof(pi));
		string strCmd = argv[0];
		strCmd += " 1";  // 此时的argc=2
		BOOL bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, (LPCSTR)wstrDir, &si, &pi);  // 如果想弹一个新的窗口，就填CREATE_NEW；否则就可以填0
		if (bRet) {
			// 创建新进程后，PROCESS_INFORMATION结构中的句柄通常在创建进程后立即关闭，除非程序需要与新创建的进程或线程进行交互。  
			// 【这里只需要让新线程去启动main中的if(argc==2)的客户端代码】
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
			TRACE("进程ID:%d\r\n",pi.dwProcessId);
			TRACE("线程ID:%d\r\n", pi.dwThreadId);

			strCmd += " 2";  // 此时的argc=3
			bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);  // 启动客户端代码
			if (bRet) {
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
				TRACE("进程ID:%d\r\n", pi.dwProcessId);
				TRACE("线程ID:%d\r\n", pi.dwThreadId);
				udp_server();   // 服务器代码
			}
		}
	}
	else if (argc == 2) { // 主客户端代码
		udp_client();
	}
	else {  // 从客户端代码
		udp_client(false);
	}
	clearsock();

	// iocp();

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

class COverlapped {
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;
	char m_buffer[4096];
	COverlapped() {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_operator = 0;
		memset(m_buffer, 0, sizeof(m_buffer));
	}
};

void iocp() {
	CServer server;
	server.StartService();
	getchar();
	/*
	// SOCKET sock = socket(AF_INET, SOCK_STREAM, 0); // TCP
	SOCKET sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);  // 非阻塞的socket
	if (sock == INVALID_SOCKET) {
		CTool::ShowError();
		return;
	}
	SOCKET client = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED); // 为后面的AcceptEx();创建的客户端套接字
	HANDLE hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, sock, 4);  // 创建一个新的IO完成端口对象
	CreateIoCompletionPort((HANDLE)sock, hIOCP, 0, 0);  // 绑定；将指定的套接字与之前创建的IO完成端口关联起来

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("0.0.0.0"); // INADDR_ANY
	addr.sin_port = htons(9527);
	bind(sock, (sockaddr*)&addr, sizeof(addr));
	listen(sock, 5);

	COverlapped overlapped;
	overlapped.m_operator = 1;
	memset(&overlapped, 0, sizeof(COverlapped));
	DWORD received = 0;
	//  accept();  // 返回的是一个SOCKET;      AcceptEx(); 返回值是bool,因为套接字已经提前创建好了
	if (AcceptEx(sock, client, overlapped.m_buffer, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &received, &overlapped.m_overlapped) == FALSE) {  // 非阻塞/异步
		CTool::ShowError();
		return;
	}

	overlapped.m_overlapped = 2;
	WSASend(); // 里面有OVERLAPPED，可以异步
	overlapped.m_overlapped = 3;
	WSARecv();

	// 开启线程
	while (true) { // 代表一个线程
		LPOVERLAPPED pOverlapped = NULL;
		DWORD transferred = 0;
		DWORD key = 0;
		if (GetQueuedCompletionStatus(hIOCP, &transferred, &key, &pOverlapped, INFINITY)) {		
			// 宏CONTAINING_RECORD：可以根据一个结构体中某个成员的地址，计算出整个结构体的地址。 
			// 前面AcceptEx()只需要投递overlapped.m_overlapped进去，就可以实现COverlapped* pO与前面COverlapped overlapped;的对应
			COverlapped* pO = CONTAINING_RECORD(pOverlapped, COverlapped, m_overlapped);  // 通过成员变量m_overlapped拿到结构体/类COverlapped指针
			switch (pO->m_operator)
			{
			case 1:
				// 处理accept的操作
				break;
			default:
				break;
			}
		}
	}	
	*/
}

#include "MNetwork.h"
// 可以放入一个业务类(垃圾堆)里面
int RecvFromCB(void* arg, const MBuffer& buffer, MSockaddrIn& addr) {
	MServer* server = (MServer*)arg;
	return server->SendTo(addr, buffer);
}
int SendToCB(void* arg, const MSockaddrIn& addr, int ret) {
	MServer* server = (MServer*)arg;
	printf("sendto done!%p\r\n", server);
	return 0;
}

void udp_server() {
	printf("udp_server() %s(%d)%s\r\n", __FILE__, __LINE__, __FUNCTION__);
	std::vector<MSockaddrIn> lstclients;
	MServerParameter param(
		"127.0.0.1", 20000, MTypeUDP, NULL, NULL, NULL, RecvFromCB, SendToCB
	);
	MServer server(param);
	server.Invoke(&server);
	printf("udp_server() %s(%d)%s\r\n",__FILE__, __LINE__,__FUNCTION__);
	getchar();
	return;	
}
void udp_client(bool ishost) {
	Sleep(2000);  // 等待服务器先运行起来
	sockaddr_in server, client;
	int len = sizeof(client);
	memset(&server, 0, sizeof(server));
	memset(&client, 0, sizeof(client));
	server.sin_family = AF_INET;
	server.sin_port = htons(20000); // UDP的端口要设置的大一些
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) {
		printf("%s(%d)%s udp_client() ERROR!\r\n", __FILE__, __LINE__, __FUNCTION__);
		return;
	}
	if (ishost) {  // 主客户端代码
		printf("主机  %s(%d)%s\r\n", __FILE__, __LINE__, __FUNCTION__);
		// std::string msg = "hello world\n";
		MBuffer msg = "hello world\n";
		int ret = sendto(sock, msg, msg.size(), 0, (sockaddr*)&server, sizeof(server));
		printf("%s(%d)%s sendto()-ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
		if (ret > 0) {
			msg.resize(1024);
			memset((char*)msg.c_str(), 0, sizeof(msg));
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
			printf("%s(%d)%s recvfrom() ERROR(%d)! ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			if (ret > 0) {
				printf("%s(%d)%s IP:%08X port:%d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, ntohs(client.sin_port));
				printf("%s(%d)%s msg_size=%d\r\n", __FILE__, __LINE__, __FUNCTION__, msg.size());
			}

			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
			printf("%s(%d)%s recvfrom() ERROR(%d)! ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			if (ret > 0) {
				printf("%s(%d)%s IP:%08X port:%d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, ntohs(client.sin_port));
				printf("%s(%d)%s msg=%s\r\n", __FILE__, __LINE__, __FUNCTION__, msg.c_str());
			}
		}
	}
	else {    // 从客户端代码
		Sleep(10);  // 保证主机线程先运行起来
		printf("从机  %s(%d)%s\r\n", __FILE__, __LINE__, __FUNCTION__);
		std::string msg = "hello world";
		int ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server, sizeof(server));
		printf("%s(%d)%s sendto()-ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
		if (ret > 0) {
			msg.resize(1024);
			memset((char*)msg.c_str(), 0, sizeof(msg));
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
			printf("%s(%d)%s recvfrom() ERROR(%d)! ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			if (ret > 0) {
				sockaddr_in addr;
				memcpy(&addr, msg.c_str(), sizeof(sockaddr_in));
				sockaddr_in* paddr = (sockaddr_in*)&addr;
				printf("%s(%d)%s IP:%08X port:%d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, ntohs(client.sin_port));
				printf("%s(%d)%s msg_size=%d\r\n", __FILE__, __LINE__, __FUNCTION__, msg.size());
				printf("%s(%d)%s IP:%08X port:%d\r\n", __FILE__, __LINE__, __FUNCTION__, paddr->sin_addr.s_addr, ntohs(paddr->sin_port));
				msg = "hello,I am client!\r\n";
				ret = sendto(sock,msg.c_str(),msg.size(), 0, (sockaddr*)paddr, sizeof(sockaddr_in));
				printf("%s(%d)%s recvfrom() ERROR(%d)! ret=%d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			}
		}
	}
	closesocket(sock);
}


/*
* 1 思路：做或者实现一个需求的过程
*		确定需求(阶段性的)、选定技术方案(依据技术点)、从框架开发到细节实现(从顶到底)、编译问题、
*		内存泄漏(线程结束，exit函数)、bug排查与功能测试(日志、断点、调用堆栈、内存、监视、局部变量、自动变量)、
*		压力测试(额外写代码的，写到极限)、功能上线
*		【不要混】
* 2 设计：易用性、移植性(可复用性)、安全性(线程安全、异常处理、资源处理cpu/内存)、稳定性(鲁邦性)、可扩展性
*		有度、有条件的(可读性、效率、易用性)
*		【自己的代码库】
* 
* 【学会的才是自己的】
*/
