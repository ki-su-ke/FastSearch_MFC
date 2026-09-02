// CSplashDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "FastSearchApp_MFC.h"
#include "afxdialogex.h"
#include "CSplashDlg.h"

#include "CustomMessages.h"


// CSplashDlg ダイアログ

IMPLEMENT_DYNAMIC(CSplashDlg, CDialogEx)

CSplashDlg::CSplashDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CSplashDlg, pParent)
{

}

CSplashDlg::~CSplashDlg()
{
}

void CSplashDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS1, m_progressBar);
	DDX_Control(pDX, IDC_STATIC1, m_statusText);
}


BEGIN_MESSAGE_MAP(CSplashDlg, CDialogEx)
	// カスタムメッセージハンドラのマッピング
	ON_MESSAGE(WMU_INDEX_PROGRESS, &CSplashDlg::OnIndexProgress)	// プログレスバーの進捗通知
END_MESSAGE_MAP()


// CSplashDlg メッセージ ハンドラー

BOOL CSplashDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO: ここに初期化を追加してください
	//
	// プログレスバーの範囲設定 (0 ～ 100%)
	m_progressBar.SetRange(0, 100);
	m_progressBar.SetPos(0);
	//
	// ステータス表示の初期化
	m_statusText.SetWindowText(L"USNジャーナルをスキャン中...");

	return TRUE;  // return TRUE unless you set the focus to a control
	// 例外 : OCX プロパティ ページは必ず FALSE を返します。
}


// 進捗通知を受け取ってプログレスバーを動かす
LRESULT CSplashDlg::OnIndexProgress(WPARAM wParam, LPARAM lParam)
{
    int nProgress = static_cast<int>(wParam);
    m_progressBar.SetPos(nProgress);

    if (lParam != 0) {
        wchar_t* pStatusMsg = reinterpret_cast<wchar_t*>(lParam);
		m_statusText.SetWindowText(pStatusMsg);
		//delete[] pStatusMsg;	// ここは解放してやる必要がある -> 固定文字列渡すようにしたので不要
    }

    return 0;
}
