/*
 * Copyright (c) 2017 Akitsugu Komiyama
 * under the Apache License Version 2.0
 */

#pragma once

#include <windows.h>
#include "tstring.h"

using namespace std;

using PFNCREATE  = int(*)(HWND hWnd, LPCTSTR szFileName);
using PFNDESTROY = int(*)(HWND hWnd);
using PFNSELECT  = int(*)(HWND hWnd, int iListBoxSelectedIndex, LPCTSTR szListBoxSelectedItem, int iItemHeight);
using PFNKEYDOWN = int(*)(HWND hWnd, WPARAM wParam);

#define AUTOCOMPLETELIST_MAX_COUNT 1000
struct AUTOCOMPLETELIST {
	LPTSTR string_ptr_list[AUTOCOMPLETELIST_MAX_COUNT];
	size_t list_size;
};
using PFNADDLIST = AUTOCOMPLETELIST* (*) (HWND hWnd, AUTOCOMPLETELIST *pAutoCompleteOriginalList);

namespace AutoCompleteHelpTip {
	// 自動補完時のヘルプチップの準備
	int OnCreate(HWND hWnd, tstring filename);

	// 入力補完時に、補完リストの選択を変更した。(キーボードの上下やマウスなどによって)
	int OnListBoxSelectedIndexChanged(HWND hWnd, int iListBoxSelectedIndex, tstring szListBoxSelectedItem, RECT rect, int iItemHeight);

	// 自動補完時のヘルプチップの破棄
	int OnDestroy(HWND hWnd);

	// 特別にその瞬間に、入力補完に足しこむためのリストを決定する。
	vector<tstring> OnQueryListBoxCustomAdded(HWND hWnd, vector<tstring>& original_list);

	// エディタ部や自動補完部でキーが押された時
	int OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam);
}

//----------------------------------------------------------------------
// DLL関連

struct BINDAUTOCOMPLETEDLL {
	tstring strRegexp;
	tstring strDllName;
	HMODULE hDllModule;
	PFNCREATE  cr;
	PFNDESTROY dr;
	PFNSELECT  sl;
	PFNKEYDOWN kd;
	PFNADDLIST ca;

	BINDAUTOCOMPLETEDLL() {
		hDllModule = NULL;
		cr = NULL;
		dr = NULL;
		sl = NULL;
		kd = NULL;
		ca = NULL;
	}
};

extern vector< BINDAUTOCOMPLETEDLL> mapBindAutoCompleteDll;

