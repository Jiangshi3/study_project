
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "WatchDialog.h"
// #include "ClientSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_list);
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPACKET)  // 注册消息③  告诉消息的响应函数
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	// 设置IP地址、端口的初始化
	UpdateData();
	m_server_address = 0x7F000001;  // 127.0.0.1
	m_nPort = _T("9527");
	UpdateData(FALSE);

	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);

	m_isFull = false;  // 初始化缓冲没有数据

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CRemoteClientDlg::OnBnClickedBtnTest()
{
	SendCommandPacket(1981);
}

int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
{
	UpdateData();
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret = pClient->InitSocket(m_server_address, atoi((LPCTSTR)m_nPort));
	if (ret == false) {
		AfxMessageBox("网络初始化失败");
		return -1;
	}
	CPacket pack(nCmd, pData, nLength);
	pClient->Send(pack);

	pClient->DealCommand();
	int cmd = pClient->GetPacket().sCmd;
	// TRACE("ack:%d\r\n", cmd);
	if (bAutoClose) {
		pClient->CloseSocket();  // 要先判断，不能直接断开连接；因为可能要接收多个
	}
	return cmd;
}

void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	int ret = SendCommandPacket(1);
	if (ret < 0) {
		AfxMessageBox("命令处理失败！");
		return;
	}
	m_tree.DeleteAllItems();
	CClientSocket* pClient = CClientSocket::getInstance();
	std::string drivers = pClient->GetPacket().strData;  // 是以','分割;   "C,D,E"
	std::string dr;
	for (int i = 0; i < drivers.size(); i++) {
		if (drivers[i] != ',') {
			dr = drivers[i];
			dr += ":";
			HTREEITEM hTemp = m_tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			m_tree.InsertItem("", hTemp, TVI_LAST);  // 在盘符后面添加一个空的子节点
			dr.clear();
		}
	}
}


CString CRemoteClientDlg::GetPath(HTREEITEM hTree)
{
	CString strRet, strTmp;
	do{
		strTmp = m_tree.GetItemText(hTree);
		strRet = strTmp + '\\' + strRet;
		hTree = m_tree.GetParentItem(hTree);
	} while (hTree != NULL);
	return strRet;
}

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do {
		hSub = m_tree.GetChildItem(hTree);  // GetChildItem(hTree) 函数用于获取指定节点 hTree 的第一个子节点
		if (hSub != NULL) m_tree.DeleteItem(hSub);
	} while (hSub != NULL);
}


void CRemoteClientDlg::LoadFileCurrrent()  // 用于删除文件后更新m_list
{
	HTREEITEM hTree = m_tree.GetSelectedItem();
	CString strPath = GetPath(hTree);

	m_list.DeleteAllItems();
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	CClientSocket* pClient = CClientSocket::getInstance();
	PFILEINFO pInfo = (PFILEINFO)pClient->GetPacket().strData.c_str();
	while (pInfo->HasNext) {
		TRACE("[%s] isdir: %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (pInfo->IsDirectory==FALSE) {
			m_list.InsertItem(0, pInfo->szFileName);
		}

		int cmd = pClient->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0)break;
		pInfo = (PFILEINFO)pClient->GetPacket().strData.c_str();
	}
	pClient->CloseSocket();
}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_tree.ScreenToClient(&ptMouse);  // 将获取到的鼠标坐标转换为相对于树形控件客户区的坐标。
	HTREEITEM hTreeSelected = m_tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL)  // 如果点击为空白
		return;
	if (m_tree.GetChildItem(hTreeSelected) == NULL)  // 如果没有子目录（如果是个文件）
		return;
	DeleteTreeChildrenItem(hTreeSelected); // 先清空
	m_list.DeleteAllItems();
	CString strPath = GetPath(hTreeSelected);
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	CClientSocket* pClient = CClientSocket::getInstance();
	PFILEINFO pInfo = (PFILEINFO)pClient->GetPacket().strData.c_str();
	int Count = 0;
	while (pInfo->HasNext) {
		TRACE("[%s] isdir: %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (pInfo->IsDirectory) {
			if ((CString)(pInfo->szFileName) == "." || (CString)(pInfo->szFileName) == "..")  // 排除这两个目录
			{
				int cmd = pClient->DealCommand();
				TRACE("ack:%d\r\n", cmd);
				if (cmd < 0)break;
				pInfo = (PFILEINFO)pClient->GetPacket().strData.c_str();
				continue;
			}
			HTREEITEM hTemp = m_tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);
			m_tree.InsertItem("", hTemp, TVI_LAST);  // 如果是目录，在后面添加一个空的子节点
		}
		else {
			m_list.InsertItem(0, pInfo->szFileName);
		}
		Count++;
		int cmd = pClient->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0)break;
		pInfo = (PFILEINFO)pClient->GetPacket().strData.c_str();
	}
	pClient->CloseSocket();
	TRACE("Count: %d\r\n", Count);
}

void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}

// 单击显示
void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}

// 双击显示
void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	CPoint ptMouse, ptList;  // 设置两个Point点；其中一个用于保留最初的点击位置
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_list.ScreenToClient(&ptList);
	int ListSelected = m_list.HitTest(ptList);
	if (ListSelected < 0) return;
	CMenu menu;  // 右击到了
	menu.LoadMenu(IDR_MENU_RCLICK); // 加载菜单
	CMenu* pPupup = menu.GetSubMenu(0);
	if (pPupup != NULL) {
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);  // 左对齐
	}
}



void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	m_isClosed = false;  // 保证只有一个线程
	CWatchDialog dlg(this);
	HANDLE hThread = (HANDLE)_beginthread(CRemoteClientDlg::threadEntryWatchData, 0, this);
	dlg.DoModal();  // 模态对话框
	m_isClosed = true;
	WaitForSingleObject(hThread, 500);  // 等待先前启动的线程结束，或者超时后继续执行; 它等待线程结束最多500毫秒。
}

void CRemoteClientDlg::threadEntryWatchData(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;  // 传递进来的参数是this
	thiz->threadWatchData();  // 此时就可以通过thiz来调用非静态成员函数了
	_endthread();
}

void CRemoteClientDlg::threadWatchData()
{
	Sleep(50);
	CClientSocket* pClient = NULL;
	do {
		pClient = CClientSocket::getInstance();
	} while (pClient == NULL);  // 保证pClient不为空，以建立连接  ************** 【学习这种写法】
	while(!m_isClosed) {
		if (m_isFull == false) {  // 更新数据到m_image缓存
			int ret = SendMessage(WM_SEND_PACKET, 6 << 1 | 1);
			if (ret == 6) {
				BYTE* pData = (BYTE*)pClient->GetPacket().strData.c_str();
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);  // 使用 GlobalAlloc() 分配一个可移动的全局内存块，并将句柄存储在 hMem 中
				if (hMem == NULL) {
					TRACE(_T("内存不足！"));
					Sleep(1);  // 注意在死循环内部，最好要添加Sleep()防止拉满
					continue;
				}
				IStream* pStream = NULL;
				HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);  // 调用 CreateStreamOnHGlobal() 函数，将全局内存块转换为一个流对象，并将结果存储在 pStream 中
				if (hRet == S_OK) {
					ULONG length = 0;
					pStream->Write(pData, pClient->GetPacket().strData.size(), &length);  // 将字节数据写入流中
					LARGE_INTEGER bg = { 0 };  // 用于在流中设置指针位置
					pStream->Seek(bg, STREAM_SEEK_SET, NULL);  // 将流的指针位置设置为起始位置，以便后续读取操作。
					if ((HBITMAP)m_image != NULL) m_image.Destroy();
					m_image.Load(pStream);  // 将图像从数据流中解析并加载到 m_image 对象中
					m_isFull = true;
				}
			}
			else {
				Sleep(1);
			}
		}
		else {
			Sleep(1); // 防止Send()失败后，在死循环里面瞬间拉满  ************** 【学习这种思想】
		}
	}
}

void CRemoteClientDlg::OnDownloadFile()
{
	_beginthread(CRemoteClientDlg::threadEntryDownFile, 0, this);  // 线程
	BeginWaitCursor();  // 把光标设置为等待(沙漏)状态
	m_dlgStatus.m_info.SetWindowText(_T("命令正在执行中！"));
	m_dlgStatus.ShowWindow(SW_SHOW);
	m_dlgStatus.CenterWindow(this);  // 居中
	m_dlgStatus.SetActiveWindow();
	// Sleep(50); // 确保线程开启
}

void CRemoteClientDlg::threadEntryDownFile(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;  // 传递进来的参数是this
	thiz->threadDownFile();  // 此时就可以通过thiz来调用非静态成员函数了
	_endthread();
}

void CRemoteClientDlg::threadDownFile()
{
	int nListSelected = m_list.GetSelectionMark();  // 列表框控件中获取当前选择的项目的索引
	CString strFile = m_list.GetItemText(nListSelected, 0);
	CFileDialog dlg(false, NULL, strFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, this);  // 创建一个文件对话框对象 dlg
	if (dlg.DoModal() == IDOK)  // 表示用户已经选择了一个文件并点击了对话框的确定按钮
	{
		FILE* pFile = fopen(dlg.GetPathName(), "wb+");
		if (pFile == NULL) {
			AfxMessageBox(_T("本地没有权限保存该文件，或者文件无法创建！"));
			m_dlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();
			return;
		}
		HTREEITEM hSelected = m_tree.GetSelectedItem();
		strFile = GetPath(hSelected) + strFile;
		TRACE("%s\r\n", (LPCTSTR)strFile);
		CClientSocket* pClient = CClientSocket::getInstance();
		do {
			//int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
			int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCSTR)strFile);   // 通过SendMessage();    WM_SEND_PACKET消息ID
			if (ret < 0) {
				AfxMessageBox("执行下载文件失败！");
				TRACE("执行下载文件失败：ret=%d\r\n", ret);
				break;
			}
			int nLength = *(long long*)pClient->GetPacket().strData.c_str();
			if (nLength == 0) {
				AfxMessageBox("文件长度为0或无法读取文件！");
				break;
			}
			long long nCount = 0;
			while (nCount < nLength) {
				ret = pClient->DealCommand();
				if (ret < 0) {
					AfxMessageBox("传输文件失败！");
					TRACE("传输文件失败：ret=%d\r\n", ret);
					break;
				}
				fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
				nCount += pClient->GetPacket().strData.size();
			}
		} while (false);  // do while只执行一次，里面的break都会跳转到这里执行后面的关闭操作
		fclose(pFile);
		pClient->CloseSocket();
	}
	m_dlgStatus.ShowWindow(SW_HIDE);
	EndWaitCursor();
	MessageBox(_T("下载完成"), _T("完成"));
}



void CRemoteClientDlg::OnDeleteFile()
{
	int nListSelected = m_list.GetSelectionMark();  // 列表框控件中获取当前选择的项目的索引
	CString strFile = m_list.GetItemText(nListSelected, 0);
	HTREEITEM hSelected = m_tree.GetSelectedItem();
	strFile = GetPath(hSelected) + strFile;
	int ret = SendCommandPacket(9, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	TRACE("ret:%d\r\n", ret);
	if (ret < 0) {
		AfxMessageBox("删除文件失败！");
	}
	LoadFileCurrrent();
}


void CRemoteClientDlg::OnRunFile()
{
	int nListSelected = m_list.GetSelectionMark();  // 列表框控件中获取当前选择的项目的索引
	CString strFile = m_list.GetItemText(nListSelected, 0);
	HTREEITEM hSelected = m_tree.GetSelectedItem();
	strFile = GetPath(hSelected) + strFile;
	int ret = SendCommandPacket(3, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("打开文件失败！");
	}
}


// 实现消息响应函数④
LRESULT CRemoteClientDlg::OnSendPACKET(WPARAM wParam, LPARAM lParam)
{
	int ret = 0;
	int cmd = wParam >> 1;
	switch (cmd)
	{
	case 4:  // 下载文件
		{
			CString strFile = (LPCSTR)lParam;
			// 这里只能接受两个参数，所以要把原先的四个参数，合并到两个参数
			// int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
			ret = SendCommandPacket(cmd, wParam & 1, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
		}
		break;
	case 5:  // 鼠标
		ret = SendCommandPacket(cmd, wParam & 1, (BYTE*)lParam, sizeof(MOUSEEV));
		break;
	case 6:   // 监控屏幕
		ret = SendCommandPacket(cmd, wParam & 1);
		break;
	case 7:  // 锁机
		ret = SendCommandPacket(cmd, wParam & 1);
		break;
	case 8:  // 解锁
		ret = SendCommandPacket(cmd, wParam & 1);
		break;
/*
	case 6:
	case 7:
	case 8:
		ret = SendCommandPacket(cmd, wParam & 1);
		break;
*/
	default:
		ret = -1;
		break;
	}
	return ret;
}



void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);
}
