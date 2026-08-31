#pragma once
#include <windows.h>

#define WMU_INDEX_PROGRESS (WM_USER + 101)  // 進捗通知 (WPARAM: 進捗率0-100等)
#define WMU_INDEX_COMPLETE (WM_USER + 102)  // 完了通知
