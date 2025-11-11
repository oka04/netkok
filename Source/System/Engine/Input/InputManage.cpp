//*****************************************************************************
//
// 入力管理
//
// InputManage.cpp
//
// K_Yamaoka
//
// 2016/11/14
//
// 2016/11/25 マウスボタンのassert要因のバグ解消
//
// 2021/01/26 GetPadCrossKeyStateSync関数を追加
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "InputManage.h"

#include <cassert>

//パッドの入力範囲
const int InputManage::PAD_RANGE = 1000;

//=============================================================================
// メンバの初期化（コンストラクタ）
//=============================================================================
InputManage::InputManage()
	: m_pDirectInput(nullptr)
	, m_pKeyboardDevice(nullptr)
	, m_pMouseDevice(nullptr)
{

}

//=============================================================================
// デストラクタ
//=============================================================================
InputManage::~InputManage()
{
	for (unsigned int i = 0; i < m_vecPad.size(); i++) {
		if (m_vecPad[i].m_pPadDevice) {
			m_vecPad[i].m_pPadDevice->Unacquire();
			m_vecPad[i].m_pPadDevice->Release();
			m_vecPad[i].m_pPadDevice = nullptr;
		}
	}

	if (m_pMouseDevice) {
		m_pMouseDevice->Unacquire();
		m_pMouseDevice->Release();
		m_pMouseDevice = nullptr;
	}

	if (m_pKeyboardDevice) {
		m_pKeyboardDevice->Unacquire();
		m_pKeyboardDevice->Release();
		m_pKeyboardDevice = nullptr;
	}

	if (m_pDirectInput) {
		m_pDirectInput->Release();
		m_pDirectInput = nullptr;
	}
}

//=============================================================================
// 入力管理の初期化
// 引　数：const HWND & ウィンドウハンドル
// 　　　　const HINSTANCE & インスタンスハンドル
//=============================================================================
void InputManage::Initialize(const HWND &hAppWnd, const HINSTANCE &hInstance)
{
	//DirectInputの作成
	if (FAILED(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_pDirectInput, nullptr))) {
		throw DxSystemException(DxSystemException::OM_INPUT_DI_CREATE_ERROR);
	}

	//キーボードデバイスの初期化
	if (!InitializeKeyboard(hAppWnd)) {
		throw DxSystemException(DxSystemException::OM_INPUT_INIT_KEYBOARD_ERROR);
	}

	//マウスデバイスの初期化
	if (!InitializeMouse(hAppWnd)) {
		throw DxSystemException(DxSystemException::OM_INPUT_INIT_MOUSE_ERROR);
	}

	//パッドの初期化
	if (!InitializePad(hAppWnd)) {
		throw DxSystemException(DxSystemException::OM_INPUT_INIT_PAD_ERROR);
	}
}

//=============================================================================
// キーボードの状態を更新する
//=============================================================================
void InputManage::UpdateKeyboardState()
{
	if (FAILED(m_pKeyboardDevice->GetDeviceState(sizeof(m_keyBuffer), (void*)&m_keyBuffer))) {
		m_pKeyboardDevice->Acquire();
	}
}

//=============================================================================
// キーの状態を取得する
// 戻り値：BYTE キーの状態
// 引　数：const BYTE キーコード (DIK_??)
//=============================================================================
BYTE InputManage::GetKeyboardState(const BYTE keyCode)
{
	return m_keyBuffer[keyCode];
}

//=============================================================================
// キーの状態を取得する（一度キーを離さないと押せない）
// 戻り値：BYTE キーの状態
// 引　数：const BYTE キーコード (DIK_??)
//=============================================================================
BYTE InputManage::GetKeyboardStateSync(const BYTE keyCode)
{
	if (m_keyBuffer[keyCode]) {
		if (m_keyCheck[keyCode] == 0x00) {
			m_keyCheck[keyCode] = m_keyBuffer[keyCode];
			return m_keyBuffer[keyCode];
		}
		else {
			return 0x00;
		}
	}
	else {
		if (m_keyCheck[keyCode]) {
			m_keyCheck[keyCode] = m_keyBuffer[keyCode];
		}
		return 0x00;
	}
}

//=============================================================================
// マウスの状態を更新する
//=============================================================================
void InputManage::UpdateMouseState()
{
	if (FAILED(m_pMouseDevice->GetDeviceState(sizeof(DIMOUSESTATE), &m_mouseState))) {
		m_pMouseDevice->Acquire();
	}
}

//=============================================================================
// マウスの移動量を取得する
// 戻り値：POINT Ｘ,Ｙの移動量
//=============================================================================
POINT InputManage::GetMouseMove() const
{
	POINT point;

	point.x = m_mouseState.lX;
	point.y = m_mouseState.lY;

	return point;
}

//=============================================================================
// マウスホイールの回転量を取得
// 戻り値：マウスホイールの回転量
// 備　考：回転方向↑　正の値を返す
// 　　　　回転方向↓　負の値を返す
// 　　　　回転していない場合は０を返す
//=============================================================================
int InputManage::GetMouseWheelDelta() const
{
	return m_mouseState.lZ;
}

//=============================================================================
// マウスのボタンの状態を取得する
// 戻り値：BYTE ボタンの状態
// 引　数：const BYTE キーコード
// 　　　　列挙体として(DIK_LBUTTON, DIK_RBUTTON, DIK_MBUTTON)を用意してある
//=============================================================================
BYTE InputManage::GetMouseState(const BYTE keyCode)
{
	assert(keyCode >= 0 && keyCode < sizeof(m_mouseState.rgbButtons) / sizeof(m_mouseState.rgbButtons[0]));

	return m_mouseState.rgbButtons[keyCode];
}

//=============================================================================
// マウスのボタンの状態を取得する（一度キーを離さないと押せない）
// 戻り値：BYTE ボタンの状態
// 引　数：const BYTE キーコード
// 　　　　列挙体として(DIK_LBUTTON, DIK_RBUTTON, DIK_MBUTTON)を用意してある
//=============================================================================
BYTE InputManage::GetMouseStateSync(const BYTE keyCode)
{
	assert(keyCode >= 0 && keyCode < sizeof(m_mouseState.rgbButtons) / sizeof(m_mouseState.rgbButtons[0]));

	if (m_mouseState.rgbButtons[keyCode]) {
		if (m_mouseCheck.rgbButtons[keyCode] == 0x00) {
			m_mouseCheck.rgbButtons[keyCode] = m_mouseState.rgbButtons[keyCode];
			return m_mouseState.rgbButtons[keyCode];
		}
		else {
			return 0x00;
		}
	}
	else {
		if (m_mouseCheck.rgbButtons[keyCode]) {
			m_mouseCheck.rgbButtons[keyCode] = m_mouseState.rgbButtons[keyCode];
		}
		return 0x00;
	}
}

//=============================================================================
// パッドが接続されているか確認する
// 戻り値：true 接続済み、false 接続されていない
//=============================================================================
bool InputManage::IsConnectGamePad() const
{
	if (m_vecPad.size() > 0) {
		return true;
	}

	return false;
}

//=============================================================================
// 接続されているパッドの数を取得する
// 戻り値：int パッドの数
//=============================================================================
int InputManage::GetConnectedGamePadNum() const
{
	return m_vecPad.size();
}

//=============================================================================
// パッドの状態を更新する
//=============================================================================
void InputManage::UpdatePadState()
{
	if (m_vecPad.empty()) {
		return;
	}

	for (unsigned int i = 0; i < m_vecPad.size(); i++) {

		m_vecPad[i].m_pPadDevice->Poll();

		if (FAILED(m_vecPad[i].m_pPadDevice->GetDeviceState(sizeof(DIJOYSTATE), &m_vecPad[i].m_padState))) {
			m_vecPad[i].m_pPadDevice->Acquire();
		}
	}

}

//=============================================================================
// パッドの状態を取得
// 戻り値：DIJOYSTATE パッドの状態
// 引　数：const unsigned int index ゲームパッド番号（０～）
//=============================================================================
DIJOYSTATE InputManage::GetPadState(const unsigned int index) const
{
	if (m_vecPad.empty() || index < 0 || index >= m_vecPad.size()) {
		DIJOYSTATE work = { 0 };
		return work;
	}

	return m_vecPad[index].m_padState;
}

//=============================================================================
// パッドのボタンの状態を取得
// 戻り値：BYTE ボタンの状態
// 引　数：const unsigned int ゲームパッド番号（０～）
// 　　　　const unsigned int ボタン番号（０～３１）
//=============================================================================
BYTE InputManage::GetPadButtonState(const unsigned int idxPad, const unsigned int idxButton) const
{
	if (m_vecPad.empty() || idxPad < 0 || idxPad >= m_vecPad.size() || idxButton < 0 || idxButton >= sizeof(m_vecPad[0].m_padState.rgbButtons) / sizeof(m_vecPad[0].m_padState.rgbButtons[0])) {
		return 0;
	}

	return m_vecPad[idxPad].m_padState.rgbButtons[idxButton];
}

//=============================================================================
// パッドのボタンの状態を取得（一度キーを離さないと押せない）
// 戻り値：BYTE ボタンの状態
// 引　数：const unsigned int ゲームパッド番号（０～）
// 　　　　const unsigned int ボタン番号（０～３１）
//=============================================================================
BYTE InputManage::GetPadButtonStateSync(const unsigned int idxPad, const unsigned int idxButton)
{
	if (m_vecPad.empty() || idxPad < 0 || idxPad >= m_vecPad.size() || idxButton < 0 || idxButton >= sizeof(m_vecPad[0].m_padState.rgbButtons) / sizeof(m_vecPad[0].m_padState.rgbButtons[0])) {
		return 0;
	}

	if (m_vecPad[idxPad].m_padState.rgbButtons[idxButton]) {
		if (m_vecPad[idxPad].m_padCheck.rgbButtons[idxButton] == 0x00) {
			m_vecPad[idxPad].m_padCheck.rgbButtons[idxButton] = m_vecPad[idxPad].m_padState.rgbButtons[idxButton];
			return m_vecPad[idxPad].m_padState.rgbButtons[idxButton];
		}
		else {
			return 0x00;
		}
	}
	else {
		if (m_vecPad[idxPad].m_padCheck.rgbButtons[idxButton]) {
			m_vecPad[idxPad].m_padCheck.rgbButtons[idxButton] = m_vecPad[idxPad].m_padState.rgbButtons[idxButton];
		}
		return 0x00;
	}
}

//=============================================================================
// パッドの十字キーの状態を取得
// 戻り値：BYTE ボタンの状態
// 引　数：const unsigned int ゲームパッド番号（０～）
// 　　　　const unsigned int 方向（DIK_HORIZONTAL, DIK_VERTICAL）
//=============================================================================
int InputManage::GetPadCrossKeyState(const unsigned int idxPad, const unsigned int direction) const
{
	if (m_vecPad.empty() || idxPad < 0 || idxPad >= m_vecPad.size()) {
		return 0;
	}

	if (direction == InputKey::DIK_HORIZONTAL) {
		return m_vecPad[idxPad].m_padState.lX / PAD_RANGE;
	}

	if (direction == InputKey::DIK_VERTICAL) {
		return m_vecPad[idxPad].m_padState.lY / PAD_RANGE;
	}

	return 0;
}

//=============================================================================
// パッドの十字キーの状態を取得（一度キーを離さないと押せない）
// 戻り値：BYTE ボタンの状態
// 引　数：const unsigned int ゲームパッド番号（０～）
// 　　　　const unsigned int 方向（DIK_HORIZONTAL, DIK_VERTICAL）
//=============================================================================
int InputManage::GetPadCrossKeyStateSync(const unsigned int idxPad, const unsigned int direction)
{
	if (m_vecPad.empty() || idxPad < 0 || idxPad >= m_vecPad.size()) {
		return 0;
	}

	if (direction == InputKey::DIK_HORIZONTAL) {
		if (abs(m_vecPad[idxPad].m_padState.lX) >= PAD_RANGE) {
			if (m_vecPad[idxPad].m_padCheck.rglSlider[0] == 0) {
				m_vecPad[idxPad].m_padCheck.rglSlider[0] = 1;
				return m_vecPad[idxPad].m_padState.lX / PAD_RANGE;
			}
			else {
				return 0;
			}
		}
		else {
			m_vecPad[idxPad].m_padCheck.rglSlider[0] = 0;
		}
	}

	if (direction == InputKey::DIK_VERTICAL) {
		if (abs(m_vecPad[idxPad].m_padState.lY) >= PAD_RANGE) {
			if (m_vecPad[idxPad].m_padCheck.rglSlider[1] == 0) {
				m_vecPad[idxPad].m_padCheck.rglSlider[1] = 1;
				return m_vecPad[idxPad].m_padState.lY / PAD_RANGE;
			}
			else {
				return 0;
			}
		}
		else {
			m_vecPad[idxPad].m_padCheck.rglSlider[1] = 0;
		}
	}

	return 0;
}

//=============================================================================
// パッドのアナログスティック（左）の状態を取得
// 戻り値：double ボタンの状態（－１～＋１）
// 引　数：const unsigned int ゲームパッド番号（０～）
// 　　　　const unsigned int 方向（DIK_HORIZONTAL, DIK_VERTICAL）
//=============================================================================
double InputManage::GetPadLeftAnalogStickState(const unsigned int idxPad, const unsigned int direction) const
{
	if (m_vecPad.empty() || idxPad < 0 || idxPad >= m_vecPad.size()) {
		return 0;
	}

	if (direction == InputKey::DIK_HORIZONTAL) {
		return m_vecPad[idxPad].m_padState.lX / static_cast<double>(PAD_RANGE);
	}

	if (direction == InputKey::DIK_VERTICAL) {
		return m_vecPad[idxPad].m_padState.lY / static_cast<double>(PAD_RANGE);
	}

	return 0;
}

//=============================================================================
// パッドのアナログスティック（右）の状態を取得
// 戻り値：double ボタンの状態（－１～＋１）
// 引　数：const unsigned int ゲームパッド番号（０～）
// 　　　　const unsigned int 方向（DIK_HORIZONTAL, DIK_VERTICAL）
//=============================================================================
double InputManage::GetPadRightAnalogStickState(const unsigned int idxPad, const unsigned int direction) const
{
	if (m_vecPad.empty() || idxPad < 0 || idxPad >= m_vecPad.size()) {
		return 0;
	}

	if (direction == InputKey::DIK_HORIZONTAL) {
		return m_vecPad[idxPad].m_padState.lZ / static_cast<double>(PAD_RANGE);
	}

	if (direction == InputKey::DIK_VERTICAL) {
		return m_vecPad[idxPad].m_padState.lRz / static_cast<double>(PAD_RANGE);
	}

	return 0;
}




//*****************************************************************************
// private関数
//*****************************************************************************


//-----------------------------------------------------------------------------
// キーボードデバイスの初期化
// 戻り値：成功 true  失敗 false
// 引　数：const HWND & ウィンドウハンドル
//-----------------------------------------------------------------------------
bool InputManage::InitializeKeyboard(const HWND &hAppWnd)
{
	//キーボードデバイスの作成
	if (FAILED(m_pDirectInput->CreateDevice(GUID_SysKeyboard, &m_pKeyboardDevice, nullptr))) {
		return false;
	}

	//データフォーマットの指定
	if (FAILED(m_pKeyboardDevice->SetDataFormat(&c_dfDIKeyboard))) {
		return false;
	}

	//協調レベルの設定（アクティブかつ非排他）
	if (FAILED(m_pKeyboardDevice->SetCooperativeLevel(hAppWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE))) {
		return false;
	}

	//キーバッファのクリア
	memset(m_keyBuffer, 0, sizeof(m_keyBuffer));

	//チェック用バッファのクリア
	memset(m_keyCheck, 0, sizeof(m_keyCheck));

	//アクセス権の取得
	m_pKeyboardDevice->Acquire();

	return true;
}

//-----------------------------------------------------------------------------
// マウスデバイスの初期化
// 戻り値：成功 true  失敗 false
// 引　数：const HWND & ウィンドウハンドル
//-----------------------------------------------------------------------------
bool InputManage::InitializeMouse(const HWND &hAppWnd)
{
	//マウスデバイスの作成
	if (FAILED(m_pDirectInput->CreateDevice(GUID_SysMouse, &m_pMouseDevice, nullptr))) {
		return false;
	}

	//データフォーマットの指定
	if (FAILED(m_pMouseDevice->SetDataFormat(&c_dfDIMouse))) {
		return false;
	}

	//協調レベルの設定（アクティブかつ非排他）
	if (FAILED(m_pMouseDevice->SetCooperativeLevel(hAppWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE))) {
		return false;
	}

	//プロパティの設定
	DIPROPDWORD diProperty;

	diProperty.diph.dwSize = sizeof(diProperty);
	diProperty.diph.dwHeaderSize = sizeof(diProperty.diph);
	diProperty.diph.dwObj = 0;
	diProperty.diph.dwHow = DIPH_DEVICE;
	diProperty.dwData = DIPROPAXISMODE_REL;

	if (FAILED(m_pMouseDevice->SetProperty(DIPROP_AXISMODE, &diProperty.diph))) {
		return false;
	}

	//アクセス権の取得
	m_pMouseDevice->Acquire();

	//チェック用領域のクリア
	memset(m_mouseCheck.rgbButtons, 0, sizeof(BYTE) * (sizeof(m_mouseCheck.rgbButtons) / sizeof(m_mouseCheck.rgbButtons[0])));

	return true;
}

//-----------------------------------------------------------------------------
// ゲームパッドの初期化
// 戻り値：成功 true  失敗 false
// 引　数：const HWND & ウィンドウハンドル
//-----------------------------------------------------------------------------
bool InputManage::InitializePad(const HWND &hAppWnd)
{
	//デバイスの列挙
	m_pDirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL, (LPDIENUMDEVICESCALLBACK)EnumGamepadCallback, this, DIEDFL_ATTACHEDONLY);

	//ゲームパッドが無い場合は正常終了
	if (m_vecPad.empty()) {
		return true;
	}

	//ゲームパッドがあった場合は各種設定を行う
	for (unsigned int i = 0; i < m_vecPad.size(); i++) {

		//データフォーマットの指定
		if (FAILED(m_vecPad[i].m_pPadDevice->SetDataFormat(&c_dfDIJoystick))) {
			return false;
		}

		//協調レベルの設定（アクティブかつ非排他）
		if (FAILED(m_vecPad[i].m_pPadDevice->SetCooperativeLevel(hAppWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE))) {
			return false;
		}

		//チェック用データの初期化
		memset(&m_vecPad[i].m_padCheck, 0, sizeof(m_vecPad[i].m_padCheck));
		
		//パッドの軸範囲などの設定
		m_vecPad[i].m_pPadDevice->EnumObjects(EnumAxisCallback, &m_vecPad[i], DIDFT_AXIS);
		
		if (FAILED(m_vecPad[i].m_pPadDevice->Poll())) {

			while (m_vecPad[i].m_pPadDevice->Acquire() == DIERR_INPUTLOST)
				;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// ゲームパッドの列挙（コールバック関数）
// 引　数：VOID* InputManageクラスのポインタ
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumGamepadCallback(LPDIDEVICEINSTANCE pdidInstance, VOID *pContext)
{
	std::vector<GamePad> &vecPad = static_cast<InputManage*>(pContext)->GetPadVector();

	GamePad gamePad;

	gamePad.m_pPadDevice = nullptr;

	vecPad.push_back(gamePad);

	if (FAILED(((InputManage*)pContext)->GetDirectInput()->CreateDevice(pdidInstance->guidInstance, &(vecPad.back().m_pPadDevice), nullptr))) {
		return DIENUM_STOP;
	}

	return DIENUM_CONTINUE;
}

//-----------------------------------------------------------------------------
// ゲームパッドの列挙（コールバック関数）
// 引　数：VOID* GamePad構造体のポインタ
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumAxisCallback(const DIDEVICEOBJECTINSTANCE *pdidObjectInstance, VOID *pContext)
{
	GamePad *pGamePad = reinterpret_cast<GamePad*>(pContext);

	pGamePad->m_padRange.diph.dwSize = sizeof(DIPROPRANGE);
	pGamePad->m_padRange.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	pGamePad->m_padRange.diph.dwHow = DIPH_BYID;
	pGamePad->m_padRange.diph.dwObj = pdidObjectInstance->dwType;
	pGamePad->m_padRange.lMin = -InputManage::PAD_RANGE;
	pGamePad->m_padRange.lMax = +InputManage::PAD_RANGE;

	if (FAILED(pGamePad->m_pPadDevice->SetProperty(DIPROP_RANGE, &pGamePad->m_padRange.diph))) {
		return DIENUM_STOP;
	}

	return DIENUM_CONTINUE;
}

//-----------------------------------------------------------------------------
// DirectInputの参照を取得
// 戻り値：LPDIRECTINPUT8& DirectInputの参照
//-----------------------------------------------------------------------------
LPDIRECTINPUT8& InputManage::GetDirectInput()
{
	return m_pDirectInput;
}

//-----------------------------------------------------------------------------
// ゲームパッド「ベクタ」の参照を取得
// 戻り値：std::vector<GamePad>& ゲームパッド「ベクタ」の参照
//-----------------------------------------------------------------------------
std::vector<GamePad>& InputManage::GetPadVector()
{
	return m_vecPad;
}
