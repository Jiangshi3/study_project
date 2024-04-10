
// RemoteClientDlg.cpp: 实现文件

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "WatchDialog.h"
// #include "ClientSocket.h"
#include "ClientController.h"

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
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CRemoteClientDlg::OnSendPackAck)
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
	InitUIData();
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
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1981);
}

void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	std::list<CPacket> lstPackets;
	bool ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1, true, NULL, 0);
	if (ret == 0) {
		AfxMessageBox("命令处理失败！");
		return;
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
	int nCmd = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	CClientSocket* pClient = CClientSocket::getInstance();  // TODO
	PFILEINFO pInfo = (PFILEINFO)pClient->GetPacket().strData.c_str();
	while (pInfo->HasNext) {
		TRACE("[%s] isdir: %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (pInfo->IsDirectory==FALSE) {
			m_list.InsertItem(0, pInfo->szFileName);
		}
		int cmd = CClientController::getInstance()->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0)break;
		pInfo = (PFILEINFO)pClient->GetPacket().strData.c_str();
	}
	// CClientController::getInstance()->CloseSocket();
}


void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_tree.ScreenToClient(&ptMouse);  // 将获取到的鼠标坐标转换为相对于树形控件客户区的坐标。
	HTREEITEM hTreeSelected = m_tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL)  // 如果点击为空白
		return;
	DeleteTreeChildrenItem(hTreeSelected); // 先清空
	m_list.DeleteAllItems();
	CString strPath = GetPath(hTreeSelected);
	CClientController* pCtrl = CClientController::getInstance();
	CClientSocket*pClient = CClientSocket::getInstance();
	TRACE("hTreeSelected:%08X\r\n", hTreeSelected);
	pCtrl->SendCommandPacket(GetSafeHwnd(), 2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength(), (WPARAM)hTreeSelected);
}

void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}

// 单击显示
void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
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
	CClientController::getInstance()->StartWatchScreen();
}

void CRemoteClientDlg::OnDownloadFile()
{
	int nListSelected = m_list.GetSelectionMark();  // 列表框控件中获取当前选择的项目的索引
	CString strFile = m_list.GetItemText(nListSelected, 0);
	HTREEITEM hSelected = m_tree.GetSelectedItem();
	strFile = GetPath(hSelected) + strFile;  // 拿到路径
	int ret = CClientController::getInstance()->DownFile(strFile);
	if (ret != 0) {
		MessageBox(_T("下载失败！"));
		TRACE("下载失败！\r\n");
	}
}

void CRemoteClientDlg::OnDeleteFile()
{
	int nListSelected = m_list.GetSelectionMark();  // 列表框控件中获取当前选择的项目的索引
	CString strFile = m_list.GetItemText(nListSelected, 0);
	HTREEITEM hSelected = m_tree.GetSelectedItem();
	strFile = GetPath(hSelected) + strFile;
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 9, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
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
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 3, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("打开文件失败！");
	}
}


// 实现消息响应函数④
//LRESULT CRemoteClientDlg::OnSendPACKET(WPARAM wParam, LPARAM lParam)
//{
//	int ret = 0;
//	int cmd = wParam >> 1;
//	switch (cmd)
//	{
//	case 4:  // 下载文件
//		{
//			CString strFile = (LPCSTR)lParam;
//			// 这里只能接受两个参数，所以要把原先的四个参数，合并到两个参数
//			// int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
//			ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
//		}
//		break;
//	case 5:  // 鼠标
//		ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1, (BYTE*)lParam, sizeof(MOUSEEV));
//		break;
//	case 6:   // 监控屏幕
//		ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1);
//		break;
//	case 7:  // 锁机
//		ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1);
//		break;
//	case 8:  // 解锁
//		ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1);
//		break;
///*
//	case 6:
//	case 7:
//	case 8:
//		ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1);
//		break;
//*/
//	default:
//		ret = -1;
//		break;
//	}
//	return ret;
//}



void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);
}


void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));

	*pResult = 0;
}

void CRemoteClientDlg::OnEnChangeEditPort()
{
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
}

LRESULT CRemoteClientDlg::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if ((lParam == -1) || (lParam == -2)) {
		// TODO 错误处理
		TRACE("socket is error: %d\r\n", lParam);
	}
	else if (lParam == 1) {
		// 对方关闭了套接字
		TRACE("socket is closed!\r\n");
	}
	else {  // 正常的情况
		if (wParam != NULL) {
			CPacket head = *(CPacket*)wParam;  // CPacket head是局部变量自动销毁；复制；
			delete (CPacket*)wParam;  // 当场销毁
			// 在RemoteDialog中对命令1、2、3、4响应
			DealCommand(head.sCmd, head.strData, lParam);
		}
	}
	return 0;
}

void CRemoteClientDlg::DealCommand(WORD nCmd, const std::string& strData, LPARAM lParam)
{
	switch (nCmd) {
	case 1:  // 查看磁盘分区
		Str2Tree(strData, m_tree);  // "C,D,E"
		break;
	case 2:  // 查看指定目录下的文件
		UpdateFileInfo(*(PFILEINFO)strData.c_str(), (HTREEITEM)lParam);
		break;
	case 3:  // 运行文件
		MessageBox("打开文件完成！", "操作完成", MB_ICONINFORMATION);
		break;
	case 4:  // 下载文件
		UpdateDownloadFile((FILE*)lParam, strData);
		break;
	case 9: // 删除文件
		MessageBox("删除文件完成！", "操作完成", MB_ICONINFORMATION);
		break;
	case 1981:  // 测试连接
		MessageBox("连接测试成功！", "连接成功", MB_ICONINFORMATION);
		break;
	default:
		TRACE("unknow data recevied! %d\r\n", nCmd);
		break;
	}
}


void CRemoteClientDlg::Str2Tree(const std::string& drivers, CTreeCtrl& tree)
{
	std::string dr;
	tree.DeleteAllItems();
	for (int i = 0; i < drivers.size(); i++) {
		if (drivers[i] != ',') {
			dr = drivers[i];
			dr += ":";
			HTREEITEM hTemp = tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			tree.InsertItem(NULL, hTemp, TVI_LAST);  // 在盘符后面添加一个空的子节点
			dr.clear();
		}
	}
}

void CRemoteClientDlg::UpdateFileInfo(const FILEINFO& fInfo, HTREEITEM hParent)
{
	TRACE("HasNext:%d, IsDirectory:%d, szFileName:%s\r\n", fInfo.HasNext, fInfo.IsDirectory, fInfo.szFileName);
	if (fInfo.HasNext == false) {
		return;
	}
	if (fInfo.IsDirectory) {
		if ((CString)(fInfo.szFileName) == "." || (CString)(fInfo.szFileName) == "..")  // 排除这两个目录
		{
			return;
		}
		TRACE("hTreeSelected:%08X; GetSelectedItem:%08X\r\n", hParent, m_tree.GetSelectedItem());
		HTREEITEM hTemp = m_tree.InsertItem(fInfo.szFileName, hParent, TVI_LAST);
		m_tree.InsertItem("", hTemp, TVI_LAST);  // 如果是目录，在后面添加一个空的子节点
		m_tree.Expand(hParent, TVE_EXPAND);  // InsertItem()插入成功后，需要主动展开才能显示
	}
	else {
		m_list.InsertItem(0, fInfo.szFileName);
	}
}

void CRemoteClientDlg::UpdateDownloadFile(FILE* pFile, const std::string& strData)
{
	static LONGLONG length = 0;
	static LONGLONG index = 0;
	TRACE("length:%d, index:%d\r\n", length, index);
	if (length == 0) {  // 处理第一个包获取文件大小； 第一个包里面的数据是文件的大小
		length = *(long long*)strData.c_str();
		if (length == 0) {
			fclose(pFile);
			CClientController::getInstance()->DownloadEnd();
		}
	}
	else if (length > 0 && (index >= length)) {  // 文件结束
		fclose(pFile);
		length = 0;
		index = 0;
		CClientController::getInstance()->DownloadEnd();
	}
	else {  // 写文件
		fwrite(strData.c_str(), 1, strData.size(), pFile);
		index += strData.size();
		TRACE("index=%d\r\n", index);
		if (index >= length) {
			fclose(pFile);
			length = 0;
			index = 0;
			CClientController::getInstance()->DownloadEnd();
		}
	}
}

void CRemoteClientDlg::InitUIData()
{
	// 设置IP地址、端口的初始化
	UpdateData();
	m_server_address = 0x7F000001;  // 127.0.0.1
	// m_server_address = 0xC0A8E780;  //  192.168.231.128
	m_nPort = _T("9527");
	CClientController* pController = CClientController::getInstance();    // 一开始IP和Port也给他初始化上
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
	UpdateData(FALSE);
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);
}

