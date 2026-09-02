
// FastSearchApp_MFCDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "framework.h"
#include "afxdialogex.h"
#include "shellapi.h"

#include "FastSearchApp_MFC.h"
#include "FastSearchApp_MFCDlg.h"
#include "CustomMessages.h"


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
int CALLBACK CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
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
// パス文字列からドライブレター（例: "C:"）を抽出するヘルパー Windows依存
std::wstring ExtractDriveLetter(const std::wstring& path)
{
	if (path.length() >= 2 && path[1] == L':') {
		return path.substr(0, 2); // "C:" を切り出す
	}
	return L"C:"; // デフォルトフォールバック
}

// 非同期検索で受け取ったペイロードの文字列を解放するヘルパー関数
void FreePayloadStrings(AsyncSearchResultPayload* payload)
{
	if (!payload)
		return;

	for (auto& item : payload->results) {
		std::free(const_cast<wchar_t*>(item.fileName));
		std::free(const_cast<wchar_t*>(item.filePath));
		item.fileName = nullptr;
		item.filePath = nullptr;
	}
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
	// m_hSearchEngine へのアクセスを保護するために mutex をロック
	std::lock_guard<std::mutex> lock(m_engineMutex);
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
	ON_MESSAGE(WMU_INDEX_COMPLETE, &CFastSearchAppMFCDlg::OnIndexComplete)
	ON_MESSAGE(WMU_SEARCH_COMPLETE, &CFastSearchAppMFCDlg::OnSearchComplete)
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
	//InitSearchEngine(); // 検索エンジンの初期化
	InitSearchEngineAsync(); // 検索エンジンの初期化を非同期で行う
	

	return TRUE;  // フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}

void CFastSearchAppMFCDlg::InitControls()
{
	// リストコントロールの拡張スタイルとカラム（列）設定
	m_listSearchResult.ModifyStyle(LVS_TYPEMASK, LVS_REPORT);
	m_listSearchResult.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	//m_listSearchResult.InsertColumn(0, L"ファイル名", LVCFMT_RIGHT, 100);
	//m_listSearchResult.InsertColumn(1, L"ファイルパス", LVCFMT_LEFT, 550);
	m_listSearchResult.InsertColumn(0, L"ファイル名",     LVCFMT_LEFT,  200);
    m_listSearchResult.InsertColumn(1, L"サイズ",         LVCFMT_RIGHT, 100);
    m_listSearchResult.InsertColumn(2, L"更新日時",       LVCFMT_LEFT,  140);
    m_listSearchResult.InsertColumn(3, L"作成日時",       LVCFMT_LEFT,  140);
    m_listSearchResult.InsertColumn(4, L"フルパス",       LVCFMT_LEFT,  400);
	
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

// 検索エンジンの初期化を非同期で行う
void CFastSearchAppMFCDlg::InitSearchEngineAsync()
{
	// スプラッシュ画面をモードレス (Create) で表示
	m_pSplashDlg = new CSplashDlg(this);
	m_pSplashDlg->Create(CSplashDlg::IDD, this);
	m_pSplashDlg->ShowWindow(SW_SHOW);
	m_pSplashDlg->CenterWindow();

	// メインウィンドウの操作を一時的に無効化（または非表示）
	EnableWindow(FALSE);

	// インデックス構築用ワーカースレッドを起動
	HWND hMainWnd = m_hWnd;
	HWND hSplashWnd = m_pSplashDlg->m_hWnd;

	// jthread で管理し、終了時は OnDestroy で request_stop + join する
	m_initThread = std::jthread([hMainWnd, hSplashWnd](std::stop_token stoken) {
		if (stoken.stop_requested())
			return;

		if (::IsWindow(hSplashWnd)) {
			::PostMessage(hSplashWnd, WMU_INDEX_PROGRESS, 10, (LPARAM)L"検索エンジンを初期化中...");
		}

		SearchEngineHandle hEngine = Engine_Create();
		bool success = false;

		if (!stoken.stop_requested() && hEngine != nullptr) {
			if (::IsWindow(hSplashWnd)) {
				::PostMessage(hSplashWnd, WMU_INDEX_PROGRESS, 30, (LPARAM)L"USNジャーナルをスキャン中...");
			}

			if (!stoken.stop_requested() && Engine_BuildIndex(hEngine, L"C:")) {
				success = true;
				if (::IsWindow(hSplashWnd)) {
					::PostMessage(hSplashWnd, WMU_INDEX_PROGRESS, 100, (LPARAM)L"準備完了");
				}
			}
		}

		if (stoken.stop_requested()) {
			if (hEngine) {
				Engine_Destroy(hEngine);
			}
			return;
		}

		if (!::IsWindow(hMainWnd) || !::PostMessage(hMainWnd, WMU_INDEX_COMPLETE, (WPARAM)(success ? 1 : 0), (LPARAM)hEngine)) {
			if (hEngine) {
				Engine_Destroy(hEngine);
			}
		}
	});
}

// バックグラウンドスレッドから完了通知を受けた時
LRESULT CFastSearchAppMFCDlg::OnIndexComplete(WPARAM wParam, LPARAM lParam)
{
	bool isSuccess = (wParam == 1);
	SearchEngineHandle hEngine = reinterpret_cast<SearchEngineHandle>(lParam);

	// スプラッシュ画面を破棄
	if (m_pSplashDlg) {
		m_pSplashDlg->DestroyWindow();
		delete m_pSplashDlg;
		m_pSplashDlg = nullptr;
	}

	// メインウィンドウを有効化し、フォーカスをコンボへ
	EnableWindow(TRUE);
	m_cmbTargetDir.SetFocus();

	if (isSuccess) {
		std::lock_guard<std::mutex> lock(m_engineMutex);
		m_hSearchEngine = hEngine;
		m_currentLoadDrive = L"C:";
	} else {
		if (hEngine) {
			Engine_Destroy(hEngine); // 失敗時は破棄
		}
		AfxMessageBox(L"USN インデックスの構築に失敗しました。\n管理者権限で実行されているか確認してください。");
	}
	
	return 0;
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
	// エンジン解放前に、エンジンを触る可能性があるワーカーを確実に停止する
	if (m_initThread.joinable()) {
		m_initThread.request_stop();
		m_initThread.join();
	}

	if (m_searchThread.joinable()) {
		m_searchThread.request_stop();
		m_searchThread.join();
	}

	if (m_pSplashDlg) {
		m_pSplashDlg->DestroyWindow();
		delete m_pSplashDlg;
		m_pSplashDlg = nullptr;
	}

	ReleaseSearchEngine();

	CDialogEx::OnDestroy();
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

void CFastSearchAppMFCDlg::DisplaySearchResults(const std::vector<SearchResultItem>& results)
{
	// CListCtrl の描画を一時停止（大量データ挿入時のチラつき防止と高速化）
	m_listSearchResult.SetRedraw(FALSE);
	ClearSearchResults();

	int nItemIndex = 0;
	for (const auto& res : results) {
		//
		// 第1列: ファイル名を挿入
		int nInsertedRow = m_listSearchResult.InsertItem(nItemIndex, res.fileName);
		//
		// 第2列: ファイルサイズ
		CString strSize = FormatFileSize(res.fileSize);
		m_listSearchResult.SetItemText(nInsertedRow, 1, strSize);
		//
		// 第3列: 更新日時
		CString strUpdatedAt = Format64BitTime(res.lastWriteTime);
		m_listSearchResult.SetItemText(nInsertedRow, 2, strUpdatedAt);
		//
		// 第4列: 作成日時
		CString strCreatedAt = Format64BitTime(res.creationTime);
		m_listSearchResult.SetItemText(nInsertedRow, 3, strCreatedAt);
		//
		// 第5列: ファイルフルパス
		m_listSearchResult.SetItemText(nInsertedRow, 4, res.filePath);

		// ソート用の文字列を確保してベクターに保持する
		// ラムダ構造体のおかげで、普通に new するときみたいに1引数で書ける！
		WcharUniquePtr smartPtr(_wcsdup(res.filePath));	//	_wcsdup() はwchar_t版malloc() Free_Deleterで解放できるし、ダイアログ終了時に自動で解放もされる

		wchar_t* pRawAddress = smartPtr.get();
		m_sortTexts.push_back(std::move(smartPtr));

		m_listSearchResult.SetItemData(nInsertedRow, reinterpret_cast<LPARAM>(pRawAddress));
		
		nItemIndex++;
	}

	// 描画ロック解除と更新
	m_listSearchResult.SetRedraw(TRUE);
	m_listSearchResult.Invalidate();
}


/*******************************
// 検索ボタンクリック
// シングルスレッドバージョン
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
		int index = m_listSearchResult.InsertItem(i, results.items[i].fileName);
		//
		// 第2列: ファイルフルパス
		//m_listSearchResult.SetItemText(index, 1, results.items[i].filePath);
		//
		// 第2列: ファイルサイズ
		CString strSize;
		strSize.Format(L"%llu", results.items[i].fileSize);
		m_listSearchResult.SetItemText(index, 1, strSize);
		//
		// 第3列: 更新日時
		//CString strUpdateTime = FileTimeToString(results.items[i].ftLastWriteTime);
		CString strUpdateTime = Format64BitTime(results.items[i].lastWriteTime);
		m_listSearchResult.SetItemText(index, 2, strUpdateTime);
		//
		// 第4列: 作成日時
		CString strCreationTime = Format64BitTime(results.items[i].creationTime);
		m_listSearchResult.SetItemText(index, 3, strCreationTime);
		//
		// 第5列: フルパス
		m_listSearchResult.SetItemText(index, 4, results.items[i].filePath);
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
/******************/

// 検索ボタンクリック
// 非同期バージョン
void CFastSearchAppMFCDlg::OnBnClickedButtonSearch()
{
	if (m_isSearching)
		return;		// 二重検索防止

	// UI から入力値を取得
	CString strKeyword, strSelectedPath;
	m_editKeyword.GetWindowText(strKeyword);
	m_cmbTargetDir.GetWindowText(strSelectedPath); // 選択されたディレクトリパス
	strKeyword.Trim(L" \t　");	// タブ文字と半角/全角スペースを除去
	strSelectedPath.Trim(L" \t　");
	if (strKeyword.IsEmpty() || strSelectedPath.IsEmpty())
		return;

	std::wstring keyword = strKeyword.GetString();
	std::wstring selectedPath = strSelectedPath.GetString();

	// 検索中はボタン無効化
	m_isSearching = true;
	m_btnSearch.EnableWindow(FALSE);

	// 非同期検索の実行
	_AsyncSearch(keyword, selectedPath);
	
}

// 検索ボタンクリックから呼び出されて、検索処理を非同期で行う
void CFastSearchAppMFCDlg::_AsyncSearch(const std::wstring& keyword, const std::wstring& targetPath)
{
	const std::wstring targetDrive = ExtractDriveLetter(targetPath);
	HWND hMainWnd = m_hWnd;

	// ワーカーは UI を直接更新せず、結果は WMU_SEARCH_COMPLETE で UI スレッドへ返す
	m_searchThread = std::jthread([this, hMainWnd, targetDrive, targetPath, keyword](std::stop_token stoken) {
		auto* payload = new AsyncSearchResultPayload();
		bool success = false;

		// stop_token は協調停止。重い処理に入る前に都度確認して中断する
		if (!stoken.stop_requested()) {
			SearchEngineHandle hEngine = nullptr;
			std::wstring loadedDrive;
			//
			// m_hSearchEngine へのアクセスを保護するために mutex をロックしてスナップショットを取得
			//	これで後はローカル値を見てやればよいので、スコープを区切って mutex を解放する
			//	今回は関係ないけど、 mutex をロックしたままm_hSearchEngineを使おうとするとデッドロックとかあるので
			//	今回(OnDestroy()などの停止要求->join->ReleaseSearchEngine()と整合性が取れている場合は特に)はこの形で
			{
				std::lock_guard<std::mutex> lock(m_engineMutex);
				hEngine = m_hSearchEngine;
				loadedDrive = m_currentLoadDrive;
			}

			if (hEngine != nullptr) {
				//
				// ターゲットドライブが現在ロードされているドライブと異なる場合は、インデックスを構築する
				if (loadedDrive != targetDrive) {
					if (Engine_BuildIndex(hEngine, targetDrive.c_str()) == 0) {
						std::lock_guard<std::mutex> lock(m_engineMutex);
						m_currentLoadDrive = targetDrive;
						success = true;
					}
				}
				else {
					success = true;
				}

				if (success && !stoken.stop_requested()) {
					//
					// 検索実行
					SearchResults cResults = Engine_Search(
						hEngine,
						keyword.c_str(),
						targetPath.c_str(),
						1000
					);

					if (cResults.items != nullptr && cResults.count > 0) {
						//
						// 検索結果を payload にコピー
						payload->results.reserve(cResults.count);
						for (int i = 0; i < cResults.count; i++) {
							SearchResultItem item{};
							item.fileName = _wcsdup(cResults.items[i].fileName ? cResults.items[i].fileName : L"");
							item.filePath = _wcsdup(cResults.items[i].filePath ? cResults.items[i].filePath : L"");
							item.fileSize = cResults.items[i].fileSize;
							item.lastWriteTime = cResults.items[i].lastWriteTime;
							item.creationTime = cResults.items[i].creationTime;

							if (!item.fileName || !item.filePath) {
								//
								// ファイル名、パスの中身が空の場合は、メモリを解放して失敗扱いとする
								std::free(const_cast<wchar_t*>(item.fileName));
								std::free(const_cast<wchar_t*>(item.filePath));
								payload->isSuccess = false;
								success = false;
								break;
							}

							payload->results.push_back(item);
						}
					}

					Engine_FreeSearchResults(&cResults);
				}
			}
		}
		// 検索結果の成功フラグを設定
		// 検索が成功しても、stop_requested() が true の場合は失敗扱いにする
		payload->isSuccess = success && !stoken.stop_requested();

		// 受け側ウィンドウが有効なときだけ通知。失敗時はここでpayloadを破棄する
		if (!::IsWindow(hMainWnd) || !::PostMessage(hMainWnd, WMU_SEARCH_COMPLETE, static_cast<WPARAM>(payload->isSuccess ? 1 : 0), reinterpret_cast<LPARAM>(payload))) {
			FreePayloadStrings(payload);
			delete payload;
		}
	});
}


// 検索完了受け取り
// 非同期ワーカーから通知された結果を UI スレッドで受け取り、画面更新と後始末を行う
LRESULT CFastSearchAppMFCDlg::OnSearchComplete(WPARAM wParam, LPARAM lParam)
{
	auto* payload = reinterpret_cast<AsyncSearchResultPayload*>(lParam);

	if (payload) {
		if (payload->isSuccess) {
			// CListCtrlへ結果をセット
			DisplaySearchResults(payload->results);
		}
		else {
			AfxMessageBox(L"検索の実行、または USN インデックスの構築に失敗しました。");
		}

		FreePayloadStrings(payload);
		delete payload;
	}

	// UI の無効化解除
	m_isSearching = false;
	m_btnSearch.EnableWindow(TRUE);
	//m_editKeyword.SetFocus();
	m_btnSearch.SetFocus(); // 検索ボタンにフォーカスを戻す

	return 0;
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


// FILETIME を "YYYY/MM/DD HH:MM:SS" の CString に変換する処理例
CString CFastSearchAppMFCDlg::FileTimeToString(const FILETIME& ft) {

	if (ft.dwLowDateTime == 0 && ft.dwHighDateTime == 0)
		return L"";

	// UTC時間からローカル時間に変換
	FILETIME ftLocal;
	FileTimeToLocalFileTime(&ft, &ftLocal);

	SYSTEMTIME st;
	FileTimeToSystemTime(&ftLocal, &st);

	CString strText;
	//strText.Format(L"%04d/%02d/%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	//
	// 秒はいらないか
	strText.Format(L"%04d/%02d/%02d %02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);

	return strText;
}

// 64bit 整数から FILETIME へ復元して文字列化するヘルパー
CString CFastSearchAppMFCDlg::Format64BitTime(ULONGLONG ullRawTime) {
	if (ullRawTime == 0)
		return L"";

	ULARGE_INTEGER ulTime;
	ulTime.QuadPart = ullRawTime;

	FILETIME ft;
	ft.dwLowDateTime = ulTime.LowPart;
	ft.dwHighDateTime = ulTime.HighPart;

	return FileTimeToString(ft);
}

// ファイルサイズを人間が見やすい形式 (KB, MB) に変換
CString CFastSearchAppMFCDlg::FormatFileSize(ULONGLONG sizeInBytes)
{
	if (sizeInBytes < 1024) {
		CString str;
		str.Format(L"%llu B", sizeInBytes);
		return str;
	}
	//
	// KB, MB, GB, TB などの単位に変換
	wchar_t szBuf[64];
	StrFormatByteSizeW(static_cast<LONGLONG>(sizeInBytes), szBuf, ARRAYSIZE(szBuf));
	return CString(szBuf);
}

