
// FastSearchApp_MFCDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "framework.h"
#include "FastSearchApp_MFC.h"
#include "FastSearchApp_MFCDlg.h"
#include "afxdialogex.h"

#include "shellapi.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif



/////////////////////////////////////////////////////////////////////////////
// アプリケーションのバージョン情報に使われる CAboutDlg ダイアログ
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

// 実装
protected:
	DECLARE_MESSAGE_MAP()
public:
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


/////////////////////////////////////////////////////////////////////
// Sort で使用する比較関数
// CListCtrl::SortItems から呼ばれる
int CALLBACK CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
	//
	// lParam に設定した文字列ポインタを取り出す
	const wchar_t* pStr1 = reinterpret_cast<const wchar_t*>(lParam1);
	const wchar_t* pStr2 = reinterpret_cast<const wchar_t*>(lParam2);
	SortContext* pContext = reinterpret_cast<SortContext*>(lParamSort);

	if (!pStr1 || !pStr2 || !pContext)
		return 0;

	// 大文字小文字を無視して比較
	int result = _wcsicmp(pStr1, pStr2);

	// 降順の場合は結果を反転
	return pContext->bAscending ? result : -result;
}

////////////////////////////////////////////////////////////////////
// CFastSearchAppMFCDlg ダイアログ

CFastSearchAppMFCDlg::CFastSearchAppMFCDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_FASTSEARCHAPP_MFC_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CFastSearchAppMFCDlg::~CFastSearchAppMFCDlg()
{
	ReleaseSearchEngine();
}


// 検索エンジンの解放
void CFastSearchAppMFCDlg::ReleaseSearchEngine()
{
	if (m_hSearchEngine) {
		Engine_Destroy(m_hSearchEngine);
		m_hSearchEngine = nullptr;
	}
}

void CFastSearchAppMFCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_TARGET_DIR, m_cmbTargetDir);
	DDX_Control(pDX, IDC_BUTTON_BROWSE, m_btnBrowse);
	DDX_Control(pDX, IDC_EDIT_KEYWORD, m_editKeyword);
	DDX_Control(pDX, IDC_BUTTON_SEARCH, m_btnSearch);
	DDX_Control(pDX, IDC_LIST_SEARCHRESULT, m_listSearchResult);
}

BEGIN_MESSAGE_MAP(CFastSearchAppMFCDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_CBN_SELCHANGE(IDC_COMBO_TARGET_DIR, &CFastSearchAppMFCDlg::OnCbnSelchangeCombo1)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CFastSearchAppMFCDlg::OnBnClickedButtonBrowse)
	ON_BN_CLICKED(IDC_BUTTON_SEARCH, &CFastSearchAppMFCDlg::OnBnClickedButtonSearch)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_SEARCHRESULT, &CFastSearchAppMFCDlg::OnNMDblclkListSearchresult)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_SEARCHRESULT, &CFastSearchAppMFCDlg::OnNMRClickListSearchresult)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_SEARCHRESULT, &CFastSearchAppMFCDlg::OnLvnColumnclickListSearchresult)
END_MESSAGE_MAP()


// CFastSearchAppMFCDlg メッセージ ハンドラー

BOOL CFastSearchAppMFCDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// "バージョン情報..." メニューをシステム メニューに追加します。

	// IDM_ABOUTBOX は、システム コマンドの範囲内になければなりません。
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

	// このダイアログのアイコンを設定します。アプリケーションのメイン ウィンドウがダイアログでない場合、
	//  Framework は、この設定を自動的に行います。
	SetIcon(m_hIcon, TRUE);			// 大きいアイコンの設定
	SetIcon(m_hIcon, FALSE);		// 小さいアイコンの設定

	// TODO: 初期化をここに追加します。
	InitControls(); // コントロールの初期化
	InitSearchEngine(); // 検索エンジンの初期化

	return TRUE;  // フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}

void CFastSearchAppMFCDlg::InitControls()
{
	// リストコントロールの拡張スタイルとカラム（列）設定
	m_listSearchResult.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	//m_listSearchResult.InsertColumn(0, L"ファイル名", LVCFMT_LEFT, 200);
	//m_listSearchResult.InsertColumn(1, L"ファイルパス", LVCFMT_RIGHT, 450);
	m_listSearchResult.InsertColumn(0, L"ファイルパス", LVCFMT_LEFT, 550);
	m_listSearchResult.InsertColumn(1, L"ファイルサイズ", LVCFMT_RIGHT, 100);

	// ドライブコンボボックスの初期化（ひとまず C: ドライブをセット）
	m_cmbTargetDir.AddString(L"C:");
	m_cmbTargetDir.SetCurSel(0);
}

void CFastSearchAppMFCDlg::InitSearchEngine()
{
	// 検索エンジンの初期化
	m_hSearchEngine = Engine_Create();
	if (m_hSearchEngine) {
		// ※まずはCドライブをインデックス化
		// USN Jounal を使ったインデックス構築は速いため、フォルダ毎に構築するのではなく、
		// ドライブ単位で構築するのが効率的となる
		Engine_BuildIndex(m_hSearchEngine, L"C:");
	}
	else {
		AfxMessageBox(_T("検索エンジンの初期化に失敗しました。"));
	}
}

void CFastSearchAppMFCDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// ダイアログに最小化ボタンを追加する場合、アイコンを描画するための
//  下のコードが必要です。ドキュメント/ビュー モデルを使う MFC アプリケーションの場合、
//  これは、Framework によって自動的に設定されます。

void CFastSearchAppMFCDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 描画のデバイス コンテキスト

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// クライアントの四角形領域内の中央
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// アイコンの描画
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// ユーザーが最小化したウィンドウをドラッグしているときに表示するカーソルを取得するために、
//  システムがこの関数を呼び出します。
HCURSOR CFastSearchAppMFCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CFastSearchAppMFCDlg::OnCbnSelchangeCombo1()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
}

void CFastSearchAppMFCDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: ここにメッセージ ハンドラー コードを追加します。
	ReleaseSearchEngine();
}

// 参照ボタンクリック
void CFastSearchAppMFCDlg::OnBnClickedButtonBrowse()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	// フォルダ選択ダイアログを開く
	CFolderPickerDialog dlg(L"C:\\", 0, this, sizeof(OPENFILENAME));
	if (dlg.DoModal() == IDOK) {

		CString strSelectedPath = dlg.GetPathName(); // 選択されたフォルダのパスを取得
		//
		// 選択されたパスをコンボボックスの先頭に挿入して選択状態にする
		int index = m_cmbTargetDir.InsertString(0, strSelectedPath);
		m_cmbTargetDir.SetCurSel(index);
	}
}

// 検索ボタンクリック
void CFastSearchAppMFCDlg::OnBnClickedButtonSearch()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	if (!m_hSearchEngine)
		return ;

	// UI から入力値を取得
	CString strKeyword, strBasePath;
	m_editKeyword.GetWindowText(strKeyword);
	m_cmbTargetDir.GetWindowText(strBasePath);

	if (strKeyword.IsEmpty() || strBasePath.IsEmpty())
		return ;

	// CListCtrl の描画を一時停止（大量データ挿入時のチラつき防止と高速化）
	m_listSearchResult.SetRedraw(FALSE);
	ClearSearchResults();

	// C++ API 経由で爆速検索を実行 (最大 1,000 件)
	SearchResults results = Engine_Search(
		m_hSearchEngine,
		strKeyword.GetString(),
		strBasePath.GetString(),
		1000
	);

	// 結果を CListCtrl に流し込み
	for (int i = 0; i < results.count; ++i) {
		// 第1列: ファイル名 (検索結果の構造体から取得)
		int index = m_listSearchResult.InsertItem(i, results.items[i].filePath);
		//
		// 第2列: ファイルサイズ
		CString strSize;
		strSize.Format(L"%llu", results.items[i].fileSize);
		m_listSearchResult.SetItemText(index, 1, strSize);
		//
		// ソート用の文字列を確保してベクターに保持する
		// ラムダ構造体のおかげで、普通に new するときみたいに1引数で書ける！
		WcharUniquePtr smartPtr(_wcsdup(results.items[i].filePath));	//	_wcsdup() はwchar_t版malloc() Free_Deleterで解放できるし、ダイアログ終了時に自動で解放もされる

		wchar_t* pRawAddress = smartPtr.get();
		m_sortTexts.push_back(std::move(smartPtr));

		m_listSearchResult.SetItemData(index, reinterpret_cast<LPARAM>(pRawAddress));
	}

	// 使い終わったメモリの解放
	Engine_FreeSearchResults(&results);

	// 描画の再開と更新
	m_listSearchResult.SetRedraw(TRUE);
	m_listSearchResult.Invalidate();
}


void CFastSearchAppMFCDlg::OnNMDblclkListSearchresult(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	*pResult = 0;

	// クリックされた行のインデックスを取得（アイテム以外の場所をクリックした場合は -1）
	int nItem = pNMItemActivate->iItem;
	if (nItem == -1)
		return ;

	// 第1列（インデックス0）からフルパスを取得
	CString strFullPath = m_listSearchResult.GetItemText(nItem, 0);
	if (strFullPath.IsEmpty())
		return ;

	// ShellExecuteEx でファイルまたはフォルダを開く
	SHELLEXECUTEINFO sei = { sizeof(sei) };
	sei.lpVerb = L"open";
	sei.lpFile = strFullPath.GetString();
	sei.nShow = SW_SHOWNORMAL;
	if (!ShellExecuteEx(&sei)) {
		AfxMessageBox(L"ファイルを開けませんでした。");
	}
}

void CFastSearchAppMFCDlg::OnNMRClickListSearchresult(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	*pResult = 0;

	int nItem = pNMItemActivate->iItem;
	if (nItem == -1)
		return ;

	CString strFullPath = m_listSearchResult.GetItemText(nItem, 1);
	if (strFullPath.IsEmpty())
		return ;

	// コンテキストメニューを作成
	CMenu menu;
	menu.CreatePopupMenu();
	menu.AppendMenu(MF_STRING, 1001, L"ファイルの場所を開く(&O)");

	// カーソル位置を取得してメニューを表示
	CPoint pt;
	GetCursorPos(&pt);
	int cmd = menu.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN, pt.x, pt.y, this);

	if (cmd == 1001) {
		// explorer.exe /select,"C:\path\to\file.txt" で親フォルダを開いてファイルをハイライト表示
		CString strParam;
		strParam.Format(L"/select,\"%s\"", strFullPath.GetString());

		ShellExecute(NULL, L"open", L"explorer.exe", strParam.GetString(), NULL, SW_SHOWNORMAL);
	}
}


void CFastSearchAppMFCDlg::OnLvnColumnclickListSearchresult(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	*pResult = 0;

	int nClickedCol = pNMLV->iSubItem;
	//
	// 同じ列をクリックした場合は 昇順 ⇄ 降順 を切り替え
	if (nClickedCol == m_nSortColumn) {
		m_bSortAscending = !m_bSortAscending;
	}
	else {
		m_nSortColumn = nClickedCol;
		m_bSortAscending = true; // 新しい列の場合はまず昇順
	}

	SortContext context;
	context.nColumn = m_nSortColumn;
	context.bAscending = m_bSortAscending;

	// 描画停止してソート実行
	m_listSearchResult.SetRedraw(FALSE);
	m_listSearchResult.SortItems(CompareItems, reinterpret_cast<DWORD_PTR>(&context));
	m_listSearchResult.SetRedraw(TRUE);
	m_listSearchResult.Invalidate();
}

// 検索結果リストをクリア
void CFastSearchAppMFCDlg::ClearSearchResults()
{
	m_listSearchResult.DeleteAllItems();
	m_sortTexts.clear(); // ソート用文字列のベクターもクリア
	// 
	// 検索結果リストのクリアはこの順番で！
	// ソート用のvectorを先にクリアしてしまうと、画面上にアイテムが残っている一瞬の間、
	// CListCtrl の内部データ（LPARAM）が「すでに解放されたメモリ（ゴミデータ）」を指す状態（ダングリングポインタ）になってしまう。
	// その瞬間にユーザーがリストをクリックしたりするとクラッシュする原因になるため、
	// 「画面を消してから、メモリを消す」を徹底するように。
	//　あと、CListCtrlのプロパティでは、「並び変え」を無効にしておくこと。
	// CListCtrlの内部でソートが走ると、vectorがまだ用意されていない状態でLPARAMが参照されてしまうので、クラッシュする。
}
