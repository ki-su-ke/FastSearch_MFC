
// FastSearchApp_MFCDlg.h : ヘッダー ファイル
//

#pragma once

#include <vector>
#include <memory>

#include "SearchEngine_c_api.h"

/////////////////////////////////////////////////////////////////////////////
// ソート条件を比較関数に渡すための構造体
struct SortContext {
	int nColumn;        // ソート対象の列番号 (0: フルパス, 1: サイズ)
	bool bAscending;    // true: 昇順, false: 降順
};

//////////////////////////////////////////////////////////////////////////////
// CFastSearchAppMFCDlg ダイアログ
class CFastSearchAppMFCDlg : public CDialogEx
{
// コンストラクション
public:
	CFastSearchAppMFCDlg(CWnd* pParent = nullptr);	// 標準コンストラクター
	~CFastSearchAppMFCDlg(); // デストラクター


// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FASTSEARCHAPP_MFC_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV サポート

// メンバ変数
private:
	int m_nSortColumn = 0;        // 現在ソートしている列
	bool m_bSortAscending = true; // 昇順か降順か

	// ソートで利用する文字列はstd::vectorで保持することで、CListCtrl::SortItems()のコールバックで文字列ポインタを安全に渡せるようにする
	// また、wcstrdup()で確保した文字列は、std::free()で解放する必要があるので、FreeDeleterを使ったunique_ptrで管理する。
	// ダイアログが閉じられると、vector のデストラクタが走り、中身の std::unique_ptr がすべてのメモリを一斉に自動で free となるので安心。
	// さらにデリータにラムダ式（構造体）を使うことで、型定義から「呪文」が消えて超スッキリ！
	// ラムダ式を使わない場合は、std::unique_ptr<wchar_t, decltype(&std::free)> のように書く必要があるし、
	// もっと以前の場合は、std::unique_ptr<wchar_t, void(*)(void*)> のように書く必要があった
	// まぁdecltypeの方がいいかもしれないけど。
	struct FreeDeleter {
		void operator()(void* p) const { std::free(p); }
	};
	using WcharUniquePtr = std::unique_ptr<wchar_t, FreeDeleter>;

	std::vector<WcharUniquePtr> m_sortTexts; // ソート用に確保した文字列を保持するベクター


// 実装
public:
	void ClearSearchResults(); // 検索結果リストをクリア

private:
	
	CString FileTimeToString(const FILETIME& ft);	// FILETIME を "YYYY/MM/DD HH:MM:SS" の CString に変換する
	CString Format64BitTime(ULONGLONG ullRawTime);	// 64bit時間を "YYYY/MM/DD HH:MM:SS" の CString に変換する
	// C ABI 境界を超えて呼び出すために、環境に依存しない64bit整数で時間を扱っているので2段階で変換する
	// Format64BitTime() の内部で FileTimeToString() を呼び出して文字列を返す

protected:
	HICON m_hIcon;

	SearchEngineHandle m_hSearchEngine = nullptr; // 検索エンジンハンドル

	void InitControls(); // コントロールの初期化
	void InitSearchEngine(); // 検索エンジンの初期化

	void ReleaseSearchEngine(); // 検索エンジンの解放

	// 生成された、メッセージ割り当て関数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnNMDblclkListSearchresult(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMRClickListSearchresult(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnCbnSelchangeCombo1();
	// ターゲットディレクトリの指定
	CComboBox m_cmbTargetDir;
	// 検索対象ディレクトリ参照ボタン
	CButton m_btnBrowse;
	// 検索キーワード入力Edit
	CEdit m_editKeyword;
	// 検索実行ボタン
	CButton m_btnSearch;
	// 検索結果表示リスト
	CListCtrl m_listSearchResult;
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedButtonBrowse();
	afx_msg void OnBnClickedButtonSearch();
	afx_msg void OnLvnColumnclickListSearchresult(NMHDR* pNMHDR, LRESULT* pResult);
};
