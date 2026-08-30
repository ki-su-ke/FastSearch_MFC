
#include "pch.h"
#include "MFTRecordHelper.h"


// MFTレコードからファイルサイズ(RealSize)を解析・抽出する関数
ULONGLONG GetFileSizeFromMFTRecord(PFILE_RECORD_HEADER pRecordHeader) {
    if (!pRecordHeader)
        return 0;
    //
    // 属性エリアの先頭位置を取得 (AttributeOffset 分ポインタを進める)
    BYTE* pByte = (BYTE*)pRecordHeader + pRecordHeader->wAttributeOffset;
    //
    // レコード内で使用中のサイズ (dwBytesInUse) を超えない範囲で属性を走査
    while (pByte < (BYTE*)pRecordHeader + pRecordHeader->dwBytesInUse) {
        PNTFS_ATTRIBUTE_HEADER pAttr = (PNTFS_ATTRIBUTE_HEADER)pByte;
        //
        // 終端マーカー (0xFFFFFFFF) に達したら終了
		if (pAttr->dwAttributeType == 0xFFFFFFFF)
            break;
        //
		// 0x80 = $DATA 属性（かつ byNameLength == 0 でメインデータストリームのみ対象）
        if (pAttr->dwAttributeType == 0x80 && pAttr->byNameLength == 0) {
            if (pAttr->byNonResidentFlag == 1) {
                // 非常駐 (Non-Resident): 通常のファイルサイズを取得
                return pAttr->Attr.NonResident.ulRealSize;
            }
            else {
                // 常駐 (Resident): 極小ファイルのデータサイズを取得
                return (ULONGLONG)pAttr->Attr.Resident.dwValueLength;
            }
        }
        //
		// 無限ループ防止 (サイズ異常時)
        if (pAttr->dwLength == 0)
            break;
        //
		// 次の属性へ進む
        pByte += pAttr->dwLength;
    }
    
    return 0;   // フォルダや $DATA 属性が存在しない場合は 0
}
