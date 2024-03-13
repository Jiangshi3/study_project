// WatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "WatchDialog.h"
#include "RemoteClientDlg.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{

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
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen)
{	// 800 450    1920 1080    16:9   (本电脑分辨率：2560 1440)
	CRect clientRect;
	if (isScreen)ScreenToClient(&point);  // 全局坐标到客户区域坐标;  将屏幕坐标转换为客户区域坐标。
	TRACE("x:%d, y:%d\r\n", point.x, point.y);
	// 本地坐标到远程坐标
	m_picture.GetWindowRect(clientRect);  // Width:1800, Height:1069
	// TRACE("Width:%d, Height:%d\r\n", clientRect.Width(), clientRect.Height());
	
	//int width0 = clientRect.Width();
	//int height0 = clientRect.Height();
	//int width = 1920, height = 1080;
	//int x = point.x * width / width0;   //TODO
	//int y = point.y * height / height0;
	//return CPoint(x, y);
	// return CPoint(point.x * 1920 / clientRect.Width(), point.y * 1080 / clientRect.Height());
	return CPoint(point.x * 2560 / clientRect.Width(), point.y * 1440 / clientRect.Height());
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetTimer(0, 50, NULL);  // 50ms触发一次； nIDEvent:0

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0) {
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();  // IDD_DLG_WATCH的父类是IDD_REMOTECLIENT_DIALOG(CRemoteClientDlg)
		if (pParent->isFull())  // 如果有数据就显示
		{
			CRect rect;
			m_picture.GetWindowRect(rect);
			// pParent->GetImage().BitBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, SRCCOPY);
			pParent->GetImage().StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
			m_picture.InvalidateRect(NULL);
			pParent->GetImage().Destroy();
			pParent->SetImageStatus(false);  // 设置m_isFull=false;
		}
	}

	CDialog::OnTimer(nIDEvent);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	TRACE("x:%d, y:%d\r\n", point.x, point.y);
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;  // 左键  
	event.nAction = 2;  // 按下

	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, LPARAM(&event));

	CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;  // 左键
	event.nAction = 3;  // 放开
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, LPARAM(&event));
	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 1;  // 右键 
	event.nAction = 2;  // 按下
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, LPARAM(&event));

	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 1;  // 右键  
	event.nAction = 3;  // 放开
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, LPARAM(&event));

	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// 坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	// 封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;  // 左键  
	event.nAction = 1;  // 双击
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, LPARAM(&event));

	CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// 坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	// 封装
	MOUSEEV event;
	event.ptXY = remote;
	//event.nButton = 2;  // 右键  
	//event.nAction = 2;  // 双击

	event.nButton = 1;  // 右键  
	event.nAction = 1;  // 双击
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, LPARAM(&event));
	CDialog::OnRButtonDblClk(nFlags, point);
} 


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)  // 这种拿到的是客户端坐标
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	// 封装
	MOUSEEV event;
	event.ptXY = remote;
	// event.nButton = 8;
	// event.nAction = 0; 
	event.nButton = 3;  // 没有按键
	event.nAction = 4;  // 鼠标移动

	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, LPARAM(&event));

	CDialog::OnMouseMove(nFlags, point);
}


// 单击
void CWatchDialog::OnStnClickedWatch()
{
	CPoint point;
	GetCursorPos(&point);  // 获取的是全局屏幕坐标; Screen
	CPoint remote = UserPoint2RemoteScreenPoint(point, true);  // 这里true表示需要转换到客户区坐标
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;  // 左键  
	event.nAction = 0;  // 单击
	// 不可以通过这种方式来通信；只能通过SendMessage();
	// CClientSocket* pClient = CClientSocket::getInstance();
	// CPacket pack(5, (BYTE*)&event, sizeof(event));
	// pClient->Send(pack);
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();      
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, LPARAM(&event));  // TODO: 存在一个设计隐患，网络通信和对话框有耦合；
}
