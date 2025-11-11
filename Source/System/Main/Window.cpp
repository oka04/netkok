//*****************************************************************************
//
// ウィンドウ管理クラス
//
// Window.cpp
//
// K_Yamaoka
//
// 2013/03/04
//
// 2014/11/28 エラーメッセージを追加
//
// 2014/12/04 ウィンドウのサイズ調整
//
// 2015/01/07 マウスホイールの回転量取得機能(GetMouseWheelDelta関数)を追加
//
// 2015/01/16 ウィンドウスタイルを定数に変更
//
// 2015/01/16 ウィンドウの幅、高さをAdjustWindowRectを使用して設定する方法に変更
//
// 2016/11/14 メンバ関数(GetInstanceHandle)を追加
//
// 2016/11/15 GetMouseWheelDelta関数、変数m_mouseWheelDeltaを削除
//
//*****************************************************************************

#define WIN32_LEAN_AND_MEAN
#define _USING_V110_SDK71_ 1
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Window.h"

using namespace WindowSetting;

//=============================================================================
// コンストラクタ
//=============================================================================
Window::Window()
	: m_hAppWnd(0)
	, m_hInstance(0)
	, WINDOW_STYLE(WS_CAPTION | WS_SYSMENU)
{

}

//=============================================================================
// ウィンドウクラスの初期化
// 引　数：const HINSTANCE インスタンスハンドル
// 　　　　const int       表示オプション
//=============================================================================
void Window::Initialize(const HINSTANCE hInstance, const int nShowCmd)
{
	//Windowクラス
	WNDCLASS wc;

	wc.style = CS_VREDRAW | CS_HREDRAW;
	wc.hInstance = hInstance;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = WINDOW_CLASS_NAME;
	wc.lpfnWndProc = WindowProcedure;

	//Windowクラスの登録
	if (!RegisterClass(&wc)) {
		throw DxSystemException(DxSystemException::OM_WINDOW_REGIST_ERROR);
	}

	//Windowを中央に配置

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);

	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };

	AdjustWindowRect(&rect, WINDOW_STYLE, false);

	int windowWidth = rect.right - rect.left;

	int windowHeight = rect.bottom - rect.top;

	int windowX = (screenWidth - windowWidth) / 2;

	int windowY = (screenHeight - windowHeight) / 2;

	//Windowの作成
	m_hAppWnd = CreateWindowEx(
		0,                 //拡張スタイル
		WINDOW_CLASS_NAME, //Windowクラス名
		WINDOW_TITLE_NAME, //タイトル名
		WINDOW_STYLE,      //スタイル
		windowX,           //Ｘ座標
		windowY,           //Ｙ座標
		windowWidth,       //幅
		windowHeight,      //高さ
		nullptr,           //親ウィンドウ
		nullptr,           //メニュー
		hInstance,         //ｲﾝｽﾀﾝｽﾊﾝﾄﾞﾙ
		nullptr            //常にnullptr
	);

	if (!m_hAppWnd) {
		throw DxSystemException(DxSystemException::OM_WINDOW_CREATE_ERROR);
	}

	//Windowの更新
	UpdateWindow(m_hAppWnd);

	//Windowの表示
	ShowWindow(m_hAppWnd, nShowCmd);

	//インスタンスハンドルをメンバ変数に保管（DirectInput用）
	m_hInstance = hInstance;
}

//=============================================================================
// メッセージループ
// 戻り値：int 終了コード
// 引　数：Game* ゲームクラスへのポインタ
//=============================================================================
int Window::MessageLoop(Game* pClsGame)
{
	//MSG構造体
	MSG msg;

	//メッセージループ
	for (;;) {

		//メッセージの確認
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {

			//終了処理(QUITﾒｯｾｰｼﾞで終了)
			if (msg.message == WM_QUIT) {
				break;
			}

			//メッセージの翻訳
			TranslateMessage(&msg);

			//メッセージの割り当て
			//ｳｨﾝﾄﾞｳﾌﾟﾛｼｰｼﾞｬへ
			DispatchMessage(&msg);
		}
		else {

			//Windowsからのメッセージが無い時に
			//ゲームのループを動かす

			if (!pClsGame->Run()) {
				DestroyWindow(m_hAppWnd);
			}
		}
	}

	return (int)msg.wParam;
}

//=============================================================================
// Windowハンドルの取得
// 戻り値：const HWND& ウィンドウハンドル（変更不可）
//=============================================================================
const HWND& Window::GetWindowHandle() const
{
	return m_hAppWnd;
}

//=============================================================================
// インスタンスハンドルの取得
// 戻り値：const HINSTANCE& インスタンスハンドル（変更不可）
//=============================================================================
const HINSTANCE& Window::GetInstanceHandle() const
{
	return m_hInstance;
}




//*****************************************************************************
// private関数
//*****************************************************************************


//-----------------------------------------------------------------------------
// ウィンドウプロシージャ
// 戻り値：LRESULT 
// 引　数：HWND   ウィンドウハンドル
// 　　　　UINT   ウィンドウメッセージ
// 　　　　WPARAM パラメータ１
// 　　　　LPARAM パラメータ２
//-----------------------------------------------------------------------------
LRESULT CALLBACK Window::WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#if USE_IMGUI
	//Imgui用ウインドウプロシージャ
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) {
		return true;
	}
#endif

	switch (message) {
	case WM_KEYDOWN:
		break;
	case WM_DESTROY:
		//Quitﾒｯｾｰｼﾞのﾎﾟｽﾃｨﾝｸﾞ
		PostQuitMessage(0);
		break;
	default:
		//どのアプリでも共通する動作に関してはデフォルトの関数が準備されている。
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}






















