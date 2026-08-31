#pragma once
#include "afxdialogex.h"


// CSplashDlg ダイアログ

class CSplashDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSplashDlg)

public:
	CSplashDlg(CWnd* pParent = nullptr);   // 標準コンストラクター
	virtual ~CSplashDlg();

// ダイアログ データ
//#ifdef AFX_DESIGN_TIME
//	enum { IDD = IDD_CSplashDlg };
//#endif
	enum { IDD = IDD_CSplashDlg };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	virtual BOOL OnInitDialog();

	// カスタムメッセージハンドラ
	// プログレスバーの進捗通知
	afx_msg LRESULT OnIndexProgress(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
public:
	CProgressCtrl	m_progressBar;
	CStatic			m_statusText;
	
};
