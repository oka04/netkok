//*****************************************************************************
//
// ウィンドウ設定用ヘッダ
//
// WindowSetting.h
//
// K_Yamaoka
//
// 2013/05/28
//
// 2014/11/28 MODE_WINDOWを追加
//            WINDOW_CLASS_NAMEを追加
//
//*****************************************************************************

#pragma once

//=============================================================================
// ウィンドウ設定
//=============================================================================
namespace WindowSetting
{
	//ウィンドウモード（falseでフルスクリーンモード）
	const bool MODE_WINDOW = true;

	//ウィンドウのサイズ（CapsViewerで使える解像度を確認すること）
	const int WINDOW_WIDTH = 1920;
	const int WINDOW_HEIGHT = 1080;

	//ウィンドウタイトル名（ゲーム名に変更すること）
	static const char* WINDOW_TITLE_NAME = "SilentMission";

	//ウィンドウクラス名（基本的には変更しないこと）
	static const char* WINDOW_CLASS_NAME = "3DSystemWindow";
}
