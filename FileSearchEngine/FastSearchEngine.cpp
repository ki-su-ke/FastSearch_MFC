#include "pch.h"
#include <winioctl.h>
#include "FastSearchEngine.h"
#include <vector>
#include <iostream>
#include <algorithm>
#include <cwctype>
#include <numeric>


bool FastSearchEngine::BuildIndex(const std::wstring& driveLetter) {
    m_records.clear();

	// ドライブハンドルの取得 (例: "\\.\C:")
	std::wstring volumePath = L"\\\\.\\" + driveLetter;
    HANDLE hVolume = CreateFileW(
        volumePath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    //
    // 管理者権限が無い場合、hVolumeはINVALID_HANDLE_VALUEとして返ってくる
    if (hVolume == INVALID_HANDLE_VALUE) {
		//　エラーコードを取得してデバッグ用に出力する（必要に応じてコメントアウト）
        DWORD err = GetLastError();
        std::wcout << L"[DLL Debug] CreateFileW Failed! Error Code: " << err << std::endl;
        if (err == ERROR_ACCESS_DENIED) {
            std::wcout << L"[DLL Debug] -> Cause: Administrator privilege required (Run as Admin)!" << std::endl;
        }
        return false;
    }
    //
	// USNジャーナルの情報取得
    USN_JOURNAL_DATA usnData{};
    DWORD bytesReturned = 0;
    if (!DeviceIoControl(hVolume, FSCTL_QUERY_USN_JOURNAL,
            NULL, 0, &usnData, sizeof(usnData), &bytesReturned, NULL)) {
        //
        // ジャーナルが存在しない場合は作成を試みる
        CREATE_USN_JOURNAL_DATA createData{};
        DeviceIoControl(hVolume, FSCTL_CREATE_USN_JOURNAL, &createData,
                            sizeof(createData), NULL, 0, &bytesReturned, NULL);
    }
    //
    // MFT一括読み込みの設定
    MFT_ENUM_DATA enumData{};
    enumData.HighUsn = usnData.NextUsn;
    //
    // 64kb のバッファを確保して高速一括ロード
    constexpr DWORD bufferSize = 64 * 1024;
    std::vector<BYTE> buffer(bufferSize);

    while (DeviceIoControl(hVolume, FSCTL_ENUM_USN_DATA, &enumData,
                sizeof(enumData), buffer.data(), bufferSize, &bytesReturned, NULL)) {
        if (bytesReturned < sizeof(USN))
            break;

        DWORD dwRetBytes = bytesReturned - sizeof(USN);
        PUSN_RECORD record = reinterpret_cast<PUSN_RECORD>(buffer.data() + sizeof(USN));

        while (dwRetBytes > 0) {
            //
            // ファイル名を取得
            std::wstring fileName(record->FileName, record->FileNameLength / sizeof(WCHAR));
            //
            // メモリ上で構築するファイル情報INDEX構造体へ追加
            m_records.push_back({
                record->FileReferenceNumber,
                record->ParentFileReferenceNumber,
                fileName
            });

            DWORD recordLength = record->RecordLength;
            dwRetBytes -= recordLength;
            record = reinterpret_cast<PUSN_RECORD>(reinterpret_cast<PBYTE>(record) + recordLength);
        }

        // 次の読み込み位置を指定
        enumData.StartFileReferenceNumber = *reinterpret_cast<DWORDLONG*>(buffer.data());
    }
    CloseHandle(hVolume);
    return true;
}


//
// 上のBuildIndex()はcount==0となってループ即終了しちゃうので修正&ログ出力バージョンを作成
bool FastSearchEngine::BuildIndex_2(const std::wstring& driveLetter) {
    m_records.clear();

    // 例: L"C:" -> L"\\\\.\\C:"
    std::wstring volumePath = L"\\\\.\\" + driveLetter;

    HANDLE hVolume = CreateFileW(
        volumePath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

	if (hVolume == INVALID_HANDLE_VALUE) {
		DWORD err = GetLastError();
		std::wcout << L"[DLL Debug] CreateFileW Failed. Error Code: " << err << std::endl;
		return false;
	}

	// USNジャーナルの情報取得
	USN_JOURNAL_DATA_V0 usnData{};
    DWORD dwBytesReturned = 0;
    if (!DeviceIoControl(hVolume, FSCTL_QUERY_USN_JOURNAL, NULL, 0, &usnData, sizeof(usnData), &dwBytesReturned, NULL)) {
        DWORD err = GetLastError();
        std::wcout << L"[DLL Debug] Query USN Journal Failed. Error Code: " << err << std::endl;
        CloseHandle(hVolume);
        return false;
    }

    std::wcout << L"[DLL Debug] Journal ID: " << usnData.UsnJournalID << L", NextUsn: " << usnData.NextUsn << std::endl;

	// MFT一括読み込みの設定
    // ★ 確実に全レコードを対象にするため LowUsn = 0, HighUsn = MaxUsn(NextUsn) に設定
	MFT_ENUM_DATA_V0 enumData{};
	enumData.StartFileReferenceNumber = 0;
    enumData.LowUsn = 0;
    enumData.HighUsn = usnData.NextUsn;

	// 64KBのバッファを確保
    constexpr DWORD dwBufferSize = 64 * 1024;
	std::vector<BYTE> buffer(dwBufferSize);

	// MFTの列挙ループ
    int loopCount = 0;
    while(DeviceIoControl(
            hVolume, FSCTL_ENUM_USN_DATA, &enumData, sizeof(enumData),
            buffer.data(), dwBufferSize, &dwBytesReturned, NULL )) {
        loopCount++;

        if (dwBytesReturned <= sizeof(USN)) {
			std::wcout << L"[DLL Debug] Read bytes too small: " << dwBytesReturned << std::endl;
            break;
        }

        DWORD dwRetBytes = dwBytesReturned - sizeof(USN);
        PUSN_RECORD_V2 precord = reinterpret_cast<PUSN_RECORD_V2>(buffer.data() + sizeof(USN));

        while (dwRetBytes > 0) {

			// 無限ループ防止のため、RecordLengthが0の場合はループを抜ける
            DWORD recordLength = precord->RecordLength;
            if (recordLength == 0)
                break;
            

			// USNレコードバージョンチェック (V2 / V3)
            //if (precord->Header.MajorVersion == 2 || precord->Header.MajorVersion == 3) {
            if (precord->MajorVersion == 2 || precord->MajorVersion == 3) {
                //std::wcout << L"[DLL Debug] USN Record MajorVersion: " << precord->MajorVersion << std::endl;
                //
                // ファイル名を取得
                std::wstring fileName(precord->FileName, precord->FileNameLength / sizeof(WCHAR));

                m_records.push_back({
                    precord->FileReferenceNumber,
                    precord->ParentFileReferenceNumber,
                    fileName
                });
            }

            dwRetBytes -= recordLength;
            precord = reinterpret_cast<PUSN_RECORD_V2>(reinterpret_cast<PBYTE>(precord) + recordLength);
        }

		//// 次の読み込み位置（先頭8バイトに格納されている FileReferenceNumber）を更新
  //      DWORDLONG nextFrn = *reinterpret_cast<DWORDLONG*>(buffer.data());
  //      enumData.StartFileReferenceNumber = nextFrn;

        // バッファの先頭 8 バイト（USN）を取得して、次の StartFileReferenceNumber にセット
		USN nextUsn = *reinterpret_cast<USN*>(buffer.data());
        //
        // 次の読み込み FRN が前回と同じ、または終了に達した場合はループを抜ける
        if (nextUsn == enumData.StartFileReferenceNumber) {
            break;
        }
        enumData.StartFileReferenceNumber = nextUsn;
    }

    DWORD lastErr = GetLastError();
    std::wcout << L"[DLL Debug] Loop finished. Total loops: " << loopCount
        << L", Scanned Records: " << m_records.size()
        << L", Last Error Code: " << lastErr << std::endl;
    
    CloseHandle(hVolume);
    return true;
}


/////////////////

//
// 見つけたファイルからフルパスを復元するロジック
std::wstring FastSearchEngine::BuildFullPath(
    const FileRecord& record,
    const std::unordered_map<DWORDLONG, size_t>& idToIndexMap,
    const std::wstring& driveLetter)
{
    std::wstring path = record.wsFileName;
    DWORDLONG currentParentId = record.dwParentId;
    //
	// 親をルートまで遡る（ループ数回程度なので一瞬）
    while (currentParentId != 0) {
        auto it = idToIndexMap.find(currentParentId);
        if (it == idToIndexMap.end())
            break;

        const auto& parentRecord = m_records[it->second];

		// 親のファイル名を先頭に結合
        path = parentRecord.wsFileName + L"\\" + path;

		// ルートディレクトリ（親IDが自分自身または特定の終端）に達したら終了
        if (currentParentId == parentRecord.dwParentId)
            break;

		currentParentId = parentRecord.dwParentId;
    }

    return driveLetter + L"\\" + path;
}

// 検索処理
std::vector<SearchResult> FastSearchEngine::Search(
    const std::wstring& keyword,
    const std::wstring& basePath,
    size_t maxResults)
{
    std::vector<SearchResult> results;
    if (keyword.empty() || basePath.empty())
        return results;

    // basePath の末尾文字を調整 (例: "C:\Users" -> "C:\Users\")
    std::wstring targetPrefix = basePath;
    if(targetPrefix.back() != L'\\') {
		targetPrefix += L"\\";
    }
    //
    // basePath からドライブ部分 (例: "C:") を抽出
	std::wstring drivePrefix = L"C:"; // デフォルト
    if (basePath.length() >= 2 && basePath[1] == L':') {
        drivePrefix = basePath.substr(0, 2);
    }

	results.reserve(std::min<size_t>(maxResults, 1000)); // 事前に最大件数分の領域を確保
    
    // インデックス走査
    for (const auto& record: m_records) {
        //if (ContainsIgnoreCase(record.wsFileName, keyword)) {
        // 
		// ワイルドカード対応の部分一致検索に変更
        if(MatchesPattern(record.wsFileName, keyword)) {
            //
            // フルパスを復元
			std::wstring fullPath = GetFullPath(record.dwFileId, drivePrefix);
            //
			// 指定フォルダ配下か判定 (前方一致)
            if(fullPath.length() >= targetPrefix.length()) {
                if (_wcsnicmp(fullPath.c_str(), targetPrefix.c_str(), targetPrefix.length()) == 0) {
                    //SearchResult sresult;
					//sresult.wsFileName = record.wsFileName;
					//sresult.wsFullPath = fullPath;
                    //results.push_back(sresult);
                    //
                    // 文字列コピーが頻繁に発生するのでムーブコンストラクタを使う
                    // emplace_back()では一時オブジェクトを作らず、std::move を効かせて直接 vector に追加できる
					// record.wsFileName は std::wstring なのでそのまま渡してムーブ
					// fullPath は GetFullPath で生成されたローカル変数なのでstd::moveを付けてムーブ
                    results.emplace_back(record.wsFileName, std::move(fullPath));

                    if (results.size() >= maxResults) {
						break; // 指定上限件数に達したら即抜ける
                    }
                }
            }
        }
    }

    return results;
}


// 大文字・小文字を区別しない部分一致検索（補助関数）
bool FastSearchEngine::ContainsIgnoreCase(const std::wstring& text, const std::wstring& pattern) {
    if (pattern.empty())
        return true;
    if (text.empty())
        return false;

    auto it = std::search(
        text.begin(), text.end(),
        pattern.begin(), pattern.end(),
        [](wchar_t ch1, wchar_t ch2) {
            return std::tolower(ch1) == std::tolower(ch2);
        }
    );
    return it != text.end();
}

// ワイルドカード（* や ?）に対応した大文字小文字無視のマッチング関数
bool FastSearchEngine::MatchesPattern(const std::wstring& fileName, const std::wstring& pattern) {
    if (pattern.empty())
        return true;
    if (fileName.empty())
        return false;

	// パターンにワイルドカードが含まれているかチェック
    bool hasWildcard = (pattern.find(L'*') != std::wstring::npos) ||
		                (pattern.find(L'?') != std::wstring::npos);
    if (hasWildcard) {
        // 例: "*.txt", "test_*.dll", "doc?.docx" など
        // PathMatchSpecW は大文字小文字を自動で無視してパターンマッチしてくれる
        return PathMatchSpecW(fileName.c_str(), pattern.c_str()) == TRUE;
    }
    else {
        // ワイルドカードがない場合は従来の「部分一致」にするため前後に * を付与
        // 例: "report" -> "*report*" として PathMatchSpecW を呼び出す
        std::wstring wildPattern = L"*" + pattern + L"*";
        return PathMatchSpecW(fileName.c_str(), wildPattern.c_str()) == TRUE;
    }
}

// 指定された File Reference Number (FRN) からルートまでのフルパスを復元する
std::wstring FastSearchEngine::GetFullPath(DWORDLONG dwFileId, const std::wstring& driveLetter) {
    //
    // パス要素を一時的に保持する配列 (例: ["file.txt", "SubFolder", "Folder"])
    std::vector<std::wstring_view> pathComponents;
	pathComponents.reserve(10); // 一般的な深さ分を事前に領域確保

    DWORDLONG currentId = dwFileId;

	// ルート（MFT 5番 = ドライブ直下 これは定義値らしい）または親が存在しなくなるまで MFT テーブルを遡る
    while (currentId != 0) {
        //
		// ID から m_records のインデックスを検索
        auto it = m_idToIndexMap.find(currentId);
        if (it == m_idToIndexMap.end())
            break;
        //
		// vector から直接レコードを取得 (O(1) アクセス)
        const FileRecord& record = m_records[it->second];
        if (!record.wsFileName.empty()) {
            pathComponents.push_back(record.wsFileName);
        }
        //
		// ルート に到達したら終了
        if (currentId == MFT_ROOT || record.dwParentId == currentId)
            break;

        currentId = record.dwParentId;
    }

    if (pathComponents.empty())
        return L"";

	// ドライブ文字を先頭にパスを構築
	std::wstring fullPath = driveLetter; // 例: L"C:"
    if (fullPath.empty() || fullPath.back() != L'\\') {
		fullPath += L"\\";
    }

    for (auto it = pathComponents.rbegin(); it != pathComponents.rend(); ++it) {
        if (*it == L"$" || *it == L".")
            continue;

        fullPath += *it;
        if (it + 1 != pathComponents.rend()) {
			fullPath += L"\\";
        }
    }

    return fullPath;
}

