#pragma once

#include <windows.h>
#include <winternl.h>


///////////////////////////////////////////////////////////////
//
// MFTレコードの先頭ヘッダー構造体 (1024バイトの先頭 48/56 バイト)
// ※ Windows API では未定義(非公開ヘッダー)のため自前定義
// ※ Win32APIを呼ばずに、MFTレコードのバイナリを直接解析するための構造体として
typedef struct _FILE_RECORD_HEADER {
    DWORD dwType;               // レコード識別用マジックナンバー
                                // 正常なファイルレコードの場合: 'FILE' (0x454C4946)
                                // 破損/不整合レコードの場合:   'BAAD' (0x44414142)

    WORD wUsaOffset;            // Update Sequence Array (USA) へのバイトオフセット
                                // セクタ破損チェック用データの開始位置

    WORD wUsaCount;             // Update Sequence Array (USA) の要素数 (WORD数)

    ULONGLONG ulLsn;            // Logfile Sequence Number ($LogFile のトランザクションID)

    WORD wSequenceNumber;       // レコード再利用カウンター (ファイル削除・再作成で加算)

    WORD wHardLinkCount;        // このファイルを参照するハードリンク (ディレクトリ指示) の数

    WORD wAttributeOffset;      // 最初の属性 (NTFS_ATTRIBUTE_HEADER) へのバイトオフセット
                                // 例: 通常は 0x38 (56バイト) や 0x30 位置から属性走査を開始する

    WORD wFlags;                // レコードの状態を表すビットフラグ ★最も重要
                                // 0x01 = FILE_RECORD_IN_USE           (レコードが使用中)
                                // 0x02 = FILE_FILE_NAME_INDEX_PRESENT (ディレクトリ/フォルダ)
                                // ※ 0x01 のみ      => 通常ファイル
                                // ※ 0x01 | 0x02 => ディレクトリ (フォルダ)
                                // ※ 0x00        => 削除済み空きレコード

    DWORD dwBytesInUse;         // このMFTレコード内で実際に使用されている有効データのバイト数
                                // 属性ループ時の境界チェック (Out-of-Bounds 予防) に使用

    DWORD dwBytesAllocated;     // このMFTレコード全体に割り当てられているメモリ領域サイズ (通常1024バイト)

    ULONGLONG ulBaseFileRecordSegment;  // 基本レコードへの参照番号 (0 の場合は独立したレコード)
                                        // 属性が多くて単一レコードに収まらず拡張された場合に親レコードIDを指す

    WORD wNextAttributeNumber;   // 次に割り当てられる属性識別ID (内部管理用)

    WORD wPadd;                  // 構造体のメンバ境界 (4バイト/8バイトアライメント) 調整用パディング

    DWORD dwRecordNumber;         // このMFTレコード自体のインデックス番号 (MFT内での通し番号)
    // ※ NTFS 3.1 (Windows XP以降) で追加された領域
} FILE_RECORD_HEADER, * PFILE_RECORD_HEADER;

//スキャン時の主要メンバの使い方チェック表
//Type : 0x454C4946（"FILE"）かチェックして破損データを弾く。
//
//Flags : 0x01 が立っていなければ削除済みデータ。0x02 が立っていればフォルダ。
//
//AttributeOffset : 属性スキャン（0x30:ファイル名や 0x80 : ファイルサイズなど）の開始地点を取得する。
//
//BytesInUse : while ループで属性を辿る際、レコード境界を食み出さないためのストッパーとして利用する。


///////////////////////////////////////////////////////////////

/**********************
MFT（Master File Table）のレコードから 0x80（$DATA 属性） を読み取って、
ファイルサイズ（RealSize）を取得する。

MFTスキャン時にファイルサイズも一緒にメモリ（FileRecord）に乗せておけば、
検索時のディスク access がゼロになり、結果表示が爆速になる。

MFTレコード内での $DATA 属性（0x80）の位置づけ

MFTの1レコード（通常1024バイト）は、以下の構造
    PFILE_RECORD_HEADER（レコード先頭のヘッダー）
    属性（Attribute）の連続
    0x10: $STANDARD_INFORMATION
    0x30: $FILE_NAME（ファイル名・親IDなど）
    0x80: $DATA（ファイルの実体データまたはサイズ情報）  ←ここを探す！
    0xFFFFFFFF（属性領域の終わりを示す終端マーカー）


常駐（Resident）と非常駐（Non-Resident）の違い
    $DATA 属性（0x80）には2つの形式がある。サイズの取り方が少し異なるため、フラグで分岐。

    非常駐（Non-Resident / 通常のファイル）:
        データ本体は別領域にあり、属性ヘッダー内に RealSize（8バイト整数 / 64bit） が直接格納されている。

    常駐（Resident / 数百バイト以下のごく小さなファイル）:
        データ本体がMFTレコード内に直接埋め込まれている。そのため ValueLength（データサイズ / 32bit） がファイルサイズになる。
***********************/

//////////////////////////////////////////////////////////////////////////////////////////////
// MFT 属性ヘッダーの簡略定義
// ※ MFTレコード内で AttributeOffset 以降に連続して配置されるヘッダー情報
typedef struct _NTFS_ATTRIBUTE_HEADER {
    DWORD dwAttributeType;      // 属性の種別コード
                                // 0x10 = $STANDARD_INFORMATION (作成日時・タイムスタンプ等)
                                // 0x30 = $FILE_NAME (ファイル名・親ディレクトリID・属性等)
                                // 0x80 = $DATA (ファイルの実データおよびサイズ情報)
                                // 0x90 = $INDEX_ROOT (ディレクトリのインデックスルート)
                                // 0xA0 = $INDEX_ALLOCATION (ディレクトリの大規模インデックス)
                                // 0xFFFFFFFF = 属性領域の終端マーカー (End Marker)

    DWORD dwLength;             // この属性ヘッダー＋データ本体（またはDataRun）を含めた全バイト長
                                // 次の属性位置へのポインタ加算 (pAttrByte += dwLength) に使用

    BYTE byNonResidentFlag;     // データ本体の格納場所を示すフラグ
                                // 0x00 = 常駐 (Resident): データ本体がこのMFTレコード内に収まっている
                                // 0x01 = 非常駐 (Non-Resident): データが別セクタ (Data Runs) に書き出されている

    BYTE byNameLength;          // 属性名 (ストリーム名) の文字数 (Unicode文字数)
                                // 0 = メインデータストリーム
                                // 1以上 = 代替データストリーム (ADS: 例 file.txt:stream)

    WORD wNameOffset;           // 属性名 (Unicode文字列) へのバイトオフセット (この構造体の先頭からの相対位置)

    WORD wFlags;                // 属性の状態フラグ
                                // 0x0001 = 圧縮 (Compressed)
                                // 0x4000 = 暗号化 (Encrypted)
                                // 0x8000 = スパースファイル (Sparse)

    WORD wAttributeNumber;      // レコード内での属性の一意な識別ID (内部管理用)

    union {
        // ---------------------------------------------------------------------
        // 常駐 (Resident) 属性ヘッダー (byNonResidentFlag == 0 の場合)
        // 数百バイト以下のごく小さなファイルで利用される
        // ---------------------------------------------------------------------
        struct {
            DWORD dwValueLength;    // 常駐データの実サイズ (バイト単位)
                                    // 常駐ファイルの場合は、これがファイルサイズになります

            WORD wValueOffset;      // この属性ヘッダーの先頭からデータ本体へのバイトオフセット

            BYTE byResidentFlags;   // 常駐属性固有のフラグ (0x01 = インデックス化可能 等)

            BYTE byReserved;        // パディング (アライメント調整用)
        } Resident;

        // ---------------------------------------------------------------------
        // 非常駐 (Non-Resident) 属性ヘッダー (byNonResidentFlag == 1 の場合)
        // 通常のファイルで利用される
        // ---------------------------------------------------------------------
        struct {
            ULONGLONG ulStartingVcn;        // 割り当てられている最初の VCN (Virtual Cluster Number)

            ULONGLONG ulLastVcn;            // 割り当てられている最後の VCN

            WORD wDataRunOffset;            // データラン (Data Runs: 物理クラスタ割り当てマップ) へのオフセット

            WORD wCompressionUnit;          // 圧縮ユニットのサイズ (2の乗数表現、0なら非圧縮)

            BYTE byPadding[4];              // 構造体アライメント調整用パディング

            ULONGLONG ulAllocatedSize;      // ディスク上で実際に割り当てられている領域のサイズ
                                            // (クラスタサイズの倍数に切り上げられた値)

            ULONGLONG ulRealSize;           // ファイルの実際のサイズ (バイト単位)
                                            // 非常駐ファイルの場合は、この値を取得します

            ULONGLONG ulInitializedSize;    // 初期化（書き込み）済みの実データサイズ
        } NonResident;
    } Attr;
} NTFS_ATTRIBUTE_HEADER, *PNTFS_ATTRIBUTE_HEADER;


////////////////////////////////////////////////////////////////////////
// MFTレコードからファイルサイズ(RealSize)を解析・抽出する関数
ULONGLONG GetFileSizeFromMFTRecord(PFILE_RECORD_HEADER pRecordHeader);
