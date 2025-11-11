//*****************************************************************************
//
// 入力管理
//
// InputManage.h
//
// K_Yamaoka
//
// 2016/11/14
//
// 2021/01/26 GetPadCrossKeyStateSync関数を追加
//
//*****************************************************************************

#pragma once

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif

#include "..\\Exception\\DxSystemException.h"

#include <dinput.h>
#include <vector>

//ゲームパッド構造体
struct GamePad {
	LPDIRECTINPUTDEVICE8 m_pPadDevice;
	DIPROPRANGE m_padRange;
	DIJOYSTATE m_padState;
	DIJOYSTATE m_padCheck;
};

namespace InputKey {

	//マウス用定数

	enum {
		DIK_LBUTTON,
		DIK_RBUTTON,
		DIK_MBUTTON,
		DIK_WHEEL,
	};

	//パッド用定数

	enum {
		DIK_PADBUTTON1,
		DIK_PADBUTTON2,
		DIK_PADBUTTON3,
		DIK_PADBUTTON4,
		DIK_PADBUTTON5,
		DIK_PADBUTTON6,
		DIK_PADBUTTON7,
		DIK_PADBUTTON8,
		DIK_PADBUTTON9,
		DIK_PADBUTTON10,
		DIK_PADBUTTON11,
		DIK_PADBUTTON12,
		DIK_PADBUTTON13,
		DIK_PADBUTTON14,
		DIK_PADBUTTON15,
		DIK_PADBUTTON16,
		DIK_PADBUTTON17,
		DIK_PADBUTTON18,
		DIK_PADBUTTON19,
		DIK_PADBUTTON20,
		DIK_PADBUTTON21,
		DIK_PADBUTTON22,
		DIK_PADBUTTON23,
		DIK_PADBUTTON24,
		DIK_PADBUTTON25,
		DIK_PADBUTTON26,
		DIK_PADBUTTON27,
		DIK_PADBUTTON28,
		DIK_PADBUTTON29,
		DIK_PADBUTTON30,
		DIK_PADBUTTON31,
		DIK_PADBUTTON32,
		DIK_HORIZONTAL,
		DIK_VERTICAL,
	};
};

class InputManage
{
public:

	//パッドの入力範囲
	static const int PAD_RANGE;

	//=============================================================================
	// メンバの初期化（コンストラクタ）
	//=============================================================================
	InputManage();

	//=============================================================================
	// デストラクタ
	//=============================================================================
	~InputManage();

	//=============================================================================
	// 入力管理の初期化
	// 引　数：const HWND & ウィンドウハンドル
	// 　　　　const HINSTANCE & インスタンスハンドル
	//=============================================================================
	void Initialize(const HWND &hAppWnd, const HINSTANCE &hInstance);

	//=============================================================================
	// キーボードの状態を更新する
	//=============================================================================
	void UpdateKeyboardState();

	//=============================================================================
	// キーの状態を取得する
	// 戻り値：BYTE キーの状態
	// 引　数：const BYTE キーコード (DIK_??)
	//=============================================================================
	BYTE GetKeyboardState(const BYTE keyCode);

	//=============================================================================
	// キーの状態を取得する（一度キーを離さないと押せない）
	// 戻り値：BYTE キーの状態
	// 引　数：const BYTE キーコード (DIK_??)
	//=============================================================================
	BYTE GetKeyboardStateSync(const BYTE keyCode);

	//=============================================================================
	// マウスの状態を更新する
	//=============================================================================
	void UpdateMouseState();

	//=============================================================================
	// マウスの移動量を取得する
	// 戻り値：POINT Ｘ,Ｙの移動量
	//=============================================================================
	POINT GetMouseMove() const;

	//=============================================================================
	// マウスホイールの回転量を取得
	// 戻り値：マウスホイールの回転量
	// 備　考：回転方向↑　正の値を返す
	// 　　　　回転方向↓　負の値を返す
	// 　　　　回転していない場合は０を返す
	//=============================================================================
	int GetMouseWheelDelta() const;

	//=============================================================================
	// マウスのボタンの状態を取得する
	// 戻り値：BYTE ボタンの状態
	// 引　数：const BYTE キーコード
	// 　　　　列挙体として(DIK_LBUTTON, DIK_RBUTTON, DIK_MBUTTON)を用意してある
	//=============================================================================
	BYTE GetMouseState(const BYTE keyCode);

	//=============================================================================
	// マウスのボタンの状態を取得する（一度キーを離さないと押せない）
	// 戻り値：BYTE ボタンの状態
	// 引　数：const BYTE キーコード
	// 　　　　列挙体として(DIK_LBUTTON, DIK_RBUTTON, DIK_MBUTTON)を用意してある
	//=============================================================================
	BYTE GetMouseStateSync(const BYTE keyCode);

	//=============================================================================
	// パッドが接続されているか確認する
	// 戻り値：true 接続済み、false 接続されていない
	//=============================================================================
	bool IsConnectGamePad() const;

	//=============================================================================
	// 接続されているパッドの数を取得する
	// 戻り値：int パッドの数
	//=============================================================================
	int GetConnectedGamePadNum() const;

	//=============================================================================
	// パッドの状態を更新する
	//=============================================================================
	void UpdatePadState();

	//=============================================================================
	// パッドの状態を取得
	// 戻り値：DIJOYSTATE パッドの状態
	// 引　数：const unsigned int index ゲームパッド番号（０～）
	//=============================================================================
	DIJOYSTATE GetPadState(const unsigned int index) const;

	//=============================================================================
	// パッドのボタンの状態を取得
	// 戻り値：BYTE ボタンの状態
	// 引　数：const unsigned int ゲームパッド番号（０～）
	// 　　　　const unsigned int ボタン番号（０～３１）
	//=============================================================================
	BYTE GetPadButtonState(const unsigned int idxPad, const unsigned int idxButton) const;

	//=============================================================================
	// パッドのボタンの状態を取得（一度キーを離さないと押せない）
	// 戻り値：BYTE ボタンの状態
	// 引　数：const unsigned int ゲームパッド番号（０～）
	// 　　　　const unsigned int ボタン番号（０～３１）
	//=============================================================================
	BYTE GetPadButtonStateSync(const unsigned int idxPad, const unsigned int idxButton);

	//=============================================================================
	// パッドの十字キーの状態を取得
	// 戻り値：BYTE キーの状態（－１、０、＋１）
	// 引　数：const unsigned int ゲームパッド番号（０～）
	// 　　　　const unsigned int 方向（DIK_HORIZONTAL, DIK_VERTICAL）
	//=============================================================================
	int GetPadCrossKeyState(const unsigned int idxPad, const unsigned int direction) const;

	//=============================================================================
	// パッドの十字キーの状態を取得（一度キーを離さないと押せない）
	// 戻り値：BYTE ボタンの状態
	// 引　数：const unsigned int ゲームパッド番号（０～）
	// 　　　　const unsigned int 方向（DIK_HORIZONTAL, DIK_VERTICAL）
	//=============================================================================
	int InputManage::GetPadCrossKeyStateSync(const unsigned int idxPad, const unsigned int direction);

	//=============================================================================
	// パッドのアナログスティック（左）の状態を取得
	// 戻り値：double ボタンの状態（－１～＋１）
	// 引　数：const unsigned int ゲームパッド番号（０～）
	// 　　　　const unsigned int 方向（DIK_HORIZONTAL, DIK_VERTICAL）
	//=============================================================================
	double GetPadLeftAnalogStickState(const unsigned int idxPad, const unsigned int direction) const;

	//=============================================================================
	// パッドのアナログスティック（右）の状態を取得
	// 戻り値：double ボタンの状態（－１～＋１）
	// 引　数：const unsigned int ゲームパッド番号（０～）
	// 　　　　const unsigned int 方向（DIK_HORIZONTAL, DIK_VERTICAL）
	//=============================================================================
	double GetPadRightAnalogStickState(const unsigned int idxPad, const unsigned int direction) const;

private:

	//DirectInput本体
	LPDIRECTINPUT8 m_pDirectInput;

	//キーボードデバイス
	LPDIRECTINPUTDEVICE8 m_pKeyboardDevice;

	//キーボード用キーバッファ
	enum { KEY_BUFFER = 256 };
	char m_keyBuffer[KEY_BUFFER];
	char m_keyCheck[KEY_BUFFER];

	//マウスデバイス
	LPDIRECTINPUTDEVICE8 m_pMouseDevice;

	//マウス用
	DIMOUSESTATE m_mouseState;
	DIMOUSESTATE m_mouseCheck;

	//ゲームパッド用コンテナ
	std::vector<GamePad> m_vecPad;

	//-----------------------------------------------------------------------------
	// キーボードデバイスの初期化
	// 戻り値：成功 true  失敗 false
	// 引　数：const HWND & ウィンドウハンドル
	//-----------------------------------------------------------------------------
	bool InitializeKeyboard(const HWND &hAppWnd);

	//-----------------------------------------------------------------------------
	// マウスデバイスの初期化
	// 戻り値：成功 true  失敗 false
	// 引　数：const HWND & ウィンドウハンドル
	//-----------------------------------------------------------------------------
	bool InitializeMouse(const HWND &hAppWnd);

	//-----------------------------------------------------------------------------
	// ゲームパッドの初期化
	// 戻り値：成功 true  失敗 false
	// 引　数：const HWND & ウィンドウハンドル
	//-----------------------------------------------------------------------------
	bool InitializePad(const HWND &hAppWnd);

	//-----------------------------------------------------------------------------
	// ゲームパッドの列挙（コールバック関数）
	// 引　数：VOID* InputManageクラスのポインタ
	//-----------------------------------------------------------------------------
	friend static BOOL CALLBACK EnumGamepadCallback(LPDIDEVICEINSTANCE pdidInstance, VOID *pContext);

	//-----------------------------------------------------------------------------
	// ゲームパッドの列挙（コールバック関数）
	// 引　数：VOID* GamePad構造体のポインタ
	//-----------------------------------------------------------------------------
	friend static BOOL CALLBACK EnumAxisCallback(const DIDEVICEOBJECTINSTANCE *pdidObjectInstance, VOID *pContext);

	//-----------------------------------------------------------------------------
	// DirectInputの参照を取得
	// 戻り値：LPDIRECTINPUT8& DirectInputの参照
	//-----------------------------------------------------------------------------
	LPDIRECTINPUT8& GetDirectInput();

	//-----------------------------------------------------------------------------
	// ゲームパッド「ベクタ」の参照を取得
	// 戻り値：std::vector<GamePad>& ゲームパッド「ベクタ」の参照
	//-----------------------------------------------------------------------------
	std::vector<GamePad>& GetPadVector();

	//コピー防止
	InputManage(const InputManage&);
	InputManage& operator=(const InputManage&);
};
