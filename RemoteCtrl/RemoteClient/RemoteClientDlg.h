
// RemoteClientDlg.h: 头文件
//

#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"  

#define WM_SEND_PACKET (WM_USER + 1)  // 自定义发送数据包的消息ID，依次往WM_USER后面加①

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

	bool isFull() const {
		return m_isFull;
	}
	CImage& GetImage() {
		return m_image;
	}
	void SetImageStatus(bool isFull = false) {
		m_isFull = isFull;
	}
private:
	CImage m_image; // 缓存
	bool m_isFull;  // 缓存是否有数据； true:有数据； false:没有数据
private:
	static void threadEntryWatchData(void* arg);  // 静态成员函数没有this指针，不能访问成员变量、方法
	void threadWatchData();  // 成员函数可以使用this指针
	// 线程函数
	static void threadEntryDownFile(void* arg); 
	void threadDownFile();
	// 用于删除文件后更新m_list
	void LoadFileCurrrent();
	void LoadFileInfo();
	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);
	// 1查看磁盘分区
	// 2查看指定目录下的文件
	// 3运行文件
	// 4下载文件
	// 5鼠标操作
	// 6发送屏幕内容
	// 7锁机
	// 8解锁
	// 9删除文件
	// 1981测试连接
	// bAutoClose说明：在处理接受文件目录时，可能会接受多个，不能接受一个就close连接，通过此参数来判断关闭时机
	int SendCommandPacket(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0);

// 实现
protected:
	CStatusDlg m_dlgStatus;
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

	afx_msg LRESULT OnSendPACKET(WPARAM wParam, LPARAM lParam);  // 定义自定义消息的响应函数②
	afx_msg void OnBnClickedBtnStartWatch();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
