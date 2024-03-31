#include "pch.h"
#include "ClientController.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;  // ��̬��ʵ��
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

// pMainWnd ��һ��ָ�� CWnd* ����ָ�������
int CClientController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg;
	return m_remoteDlg.DoModal();
}

//LRESULT CClientController::SendMessage(MSG msg)
//{
//	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  // ����һ��Ψһ���¼�����; �ö�������Ժ������߳�ͬ��
//	if (hEvent == NULL)return -2;
//	MSGINFO info(msg);  // ���ڰ�װ MSG �ṹ���Ա��ڲ�ͬ�߳�֮�䴫�ݡ�
//	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, (WPARAM) & info, (LPARAM) & hEvent);
//	WaitForSingleObject(hEvent, INFINITE);
//	CloseHandle(hEvent);
//	/*
//		�ȴ��¼����� hEvent ����Ϊ signaled ״̬��-1 ��ʾ���޵ȴ���ֱ���¼�����״̬��Ϊ signaled;
//		����ζ�ų����ڴ˴�������ֱ��Ŀ���̴߳�������Ϣ���������¼�����״̬��
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
//		AfxMessageBox(_T("����û��Ȩ�ޱ�����ļ��������ļ��޷�������"));
//		m_statusDlg.ShowWindow(SW_HIDE);
//		m_remoteDlg.EndWaitCursor();
//		return;
//	}
//	CClientSocket* pClient = CClientSocket::getInstance();
//	do {
//		int ret = SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
//		if (ret < 0) {
//			AfxMessageBox("ִ�������ļ�ʧ�ܣ�");
//			TRACE("ִ�������ļ�ʧ�ܣ�ret=%d\r\n", ret);
//			break;
//		}
//		int nLength = *(long long*)pClient->GetPacket().strData.c_str();
//		if (nLength == 0) {
//			AfxMessageBox("�ļ�����Ϊ0���޷���ȡ�ļ���");
//			break;
//		}
//		long long nCount = 0;
//		while (nCount < nLength) {
//			ret = DealCommand();
//			if (ret < 0) {
//				AfxMessageBox("�����ļ�ʧ�ܣ�");
//				TRACE("�����ļ�ʧ�ܣ�ret=%d\r\n", ret);
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
//	m_remoteDlg.MessageBox(_T("�������"), _T("���"));
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
	m_remoteDlg.MessageBox(_T("�������"), _T("���"));
}

int CClientController::DownFile(CString strPath)
{
	CFileDialog dlg(false, NULL, strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, &m_remoteDlg);  // ����һ���ļ��Ի������ dlg
	if (dlg.DoModal() == IDOK)  // ��ʾ�û��Ѿ�ѡ����һ���ļ�������˶Ի����ȷ����ť
	{
		m_strRemote = strPath;  // ����������Ա�����������̵߳�ʱ�򴫵�this���Ϳ���ֱ�ӷ�����������Ա��������ͨ����Ա������ɴ�ֵ��
		m_strLocal = dlg.GetPathName();  // ����·��

		FILE* pFile = fopen(m_strLocal, "wb+");
		if (pFile == NULL) {
			AfxMessageBox(_T("����û��Ȩ�ޱ�����ļ��������ļ��޷�������"));
			return -1;
		}
		SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
		// m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadFileEntry, 0, this);  // this ����ǰ�����ָ�룬�� CClientController �����ָ��
		//if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT) {  // ���ڸճɹ������������̣߳�����Wait�ͻᳬʱ
		//	return -1;
		//}
		m_remoteDlg.BeginWaitCursor(); // �ѹ������Ϊ�ȴ�(ɳ©)״̬
		m_statusDlg.m_info.SetWindowText(_T("��������ִ���У�"));  // m_dlgStatus��������RemoteClientDlg�ĳ�Ա����������Controller�ĳ�Ա
		m_statusDlg.ShowWindow(SW_SHOW);
		m_statusDlg.CenterWindow(&m_remoteDlg);  // ����
		m_statusDlg.SetActiveWindow();
		m_remoteDlg.LoadFileInfo();
	}
	return 0;
}

void CClientController::StartWatchScreen()
{
	m_isClosed = false;  // ��ֻ֤��һ���߳�
	m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreenEntry, 0, this);
	m_watchDlg.DoModal();  // ģ̬�Ի���
	m_isClosed = true;
	WaitForSingleObject(m_hThreadWatch, 500);  // �ȴ���ǰ�������߳̽��������߳�ʱ�����ִ��; ���ȴ��߳̽������500���롣
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
		if (m_watchDlg.isFull() == false) {  // �������ݵ�m_image����
			if (GetTickCount64() - nTick < 200) {
				Sleep(200 - DWORD(GetTickCount64() - nTick));		  // ���Ʒ���Ƶ��		
			}
			nTick = GetTickCount64();
			bool ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), 6, true, NULL, 0); // SendCommandPacket()������Ĭ��ֵ
			if (ret) {
				// TRACE("�ɹ���������ͼƬ����\r\n");
			}
			else 
			{
				TRACE("��������ͼƬ����ʧ��! ret:%d\r\n", ret);
			}
		}
		else {
			Sleep(1); // ��ֹSend()ʧ�ܺ�����ѭ������˲������  ************** ��ѧϰ����˼�롿
		}
	}
	TRACE("thread end %d\r\n", m_isClosed);
}

unsigned __stdcall CClientController::threadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadFunc();  // �����������̺߳���
	_endthreadex(0);  // ʹ�õ�_beginthreadex()���Ͷ�Ӧʹ��_endthreadex()
	return 0;
}

void CClientController::threadFunc()
{
	MSG msg;
	// ������Ϣѭ��
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
			SetEvent(hEvent);  //֪ͨ�� ���¼���״̬����Ϊ���ź�״̬���������еȴ����¼����߳�
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
