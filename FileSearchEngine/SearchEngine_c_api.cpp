#include "pch.h"
#include "SearchEngine_c_api.h"
#include "FastSearchEngine.h"


SearchEngineHandle Engine_Create() {
    return new FastSearchEngine();
}

int Engine_BuildIndex(SearchEngineHandle handle, const wchar_t* driveLetter) {
    if (!handle || !driveLetter)
        return 0;
    auto* engine = static_cast<FastSearchEngine*>(handle);
    //return engine->BuildIndex(driveLetter) ? 1 : 0;
	return engine->BuildIndex_2(driveLetter) ? 1 : 0; // ログ出力バージョンを使用
}

int Engine_GetFileCount(SearchEngineHandle handle) {
    if (!handle)
        return 0;
    return static_cast<int>(static_cast<FastSearchEngine*>(handle)->GetFileCount());
}


// 1つの要素を変換・バッファ確保するヘルパー関数
// Args:
//   dest: SearchResultItem構造体の参照（出力先）
//   src: SearchResult構造体の参照（入力元）
static void ConvertResultItem(SearchResultItem& dest, const SearchResult& src) {
    //
	// フルパスのコピー
    size_t pathLen = src.wsFullPath.length() + 1;
    wchar_t* pathBuf = new wchar_t[pathLen];
    wcscpy_s(pathBuf, pathLen, src.wsFullPath.c_str());
	dest.filePath = pathBuf;
    //
	// ファイル名のコピー
    size_t nameLen = src.wsFileName.length() + 1;
    wchar_t* nameBuf = new wchar_t[nameLen];
    wcscpy_s(nameBuf, nameLen, src.wsFileName.c_str());
	dest.fileName = nameBuf;
    //
	// ファイルサイズ
    dest.fileSize = src.ullFileSize;
    //
	// 作成日時 (FILETIME -> unsigned long long)
    ULARGE_INTEGER ulCreate;
    ulCreate.LowPart = src.ftCreationTime.dwLowDateTime;
    ulCreate.HighPart = src.ftCreationTime.dwHighDateTime;
    dest.creationTime = ulCreate.QuadPart;
    //
    // 更新日時 (FILETIME -> unsigned long long)
    ULARGE_INTEGER ulWrite;
    ulWrite.LowPart = src.ftLastWriteTime.dwLowDateTime;
    ulWrite.HighPart = src.ftLastWriteTime.dwHighDateTime;
	dest.lastWriteTime = ulWrite.QuadPart;
}


// 検索実行：配列を動的確保して返す
SearchResults Engine_Search(
    SearchEngineHandle handle,
    const wchar_t* keyword,
    const wchar_t* basePath,
    int maxResults)
{
    SearchResults apiResult = { nullptr, 0 };
    
    if (!handle || !keyword)
        return apiResult;

    if (!basePath) {
		basePath = L"C:"; // basePathがNULLの場合はCドライブをデフォルトにする
    }

    //// basePath からドライブ文字（例: "C:"）を自動抽出
    //std::wstring pathStr = basePath;
    //if (pathStr.length() < 2 || pathStr[1] != L':')
    //    return apiResult;
    //std::wstring driveLetter = pathStr.substr(0, 2); // "C:" を取得

    //auto* engine = static_cast<FastSearchEngine*>(handle);

    //// ドライブ全体の MFT からキーワード検索
    //std::vector<SearchResult> internalResults = engine->Search(keyword, driveLetter.c_str(), static_cast<size_t>(maxResults));
    //if (internalResults.empty()) {
    //    return apiResult;
    //}
    //
    // やり方を変更 basePath以下に絞り込むようにする
    auto* engine = static_cast<FastSearchEngine*>(handle);
    std::vector<SearchResult> internalResults = engine->Search(keyword, basePath, static_cast<size_t>(maxResults));
    if (internalResults.empty()) {
        return apiResult;
    }


    // 呼び出し側へ渡すための SearchResultItem配列を確保
    int count = static_cast<int>(internalResults.size());
    SearchResultItem* items = new SearchResultItem[count];
    //
	// 内部データを SearchResultItem にコピー
    for(int i = 0; i < count; i++) {
        ConvertResultItem(items[i], internalResults[i]);
        //
		// 以下の内容をConvertResultItem関数にまとめた
        // 
		//size_t len = internalResults[i].wsFullPath.length() + 1;
  //      wchar_t* pathBuf = new wchar_t[len];
  //      wcscpy_s(pathBuf, len, internalResults[i].wsFullPath.c_str());

  //      items[i].filePath = pathBuf;
		//items[i].fileName = internalResults[i].wsFileName.c_str(); // ファイル名を設定
		//items[i].fileSize = internalResults[i].ullFileSize; // ファイルサイズ
  //      //
		//// FILETIME を unsigned long long に変換して格納
  //      ULARGE_INTEGER ulCreate, ulWrite;
  //      ulCreate.LowPart = internalResults[i].ftCreationTime.dwLowDateTime;
  //      ulCreate.HighPart = internalResults[i].ftCreationTime.dwHighDateTime;
  //      items[i].creationTime = ulCreate.QuadPart;

  //      ulWrite.LowPart = internalResults[i].ftLastWriteTime.dwLowDateTime;
  //      ulWrite.HighPart = internalResults[i].ftLastWriteTime.dwHighDateTime;
  //      items[i].lastWriteTime = ulWrite.QuadPart;
    }

    apiResult.items = items;
    apiResult.count = count;

    return apiResult;
}
//
// 解放処理
// SearchResultsデータはこちら側で確保して削除する
void Engine_FreeSearchResults(SearchResults* results) {
    if (results && results->items) {
        for (int i = 0; i < results->count; i++) {
            delete[] results->items[i].filePath;
            delete[] results->items[i].fileName;
        }
        delete[] results->items;
        results->items = nullptr;
        results->count = 0;
    }
}

void Engine_Destroy(SearchEngineHandle handle) {
    if (handle) {
        delete static_cast<FastSearchEngine*>(handle);
    }
}
