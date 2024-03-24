
// RemoteClientDlg.h: 头文件
//

#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"  


// #define WM_SEND_PACKET (WM_USER + 1)  // 自定义发送数据包的消息ID，依次往WM_USER后面加①

// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
// 构造
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
	
public:
	CImage& GetImage() {
		return m_image;
	}

private:
	CImage m_image; // 缓存

private:
	//static void threadEntryWatchData(void* arg);  // 静态成员函数没有this指针，不能访问成员变量、方法  【已删除这个线程】
	//void threadWatchData();  // 成员函数可以使用this指针                                               【已删除】
	// 线程函数
	//static void threadEntryDownFile(void* arg);   【已删除这个线程】
	//void threadDownFile();                        【已删除】
	// 用于删除文件后更新m_list
	void LoadFileCurrrent();
	void LoadFileInfo();
	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);

// 实现
protected:
	CStatusDlg m_dlgStatus;  // TODO 要删除吗？
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnTest();
	// 服务器的ip地址
	DWORD m_server_address;
	// 端口
	CString m_nPort;
	afx_msg void OnBnClickedBtnFileinfo();
	CTreeCtrl m_tree;
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	// 显示文件
	CListCtrl m_list;
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDownloadFile();
	afx_msg void OnDeleteFile();
	afx_msg void OnRunFile();

	// afx_msg LRESULT OnSendPACKET(WPARAM wParam, LPARAM lParam);  // 定义自定义消息的响应函数②
	afx_msg void OnBnClickedBtnStartWatch();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeEditPort();
};
