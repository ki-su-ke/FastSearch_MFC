#pragma once

#ifdef FILESEARCHENGINE_EXPORTS
	#define ENGINE_API __declspec(dllexport)
#else
	#define ENGINE_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////
// C API定義

typedef void* SearchEngineHandle;

// 1項目分の情報
struct SearchResultItem {
    const wchar_t* filePath;
	unsigned long long fileSize;	// 64ビットファイルサイズ (4GB超対応)
};

// 検索結果をまとめた構造体
struct SearchResults {
	SearchResultItem* items; // 動的配列へのポインタ
	int count;               // 件数
};

// SearchEngineのインスタンスを作成
ENGINE_API SearchEngineHandle	Engine_Create();

// SearchEngineのインデックスを構築
ENGINE_API int					Engine_BuildIndex(SearchEngineHandle handle, const wchar_t* driveLetter);

//　ファイル検索を実行し、ファイル数を返す
ENGINE_API int					Engine_GetFileCount(SearchEngineHandle handle);

// 検索の実行：SearchResults 構造体を返す（配列でやりとりしよう）
ENGINE_API SearchResults Engine_Search(
		SearchEngineHandle handle,
		const wchar_t* keyword,
		const wchar_t* basePath, // 指定された basePath（例: L"C:\Users\Name\Documents"）以下を検索対象にする
		int maxResults
	);
//// 検索の実行：SearchResults 構造体を返す（配列でやりとりしよう）
//ENGINE_API SearchResults Engine_Search(
//		SearchEngineHandle handle,
//		const wchar_t* keyword,
//		const wchar_t* driveLetter,
//		int maxResults
//	);

// メモリ解放用：Engine_Search で渡した items 配列を破棄する
ENGINE_API void Engine_FreeSearchResults(SearchResults* results);


//　SearchEngineのインスタンスを破棄
ENGINE_API void					Engine_Destroy(SearchEngineHandle handle);


#ifdef __cplusplus
}
#endif
