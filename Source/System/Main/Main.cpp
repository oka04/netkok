//*****************************************************************************
//
// プログラムのエントリーポイント
//
// Main.cpp
//
// K_Yamaoka
//
// 2013/03/04
//
// 2014/11/28 エラーメッセージを追加
//
// 2016/11/14 clsGame.Initializeの引数追加
//
//*****************************************************************************

#define WIN32_LEAN_AND_MEAN
#define _USING_V110_SDK71_ 1
#include <winsock2.h>
#include <ws2tcpip.h>

//デバッグ用(メモリリーク検出)
#define _CRTDBG_MAP_ALLOC

#include <crtdbg.h>
#include <Windows.h>

#include "Main.h"

//=============================================================================
// プログラムのエントリーポイント
// 戻り値：int 終了コード
// 引　数：HINSTANCE 現在のインスタンスハンドル
// 　　　　HINSTANCE すでに存在するインスタンスハンドル
// 　　　　LPSTR     コマンドライン文字列の先頭番地
// 　　　　int       表示オプション
//=============================================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR strCmdLine, int nShowCmd)
{
	//メモリリーク検出
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(strCmdLine);

	//ウィンドウクラス
	Window clsWindow;

	//ゲームクラス
	Game clsGame;

	try {

		//ウィンドウクラスの初期化
		try {
			clsWindow.Initialize(hInstance, nShowCmd);
		}
		catch (DxSystemException dxSystemException) {
			dxSystemException.ShowOriginalMessage();
			throw DxSystemException(DxSystemException::OM_WINDOW_INITIALIZE_ERROR);
		}

		//ゲームクラスの初期化
		try {
			clsGame.Initialize(clsWindow.GetWindowHandle(), clsWindow.GetInstanceHandle());
		}
		catch (DxSystemException dxSystemException) {
			dxSystemException.ShowOriginalMessage();
			throw DxSystemException(DxSystemException::OM_GAME_INITIALIZE_ERROR);
		}

	}
	catch (DxSystemException dxSystemException) {

		dxSystemException.ShowOriginalMessage();

		return 0;
	}
	catch (...) {

		DxSystemException(DxSystemException::OM_UNKNOWN_ERROR).ShowOriginalMessage();

		return 0;
	}

	return clsWindow.MessageLoop(&clsGame);
}















