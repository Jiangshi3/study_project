#pragma once
#include "ClientSocket.h"
#include "RemoteClientDlg.h"  // Ҫ�����⼸���Ի��򣬶���ClientController�ĳ�Ա
#include "StatusDlg.h"
#include "WatchDialog.h"
#include "resource.h"
#include <map>
#include "Tool.h"

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
	// ��������������ĵ�ַ
	void UpdateAddress(int nIP, int nPort) {
		CClientSocket::getInstance()->UpdateAddress(nIP, nPort);
	}
	int DealCommand() {
		return CClientSocket::getInstance()->DealCommand();
	}
	void CloseSocket() {
		CClientSocket::getInstance()->CloseSocket();
	}
	bool SendPacket(const CPacket& pack) {
		CClientSocket* pClient = CClientSocket::getInstance();
		if (pClient->InitSocket() == false) return false;
		pClient->Send(pack);
	}
	// 1�鿴���̷���
	// 2�鿴ָ��Ŀ¼�µ��ļ�
	// 3�����ļ�
	// 4�����ļ�
	// 5������
	// 6������Ļ����
	// 7����
	// 8����
	// 9ɾ���ļ�
	// 1981��������
	// bAutoClose˵�����ڴ�������ļ�Ŀ¼ʱ�����ܻ���ܶ�������ܽ���һ����close���ӣ�ͨ���˲������жϹر�ʱ��
	int SendCommandPacket(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0)
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		if (pClient->InitSocket() == false) return false;
		pClient->Send(CPacket(nCmd, pData, nLength));
		int cmd = DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (bAutoClose) {
			CloseSocket();  // Ҫ���жϣ�����ֱ�ӶϿ����ӣ���Ϊ����Ҫ���ն��
		}
		return cmd;
	}
	int GetImage(CImage& image)
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		BYTE* pData = (BYTE*)pClient->GetPacket().strData.c_str();
		return CTool::Bytes2Image(image, pClient->GetPacket().strData);
	}

	int DownFile(CString strPath) {
		CFileDialog dlg(false, NULL, strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, &m_remoteDlg);  // ����һ���ļ��Ի������ dlg
		if (dlg.DoModal() == IDOK)  // ��ʾ�û��Ѿ�ѡ����һ���ļ�������˶Ի����ȷ����ť
		{
			m_strRemote = strPath;  // ����������Ա�����������̵߳�ʱ�򴫵�this���Ϳ���ֱ�ӷ�����������Ա��������ͨ����Ա������ɴ�ֵ��
			m_strLocal = dlg.GetPathName();  // ����·��
			m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadFileEntry, 0, this);  // this ����ǰ�����ָ�룬�� CClientController �����ָ��
			if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT) {  // ���ڸճɹ������������̣߳�����Wait�ͻᳬʱ
				return -1;
			}
			m_remoteDlg.BeginWaitCursor(); // �ѹ������Ϊ�ȴ�(ɳ©)״̬
			m_statusDlg.m_info.SetWindowText(_T("��������ִ���У�"));  // m_dlgStatus��������RemoteClientDlg�ĳ�Ա����������Controller�ĳ�Ա
			m_statusDlg.ShowWindow(SW_SHOW);
			m_statusDlg.CenterWindow(&m_remoteDlg);  // ����
			m_statusDlg.SetActiveWindow();
		}
		return 0;
	}

	void StartWatchScreen();

private:
	static void threadDownloadFileEntry(void* arg);
	void threadDownloadFile();
	static void threadWatchScreenEntry(void* arg);
	void threadWatchScreen();
	CClientController() :m_watchDlg(&m_remoteDlg), m_statusDlg(&m_remoteDlg) // ָ�������ӶԻ���ĸ�����
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
	HANDLE m_hThreadDownload; // ���ڽṹ�����̵߳ķ���ֵ�����Կ��������߳̽�����
	HANDLE m_hThreadWatch;
	bool m_isClosed; // �����߳��Ƿ�ر�
	// �����ļ���Զ��·��
	CString m_strRemote;
	// �����ļ��ı��ر���·��
	CString m_strLocal;
	unsigned int m_nThreadID;
	static CClientController* m_instance;
	static helper m_helper;
};

