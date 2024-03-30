#pragma once
#include "ClientSocket.h"
#include "RemoteClientDlg.h"  // 要包括这几个对话框，都是ClientController的成员
#include "StatusDlg.h"
#include "WatchDialog.h"
#include "resource.h"
#include <map>
#include "Tool.h"

// 自定义消息ID
// #define WM_SEND_PACK (WM_USER+1)  // 发送包数据
// #define WM_SEND_DATA (WM_USER+2)  // 发送数据
#define WM_SHOW_STATUS (WM_USER+3)  // 展示状态
#define WM_SHOW_WATCH (WM_USER+4)   // 远程监控
#define WM_SEND_MESSAGE (WM_USER+0x1000)  // 自定义消息处理


class CClientController
{
public:
	static CClientController* getInstance();
	// 初始化
	int InitController();
	// 启动
	int Invoke(CWnd*& pMainWnd);  // pMainWnd 是一个指向 CWnd* 类型指针的引用
	// 发送消息
	LRESULT SendMessage(MSG msg);  // 获取一个消息执行的返回结果
	// 更新网络服务器的地址
	void UpdateAddress(int nIP, int nPort) {
		CClientSocket::getInstance()->UpdateAddress(nIP, nPort);
	}
	int DealCommand() {
		return CClientSocket::getInstance()->DealCommand();
	}
	void CloseSocket() {
		CClientSocket::getInstance()->CloseSocket();
	}

	// 1查看磁盘分区
	// 2查看指定目录下的文件
	// 3运行文件
	// 4下载文件
	// 5鼠标操作
	// 6发送屏幕内容
	// 7锁机
	// 8解锁
	// 9删除文件
	// 1981测试连接
	// 返回值：状态
	// bAutoClose说明：在处理接受文件目录时，可能会接受多个，不能接受一个就close连接，通过此参数来判断关闭时机
	bool SendCommandPacket(
		HWND hWnd,  // 数据包收到后，需要应答的窗口
		int nCmd, 
		bool bAutoClose = true, 
		BYTE* pData = NULL, 
		size_t nLength = 0,
		WPARAM wParam = 0);
	//int GetImage(CImage& image)
	//{
	//	CClientSocket* pClient = CClientSocket::getInstance();
	//	BYTE* pData = (BYTE*)pClient->GetPacket().strData.c_str();
	//	return CTool::Bytes2Image(image, pClient->GetPacket().strData);
	//}
	void DownloadEnd();
	int DownFile(CString strPath);

	void StartWatchScreen();

private:
	static void threadDownloadFileEntry(void* arg);
	void threadDownloadFile();
	static void threadWatchScreenEntry(void* arg);
	void threadWatchScreen();
	CClientController() :m_watchDlg(&m_remoteDlg), m_statusDlg(&m_remoteDlg) // 指定两个子对话框的父窗口
	{
		m_isClosed = true;
		m_hThread = INVALID_HANDLE_VALUE;
		m_hThreadDownload = INVALID_HANDLE_VALUE;
		m_hThreadWatch = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;
	}

	~CClientController() {
		WaitForSingleObject(m_hThread, 100);
	}
	static unsigned __stdcall threadEntry(void* arg);
	void threadFunc();  // 真正的线程执行函数
	static void releseInstance(){
		if (m_instance != NULL) {
			delete m_instance;
			m_instance = NULL;
		}
	}
	// LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	// LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam); // 阻塞的

private:
	class helper {
	public:
		helper() {
			// CClientController::getInstance();  // 先不着急构造，如果直接构造后面会有个GetApp()，但此时App还么有创建，会报错；
		}
		~helper() {
			CClientController::releseInstance();
		}
	};
private:
	typedef struct MsgInfo {
		// 1消息， 2返回值， 3通知（通过event来通知）
		MSG msg;
		LRESULT result;  // 通过result来获取执行后的返回值
		MsgInfo(MSG m) {
			memcpy(&msg, &m, sizeof(MSG));
			result = 0;
		}
		MsgInfo(const MsgInfo& m) {
			memcpy(&msg, &m.msg, sizeof(MSG));
			result = m.result;
		}
		MsgInfo& operator=(const MsgInfo& m) {
			if (this != &m) {
				memcpy(&msg, &m.msg, sizeof(MSG));
				result = m.result;
			}
			return *this;
		}
	}MSGINFO;
	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	static std::map<UINT, MSGFUNC> m_mapFunc;  // 这里只是声明为静态；没有去实现，如果需要用到的话会报错；需要到.cpp里面去实现
	CRemoteClientDlg m_remoteDlg;
	CWatchDialog m_watchDlg;
	CStatusDlg m_statusDlg;
	HANDLE m_hThread;
	HANDLE m_hThreadDownload; // 用于结构开启线程的返回值；可以控制下载线程结束；
	HANDLE m_hThreadWatch;
	bool m_isClosed; // 监视线程是否关闭
	// 下载文件的远程路径
	CString m_strRemote;
	// 下载文件的本地保存路径
	CString m_strLocal;
	unsigned int m_nThreadID;
	static CClientController* m_instance;
	static helper m_helper;
};

