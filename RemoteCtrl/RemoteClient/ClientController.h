#pragma once
#include "ClientSocket.h"
#include "RemoteClientDlg.h"  // 要包括这几个对话框，都是ClientController的成员
#include "StatusDlg.h"
#include "WatchDialog.h"
#include "resource.h"
#include <map>

// 自定义消息ID
#define WM_SEND_PACK (WM_USER+1)  // 发送包数据
#define WM_SEND_DATA (WM_USER+2)  // 发送数据
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
private:
	CClientController() :m_watchDlg(&m_remoteDlg), m_statusDlg(&m_remoteDlg) // 指定两个子对话框的父窗口
	{
		m_hThread = INVALID_HANDLE_VALUE;
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
	LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam); // 阻塞的

private:
	class helper {
	public:
		helper() {
			CClientController::getInstance();
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
	unsigned int m_nThreadID;
	static CClientController* m_instance;
	static helper m_helper;
};

