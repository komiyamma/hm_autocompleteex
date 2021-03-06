/*
 * Copyright (c) 2017 Akitsugu Komiyama
 * under the Apache License Version 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <vector>
#include <locale>         // std::locale, std::tolower
#include <shlwapi.h>

#include "tstring.h"

using namespace std;

#include "outputdebugstream.h"
#include "autocomletehelptips.h"
#include "sethidemarufile.h"



namespace AutoCompleteHelpTip {

	BINDAUTOCOMPLETEDLL* GetBindingDll() {
		auto filename = GetCurrentFileName();
		for (auto&mo : mapBindAutoCompleteDll) {
			tregex re(mo.strRegexp, regex_constants::icase);
			tcmatch m;
			if (regex_search(filename, m, re)) {
				return &mo;
			}
		}

		return NULL;
	}

	/*----------------------------------------------------------------------------------------------------------------------
	* 入力補完ウィンドウが生成された直後に呼ばれるコールバック関数
	*----------------------------------------------------------------------------------------------------------------------
	* hWnd     : リストボックスウィンドウ
	* filename : ファイル名
	* (リストボックスウィンドウにSendMessageとか投げないこと。計算が狂う)
	*/
	int OnCreate(HWND hWnd, tstring filename) {
		if (mapBindAutoCompleteDll.size() == 0) {
			return FALSE;
		}
		BINDAUTOCOMPLETEDLL *m = GetBindingDll();
		if (!m) {
			return FALSE;
		}

		if (m->cr) {
			OutputDebugStream(_T("★★cr呼び出し"));
			m->cr(hWnd, filename.data());
		}


		OutputDebugStream(_T("★入力補完ウィンドウが生された直後\n"));
		return TRUE;
	}


	/*----------------------------------------------------------------------------------------------------------------------
	 * 入力補完ウィンドウの項目選択が変化した場合に呼ばれるコールバック関数
	 *----------------------------------------------------------------------------------------------------------------------
	 * hWnd                   : リストボックスウィンドウ
	 * iListBoxSelectedIndex  : リストボックスで何番めが選択されているか
	 * strListBoxSelectedItem : 内容
	 * rect                   : ウィンドウの隅っこ４点の座標
	 * (リストボックスウィンドウにSendMessageとか投げないこと。計算が狂う)
	 */
	int OnListBoxSelectedIndexChanged(HWND hWnd, int iListBoxSelectedIndex, tstring strListBoxSelectedItem, RECT rect, int iItemHeight) {
		if (mapBindAutoCompleteDll.size() == 0) {
			return FALSE;
		}

		BINDAUTOCOMPLETEDLL*m = GetBindingDll();
		if (!m) {
			return FALSE;
		}

		if (m->sl) {
			m->sl(hWnd, iListBoxSelectedIndex, strListBoxSelectedItem.data(), iItemHeight);
		}

		OutputDebugStream(_T("★入力補完の選択項目が変更された時「 %d, %s 」\n"), iListBoxSelectedIndex, strListBoxSelectedItem.data());
		return TRUE;
	}


	/*----------------------------------------------------------------------------------------------------------------------
	 * 入力補完ウィンドウが破棄される直前に呼ばれるコールバック関数
	 *----------------------------------------------------------------------------------------------------------------------
	 * hWnd                   : リストボックスウィンドウ
	 * (リストボックスウィンドウにSendMessageとか投げないこと。計算が狂う)
	 */
	int OnDestroy(HWND hWnd) {
		OutputDebugStream(_T("★入力補完ウィンドウが破棄される直前\n"));
		if (mapBindAutoCompleteDll.size() == 0) {
			return FALSE;
		}

		BINDAUTOCOMPLETEDLL *m = GetBindingDll();
		if (!m) {
			return FALSE;
		}

		if (m->dr) {
			OutputDebugStream(_T("★★dr呼び出し"));
			m->dr(hWnd);
		}

		return TRUE;
	}



	/*----------------------------------------------------------------------------------------------------------------------
	 * 入力補完リストに何か特別に追加すべき項目があるのか、必要となる度に問い合わせが来るコールバック関数
	 *----------------------------------------------------------------------------------------------------------------------
	 * original_list : 現在候補となっているオリジナルのリスト
	 * filename      : ファイル名
	 * curtext       : 編集中も含めた現状のテキスト内容(現バージョンでは、機能していない)
	 * cursor_param  : カーソル関連のパラメータ。現バージョンでは機能しない。
	 * 今、この瞬間に「入力補完候補に特別に追加するべきリスト」を返す。入力リストの「２番目」以降に挿入される。
	 * 「curtext」と「cursur_param」は、現在のところ機能しない。(秀丸で取れない)。将来対応予定
	 */

	vector<tstring> OnQueryListBoxCustomAdded(HWND hWnd, vector<tstring>& original_list) {

		OutputDebugStream(_T("★入力補完の特別追加問い合わせ\n"));
		vector<tstring> add_list;

		BINDAUTOCOMPLETEDLL *m = GetBindingDll();

		if (mapBindAutoCompleteDll.size() == 0) {
			return add_list;
		}
		if (!m) {
			return add_list;
		}
		// 無い
		if (!m->ca) {
			return add_list;
		}

		// 以下はDLLにメソッドがある。
		for each(auto s in original_list) {
			OutputDebugStream(_T("一覧:%s\n"), s.data());
		}

		// original_list 自体は全てがこの関数内では当然pin_ptrであるので、そのままポインタを渡してOKである。
		// 「「TCHAR *」の配列」
		size_t org_size = original_list.size();
		AUTOCOMPLETELIST AutoCompleteOriginalList;

		if (org_size > AUTOCOMPLETELIST_MAX_COUNT) {
			AutoCompleteOriginalList.list_size = AUTOCOMPLETELIST_MAX_COUNT;
		}
		else {
			AutoCompleteOriginalList.list_size = org_size;
		}

		for (size_t i = 0; i < AutoCompleteOriginalList.list_size; i++) {
			AutoCompleteOriginalList.string_ptr_list[i] = LPTSTR(original_list[i].data()); // アドレスの格納
			OutputDebugStream(_T("★★格納 %s"), AutoCompleteOriginalList.string_ptr_list[i]);
		}

		OutputDebugStream(_T("★★ca呼び出し"));
			
		AUTOCOMPLETELIST* AutoCompleteAdditionList = m->ca(hWnd, &AutoCompleteOriginalList);
		if (AutoCompleteAdditionList && AutoCompleteAdditionList->string_ptr_list) {
			add_list.clear();
			size_t add_size = AutoCompleteAdditionList->list_size;
				
			for (size_t i = 0; i < add_size; i++) {
				if (AutoCompleteAdditionList->string_ptr_list[i]) {
					add_list.push_back(AutoCompleteAdditionList->string_ptr_list[i]);
				}
			}
		}

		return add_list;
	}

	/*----------------------------------------------------------------------------------------------------------------------
	 * キーが押された時
	 * hWnd            : 対象のウィンドウ
	 * wParam          : 入力されたコードが入っている。
	 *----------------------------------------------------------------------------------------------------------------------
	 */
	int OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam) {
		OutputDebugStream(_T("押されたキー%c"), wParam);
		if (mapBindAutoCompleteDll.size() == 0) {
			return FALSE;
		}

		BINDAUTOCOMPLETEDLL *m = GetBindingDll();
		if (!m) {
			return FALSE;
		}

		if (m->kd) {
			OutputDebugStream(_T("★★kd呼び出し"));
			m->kd(hWnd, wParam);
		}

		return TRUE;
	}
}



vector<BINDAUTOCOMPLETEDLL> mapBindAutoCompleteDll; // 正規表現に対応するdllの登録


intptr_t BindAutoCompleter(TCHAR *szDllFileName, TCHAR *szRegexp) {


	if (!PathFileExists(szDllFileName)) {
		return FALSE;
	}


	// 登録した順番が走査順でもあるので、mapは使えない。vectorで登録する。
	// すでに登録されているかどうかは、「regexp」と「szDllFileName」の組み合わせ
	bool bIsExist = false;
	for (auto m : mapBindAutoCompleteDll) {
		// すでに登録されているなら…
		if (m.strRegexp == szRegexp && m.strDllName == szDllFileName) {
			bIsExist = true;
		}
	}

	// すでに登録されていたら何もしない
	if (bIsExist) {
		return TRUE;
	}


	BINDAUTOCOMPLETEDLL dll; // 全部0でクリアしてること重症

	dll.strRegexp = tstring(szRegexp);
	dll.strDllName = tstring(szDllFileName);

	// dllが無い
	HMODULE hAutoCompMod = LoadLibrary(szDllFileName);
	if (!hAutoCompMod) {
		OutputDebugStream( _T("★★dllが無い") );
		OutputDebugStream(szDllFileName);
		return FALSE;
	}

	PFNCREATE cr = (PFNCREATE)GetProcAddress(hAutoCompMod, "OnCreate");
	if (cr) {
		dll.cr = cr;
	}

	PFNSELECT sl = (PFNSELECT)GetProcAddress(hAutoCompMod, "OnListBoxSelectedIndexChanged");
	if (sl) {
		dll.sl = sl;
	}

	PFNDESTROY dr = (PFNDESTROY)GetProcAddress(hAutoCompMod, "OnDestroy");
	if (dr) {
		dll.dr = dr;
	}

	PFNADDLIST ca = (PFNADDLIST)GetProcAddress(hAutoCompMod, "OnQueryListBoxCustomAdded");
	if (ca) {
		dll.ca = ca;
	}

	PFNKEYDOWN	kd = (PFNKEYDOWN)GetProcAddress(hAutoCompMod, "OnKeyDown");
	if (kd) {
		dll.kd = kd;
	}

	mapBindAutoCompleteDll.push_back(dll);
	return TRUE;
}
