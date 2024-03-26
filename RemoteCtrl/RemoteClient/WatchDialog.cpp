// WatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "WatchDialog.h"
#include "ClientController.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{
	m_isFull = false;
	m_nObjWidth = -1;
	m_nObjHeight = -1;
}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDialog::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDialog::OnBnClickedBtnUnlock)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen)
{	// 800 450    1920 1080    16:9   (本电脑分辨率：2560 1440)
	CRect clientRect;
	if (isScreen)ScreenToClient(&point);  // 全局坐标到客户区域坐标;  将屏幕坐标转换为客户区域坐标。
	// TRACE("x:%d, y:%d\r\n", point.x, point.y);
	// 本地坐标到远程坐标
	m_picture.GetWindowRect(clientRect);
	// TRACE("Width:%d, Height:%d\r\n", clientRect.Width(), clientRect.Height());
	
	//int width0 = clientRect.Width();
	//int height0 = clientRect.Height();
	//int width = 1920, height = 1080;
	//int x = point.x * width / width0;   //TODO
	//int y = point.y * height / height0;
	//return CPoint(x, y);

	// return CPoint(point.x * 2560 / clientRect.Width(), point.y * 1440 / clientRect.Height());
	return CPoint(point.x * m_nObjWidth / clientRect.Width(), point.y * m_nObjHeight / clientRect.Height());
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetTimer(0, 50, NULL);  // 50ms触发一次； nIDEvent:0
	m_isFull = false;  // 初始化缓冲没有数据
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0) {
		if (m_isFull)   // 如果有数据就显示
		{
			CRect rect;
			m_picture.GetWindowRect(rect);
			m_nObjWidth = m_image.GetWidth();
			m_nObjHeight = m_image.GetHeight();
			m_image.StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
			m_picture.InvalidateRect(NULL);
			m_image.Destroy();
			m_isFull = false;
			TRACE("更新图片完成:%d %d\r\n", m_nObjWidth, m_nObjHeight);
		}
	}
	CDialog::OnTimer(nIDEvent);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		TRACE("LeftButtonDown: x:%d, y:%d\r\n", point.x, point.y);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;  // 左键  
		event.nAction = 2;  // 按下

		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*) & event, sizeof(MOUSEEV));  // 是sizeof(MOUSEEV)还是sizeof(event)？
	}
	CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;  // 左键
		event.nAction = 3;  // 放开
		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(MOUSEEV));
	}
	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		TRACE("RightButtonDown: x:%d, y:%d\r\n", point.x, point.y);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;  // 右键 
		event.nAction = 2;  // 按下
		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(MOUSEEV));
	}
	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;  // 右键  
		event.nAction = 3;  // 放开
		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(MOUSEEV));
	}
	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		// 坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		// 封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;  // 左键  
		event.nAction = 1;  // 双击
		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(MOUSEEV));
	}
	CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		// 坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		// 封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;  // 右键  
		event.nAction = 1;  // 双击
		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(MOUSEEV));
	}
	CDialog::OnRButtonDblClk(nFlags, point);
} 


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)  // 这种拿到的是客户端坐标
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		// 封装
		MOUSEEV event;
		event.ptXY = remote;
		// event.nButton = 8;
		// event.nAction = 0; 
		event.nButton = 3;  // 没有按键
		event.nAction = 4;  // 鼠标移动

		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(MOUSEEV));
	}
	CDialog::OnMouseMove(nFlags, point);
}


// 单击
void CWatchDialog::OnStnClickedWatch()
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint point;
		GetCursorPos(&point);  // 获取的是全局屏幕坐标; Screen
		CPoint remote = UserPoint2RemoteScreenPoint(point, true);  // 这里true表示需要转换到客户区坐标
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;  // 左键  
		event.nAction = 0;  // 单击

		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(MOUSEEV));
	}
}


void CWatchDialog::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类

	// CDialog::OnOK();  // 防止监视界面按下回车键后关闭
}


void CWatchDialog::OnBnClickedBtnLock()
{
		CClientController::getInstance()->SendCommandPacket(7);
}

void CWatchDialog::OnBnClickedBtnUnlock()
{
	//CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	//pParent->SendMessage(WM_SEND_PACKET, 8 << 1 | 1);
	CClientController::getInstance()->SendCommandPacket(8);
}
