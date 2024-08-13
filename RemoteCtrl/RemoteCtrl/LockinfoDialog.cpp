// LockinfoDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteCtrl.h"
#include "afxdialogex.h"
#include "LockinfoDialog.h"


// CLockinfoDialog 对话框

IMPLEMENT_DYNAMIC(CLockinfoDialog, CDialog)

CLockinfoDialog::CLockinfoDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG_INFO, pParent)
{

}

CLockinfoDialog::~CLockinfoDialog()
{
}

void CLockinfoDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CLockinfoDialog, CDialog)
END_MESSAGE_MAP()


// CLockinfoDialog 消息处理程序
