#include "pch.h"
#include "ClientController.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;  // 静态的实现
CClientController* CClientController::m_instance = NULL;
CClientController::helper CClientController::m_helper;

CClientController* CClientController::getInstance()
{
	if (m_instance == NULL) {
		m_instance = new CClientController();
		struct { 
			UINT nMsg;
			MSGFUNC func;
		}MsgFuncs[] = { 
			// {WM_SEND_PACK, &CClientController::OnSendPack},
			// {WM_SEND_DATA, &CClientController::OnSendData},
			{WM_SHOW_STATUS, &CClientController::OnShowStatus},
			{WM_SHOW_WATCH, &CClientController::OnShowWatcher},
			{(UINT)-1, NULL}
		};
		for (int i = 0; MsgFuncs[i].nMsg = !- 1; i++)
		{
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].nMsg, MsgFuncs[i].func));
			// m_mapFunc.insert(std::make_pair(MsgFuncs[i].nMsg, MsgFuncs[i].func));
		}
	}
	return m_instance;
}

int CClientController::InitController()
{
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientController::threadEntry, this, 0, &m_nThreadID);
	m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);
	return 0;
}

// pMainWnd 是一个指向 CWnd* 类型指针的引用
int CClientController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg;
	return m_remoteDlg.DoModal();
}

//LRESULT CClientController::SendMessage(MSG msg)
//{
//	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  // 返回一个唯一的事件对象; 该对象会在稍后用于线程同步
//	if (hEvent == NULL)return -2;
//	MSGINFO info(msg);  // 用于包装 MSG 结构体以便在不同线程之间传递。
//	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, (WPARAM) & info, (LPARAM) & hEvent);
//	WaitForSingleObject(hEvent, INFINITE);
//	CloseHandle(hEvent);
//	/*
//		等待事件对象 hEvent 被置为 signaled 状态。-1 表示无限等待，直到事件对象状态变为 signaled;
//		这意味着程序将在此处阻塞，直到目标线程处理完消息并设置了事件对象状态。
//	*/
//	return info.result;
//}


//void CClientController::threadDownloadFileEntry(void* arg)
//{
//	CClientController* thiz = (CClientController*)arg;
//	thiz->threadDownloadFile();
//	_endthread();
//}

//void CClientController::threadDownloadFile()
//{
//	FILE* pFile = fopen(m_strLocal, "wb+");
//	if (pFile == NULL) {
//		AfxMessageBox(_T("本地没有权限保存该文件，或者文件无法创建！"));
//		m_statusDlg.ShowWindow(SW_HIDE);
//		m_remoteDlg.EndWaitCursor();
//		return;
//	}
//	CClientSocket* pClient = CClientSocket::getInstance();
//	do {
//		int ret = SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
//		if (ret < 0) {
//			AfxMessageBox("执行下载文件失败！");
//			TRACE("执行下载文件失败：ret=%d\r\n", ret);
//			break;
//		}
//		int nLength = *(long long*)pClient->GetPacket().strData.c_str();
//		if (nLength == 0) {
//			AfxMessageBox("文件长度为0或无法读取文件！");
//			break;
//		}
//		long long nCount = 0;
//		while (nCount < nLength) {
//			ret = DealCommand();
//			if (ret < 0) {
//				AfxMessageBox("传输文件失败！");
//				TRACE("传输文件失败：ret=%d\r\n", ret);
//				break;
//			}
//			fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
//			nCount += pClient->GetPacket().strData.size();
//		}	
//	} while (false);
//	fclose(pFile);
//	CloseSocket();
//	m_statusDlg.ShowWindow(SW_HIDE);
//	m_remoteDlg.EndWaitCursor();
//	m_remoteDlg.MessageBox(_T("下载完成"), _T("完成"));
//}


bool CClientController::SendCommandPacket(HWND hWnd, int nCmd, bool bAutoClose, BYTE* pData, size_t nLength, WPARAM wParam)
{
	TRACE("nCmd:%d; %s start %lld\r\n", nCmd, __FUNCTION__, GetTickCount64());
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret = pClient->SendPacket(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam);
	return ret;
}

void CClientController::DownloadEnd()
{
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(_T("下载完成"), _T("完成"));
}

int CClientController::DownFile(CString strPath)
{
	CFileDialog dlg(false, NULL, strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, &m_remoteDlg);  // 创建一个文件对话框对象 dlg
	if (dlg.DoModal() == IDOK)  // 表示用户已经选择了一个文件并点击了对话框的确定按钮
	{
		m_strRemote = strPath;  // 设置两个成员变量，开启线程的时候传递this，就可以直接访问这两个成员变量；【通过成员变量完成传值】
		m_strLocal = dlg.GetPathName();  // 本地路径

		FILE* pFile = fopen(m_strLocal, "wb+");
		if (pFile == NULL) {
			AfxMessageBox(_T("本地没有权限保存该文件，或者文件无法创建！"));
			return -1;
		}
		SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
		// m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadFileEntry, 0, this);  // this 代表当前对象的指针，即 CClientController 对象的指针
		//if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT) {  // 对于刚成功创建出来的线程，进行Wait就会超时
		//	return -1;
		//}
		m_remoteDlg.BeginWaitCursor(); // 把光标设置为等待(沙漏)状态
		m_statusDlg.m_info.SetWindowText(_T("命令正在执行中！"));  // m_dlgStatus不再属于RemoteClientDlg的成员；而是属于Controller的成员
		m_statusDlg.ShowWindow(SW_SHOW);
		m_statusDlg.CenterWindow(&m_remoteDlg);  // 居中
		m_statusDlg.SetActiveWindow();
		m_remoteDlg.LoadFileInfo();
	}
	return 0;
}

void CClientController::StartWatchScreen()
{
	m_isClosed = false;  // 保证只有一个线程
	m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreenEntry, 0, this);
	m_watchDlg.DoModal();  // 模态对话框
	m_isClosed = true;
	WaitForSingleObject(m_hThreadWatch, 500);  // 等待先前启动的线程结束，或者超时后继续执行; 它等待线程结束最多500毫秒。
}

void CClientController::threadWatchScreenEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadWatchScreen();
	_endthread();
}

void CClientController::threadWatchScreen()
{
	Sleep(50);
	ULONGLONG nTick = GetTickCount64();
	while (!m_isClosed) {
		if (m_watchDlg.isFull() == false) {  // 更新数据到m_image缓存
			if (GetTickCount64() - nTick < 200) {
				Sleep(200 - DWORD(GetTickCount64() - nTick));		  // 控制发送频率		
			}
			nTick = GetTickCount64();
			bool ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), 6, true, NULL, 0); // SendCommandPacket()设置了默认值
			if (ret) {
				// TRACE("成功发送请求图片命令\r\n");
			}
			else 
			{
				TRACE("发送请求图片命令失败! ret:%d\r\n", ret);
			}
		}
		else {
			Sleep(1); // 防止Send()失败后，在死循环里面瞬间拉满  ************** 【学习这种思想】
		}
	}
	TRACE("thread end %d\r\n", m_isClosed);
}

unsigned __stdcall CClientController::threadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadFunc();  // 访问真正的线程函数
	_endthreadex(0);  // 使用的_beginthreadex()，就对应使用_endthreadex()
	return 0;
}

void CClientController::threadFunc()
{
	MSG msg;
	// 建立消息循环
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_SEND_MESSAGE) {
			MSGINFO* pmsg = (MSGINFO*)msg.wParam;
			HANDLE hEvent = (HANDLE)msg.lParam;
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				pmsg->result = (this->*(it->second))(pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam);			
			}
			else {
				pmsg->result = -1;
			}
			SetEvent(hEvent);  //通知； 将事件的状态设置为有信号状态，唤醒所有等待该事件的线程
		}
		else
		{
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				(this->*(it->second))(msg.message, msg.wParam, msg.lParam);
			}
		}
	}
}

//LRESULT CClientController::OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
//{
//	CClientSocket* pClient = CClientSocket::getInstance();
//	CPacket* pPacket = (CPacket*)wParam;
//	return pClient->Send(*pPacket);
//}

//LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
//{
//	CClientSocket* pClient = CClientSocket::getInstance();
//	char* pBuffer = (char*)wParam;
//	return pClient->Send(pBuffer, (int)lParam);
//}

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}
