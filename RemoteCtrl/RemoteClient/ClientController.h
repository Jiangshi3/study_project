#pragma once
#include "ClientSocket.h"
#include "RemoteClientDlg.h"  // Ҫ�����⼸���Ի��򣬶���ClientController�ĳ�Ա
#include "StatusDlg.h"
#include "WatchDialog.h"
#include "resource.h"
#include <map>

// �Զ�����ϢID
#define WM_SEND_PACK (WM_USER+1)  // ���Ͱ�����
#define WM_SEND_DATA (WM_USER+2)  // ��������
#define WM_SHOW_STATUS (WM_USER+3)  // չʾ״̬
#define WM_SHOW_WATCH (WM_USER+4)   // Զ�̼��
#define WM_SEND_MESSAGE (WM_USER+0x1000)  // �Զ�����Ϣ����


class CClientController
{
public:
	static CClientController* getInstance();
	// ��ʼ��
	int InitController();
	// ����
	int Invoke(CWnd*& pMainWnd);  // pMainWnd ��һ��ָ�� CWnd* ����ָ�������
	// ������Ϣ
	LRESULT SendMessage(MSG msg);  // ��ȡһ����Ϣִ�еķ��ؽ��
private:
	CClientController() :m_watchDlg(&m_remoteDlg), m_statusDlg(&m_remoteDlg) // ָ�������ӶԻ���ĸ�����
	{
		m_hThread = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;
	}

	~CClientController() {
		WaitForSingleObject(m_hThread, 100);
	}
	static unsigned __stdcall threadEntry(void* arg);
	void threadFunc();  // �������߳�ִ�к���
	static void releseInstance(){
		if (m_instance != NULL) {
			delete m_instance;
			m_instance = NULL;
		}
	}
	LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam); // ������

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
		// 1��Ϣ�� 2����ֵ�� 3֪ͨ��ͨ��event��֪ͨ��
		MSG msg;
		LRESULT result;  // ͨ��result����ȡִ�к�ķ���ֵ
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
	static std::map<UINT, MSGFUNC> m_mapFunc;  // ����ֻ������Ϊ��̬��û��ȥʵ�֣������Ҫ�õ��Ļ��ᱨ����Ҫ��.cpp����ȥʵ��
	CRemoteClientDlg m_remoteDlg;
	CWatchDialog m_watchDlg;
	CStatusDlg m_statusDlg;
	HANDLE m_hThread;
	unsigned int m_nThreadID;
	static CClientController* m_instance;
	static helper m_helper;
};

