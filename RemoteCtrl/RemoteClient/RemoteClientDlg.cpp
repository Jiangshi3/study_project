
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
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
	// TODO: 在此添加控件通知处理程序代码
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


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


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


void CRemoteClientDlg::OnDownloadFile()
{
	int nListSelected = m_list.GetSelectionMark();  // 列表框控件中获取当前选择的项目的索引
	CString strFile = m_list.GetItemText(nListSelected, 0);
	CFileDialog dlg(false, NULL, strFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, this);  // 创建一个文件对话框对象 dlg
	if (dlg.DoModal() == IDOK)  // 表示用户已经选择了一个文件并点击了对话框的确定按钮
	{
		FILE* pFile = fopen(dlg.GetPathName(), "wb+");
		if (pFile == NULL) {
			AfxMessageBox(_T("本地没有权限保存该文件，或者文件无法创建！"));
			return;
		}
		HTREEITEM hSelected = m_tree.GetSelectedItem();
		strFile = GetPath(hSelected) + strFile;
		TRACE("%s\r\n", (LPCTSTR)strFile);
		CClientSocket* pClient = CClientSocket::getInstance();
		do{
			int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
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
	// TODO 大文件传输额外处理
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
