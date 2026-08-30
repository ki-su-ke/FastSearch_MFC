#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <unordered_map>

#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")


#define MFT_ROOT 5 // MFTのルートディレクトリの定義値（NTFS仕様上、ルートは5番）


///////////////////////////////////////////////////////////
// MFTレコード情報を保持する構造体
struct FileRecord {
	DWORDLONG dwFileId;      // ファイルID
	DWORDLONG dwParentId;    // 親ディレクトリのID
	std::wstring wsFileName; // ファイル名
	ULONGLONG ullFileSize;   // ファイルサイズ
};

//////////////////////////////////////////////////////////////
// 検索結果として返す構造体
struct SearchResult {
	std::wstring wsFileName;	// ファイル名
	std::wstring wsFullPath;	// フルパス
	FILETIME ftCreationTime;	// 作成日時
	FILETIME ftLastWriteTime;	// 最終更新日時
	ULONGLONG ullFileSize;		// ファイルサイズ
	//
	// コンストラクタを用意しておく
	//
	// デフォルトコンストラクタ
	SearchResult() = default;
	//
	// ムーブ対応コンストラクタ
	SearchResult(std::wstring name, std::wstring path, FILETIME createTime, FILETIME lastWrwiteTime, ULONGLONG fileSize)
		: wsFileName(std::move(name)),
		  wsFullPath(std::move(path)),
		  ftCreationTime(createTime),
		  ftLastWriteTime(lastWrwiteTime),
		  ullFileSize(fileSize) {}
};

////////////////////////////////////////////////////////////
// 属性情報を一元取得するヘルパー構造体
struct FileExtraInfo {
	ULONGLONG ullFileSize = 0;
	FILETIME ftCreationTime = { 0, 0 };
	FILETIME ftLastWriteTime = { 0, 0 };
};


///////////////////////////////////////////////////////////
// FastSearchEngine クラスの定義
class FastSearchEngine {
private:
	std::vector<FileRecord> m_records; // インデックスを保持するベクター

	// FileId から m_records のインデックスを引く高速用Map
	std::unordered_map<DWORDLONG, size_t> m_idToIndexMap;


public:
	//FastSearchEngine() = default;
	FastSearchEngine();
	~FastSearchEngine() = default;

	// 指定したドライブ(ex. L"C:")のUSNジャーナルを一括走査してインデックスを構築する
	bool BuildIndex(const std::wstring& driveLetter);

	// 指定したドライブ(ex. L"C:")のUSNジャーナルを一括走査してインデックスを構築する
	// こっちはログ出力を行うバージョン
	bool BuildIndex_2(const std::wstring& driveLetter);

	// スキャンしたファイル・フォルダ総数を取得
	size_t GetFileCount() const { return m_records.size(); }

	// 部分一致検索メソッド(ドライブ名例： L"C:")
	std::vector<SearchResult> Search(
		const std::wstring& keyword,
		const std::wstring& basePath,
		size_t maxResults = 100
	);

private:
	// フルパスを組み立てる内部ヘルパー
	std::wstring BuildFullPath(
		const FileRecord& record,
		const std::unordered_map<DWORDLONG, size_t>& idToIndexMap,
		const std::wstring& driveLetter
	);

	// フルパス復元用の補助関数
	// MFT レコード構造から親ディレクトリの参照番号（Parent File Reference Number）をループで辿り、
	// C:\Folder\SubFolder\File.ext のようなフルパス文字列を組み立てる
	std::wstring GetFullPath(DWORDLONG dwFileId, const std::wstring& driveLetter);

	// 大文字・小文字を区別しない文字列検索の補助関数
	bool ContainsIgnoreCase(const std::wstring& text, const std::wstring& pattern);
	//
	// ワイルドカード検索用の補助関数
	// 例: "*.txt" のようなワイルドカードパターンにマッチするかどうかを判定する
	bool MatchesPattern(const std::wstring& fileName, const std::wstring& pattern);

	// ファイルの属性情報を取得する補助関数
	FileExtraInfo GetFileExtraInfo(const std::wstring& filePath);
};
