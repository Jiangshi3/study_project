#pragma once
#include "afxdialogex.h"


// CLockinfoDialog 对话框

class CLockinfoDialog : public CDialog
{
	DECLARE_DYNAMIC(CLockinfoDialog)

public:
	CLockinfoDialog(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CLockinfoDialog();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_INFO };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
