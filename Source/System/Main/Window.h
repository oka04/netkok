//*****************************************************************************
//
// ウィンドウ管理クラス
//
// Window.h
//
// K_Yamaoka
//
// 2013/03/04
//
// 2015/01/16 ウィンドウスタイルを定数に変更
//
// 2016/11/14 メンバ変数(m_hInstance)、メンバ関数(GetInstanceHandle)を追加
//
// 2016/11/15 変数m_mouseWheelDeltaを削除
//
//*****************************************************************************

#pragma once

#include <Windows.h>
#include <string>

#include "WindowSetting.h"

#include "..\\..\\Game.h"

#include "..\\Engine\\Imgui\\imgui_impl_win32.h"

#ifdef USE_IMGUI
//Imgui用ウインドウプロシージャ
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

class Game;

class Window
{
public:

	//=============================================================================
	// コンストラクタ
	//=============================================================================
	Window();

	//=============================================================================
	// ウィンドウクラスの初期化
	// 引　数：const HINSTANCE インスタンスハンドル
	// 　　　　const int       表示オプション
	//=============================================================================
	void Initialize(const HINSTANCE hInstance, const int nShowCmd);

	//=============================================================================
	// メッセージループ
	// 戻り値：int 終了コード
	// 引　数：Game* ゲームクラスへのポインタ
	//=============================================================================
	int MessageLoop(Game* pClsGame);

	//=============================================================================
	// Windowハンドルの取得
	// 戻り値：const HWND& ウィンドウハンドル（変更不可）
	//=============================================================================
	const HWND& GetWindowHandle() const;

	//=============================================================================
	// インスタンスハンドルの取得
	// 戻り値：const HINSTANCE& インスタンスハンドル（変更不可）
	//=============================================================================
	const HINSTANCE& GetInstanceHandle() const;

private:

	//Windowハンドル
	HWND m_hAppWnd;

	//アプリケーションのインスタンスハンドル
	HINSTANCE m_hInstance;

	//ウィンドウスタイル
	const DWORD WINDOW_STYLE;

	//-----------------------------------------------------------------------------
	// ウィンドウプロシージャ
	// 戻り値：LRESULT 
	// 引　数：HWND   ウィンドウハンドル
	// 　　　　UINT   ウィンドウメッセージ
	// 　　　　WPARAM パラメータ１
	// 　　　　LPARAM パラメータ２
	//-----------------------------------------------------------------------------
	static LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	//コピー防止
	Window& operator=(const Window&);
	Window(const Window&);
};



















