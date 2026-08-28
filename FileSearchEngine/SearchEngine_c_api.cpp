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

    // basePath からドライブ文字（例: "C:"）を自動抽出
    std::wstring pathStr = basePath;
    if (pathStr.length() < 2 || pathStr[1] != L':')
        return apiResult;
    std::wstring driveLetter = pathStr.substr(0, 2); // "C:" を取得

    auto* engine = static_cast<FastSearchEngine*>(handle);

    // ドライブ全体の MFT からキーワード検索
    std::vector<SearchResult> internalResults = engine->Search(keyword, driveLetter.c_str(), static_cast<size_t>(maxResults));
    if (internalResults.empty()) {
        return apiResult;
    }
    //
    // basePath 以下にあるファイルのみに絞り込み (前方一致チェック)
    std::vector<SearchResult> filteredResults;
    filteredResults.reserve(internalResults.size());
    //
    // 末尾の円マーク処理を統一
    std::wstring targetPrefix = pathStr;
    if (!targetPrefix.empty() && targetPrefix.back() != L'\\') {
        targetPrefix += L"\\";
    }
    for (const auto& res : internalResults) {
		// パスの先頭が targetPrefix と一致するかチェック
        //if (res.wsFullPath.compare(0, targetPrefix.length(), targetPrefix) == 0) {
        //
        // より明確にパスの先頭がtargetPrefixと一致という形をとる(targetPrefixが空なら常にTrue)
        if(res.wsFullPath.starts_with(targetPrefix)) {
            filteredResults.push_back(res);
        }
    }

    if(filteredResults.empty())
		return apiResult;
    //-------------

    // 呼び出し側へ渡すための SearchResultItem配列を確保
	int count = static_cast<int>(filteredResults.size());
    SearchResultItem* items = new SearchResultItem[count];
    //
	// 内部データを SearchResultItem にコピー
    for (int i = 0; i < count; i++) {
        size_t len = filteredResults[i].wsFullPath.length() + 1;
        wchar_t* pathBuf = new wchar_t[len];
        wcscpy_s(pathBuf, len, filteredResults[i].wsFullPath.c_str());

        items[i].filePath = pathBuf;
		items[i].fileSize = 0; // ファイルサイズは未実装のため0としておく
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
        for (int i = 0; i < results->count; ++i) {
            delete[] results->items[i].filePath;
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
