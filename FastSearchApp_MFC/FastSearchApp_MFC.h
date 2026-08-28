
// FastSearchApp_MFC.h : PROJECT_NAME アプリケーションのメイン ヘッダー ファイルです
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'pch.h' をインクルードしてください"
#endif

#include "resource.h"		// メイン シンボル


// CFastSearchAppMFCApp:
// このクラスの実装については、FastSearchApp_MFC.cpp を参照してください
//

class CFastSearchAppMFCApp : public CWinApp
{
public:
	CFastSearchAppMFCApp();

// オーバーライド
public:
	virtual BOOL InitInstance();

// 実装

	DECLARE_MESSAGE_MAP()
};

extern CFastSearchAppMFCApp theApp;
