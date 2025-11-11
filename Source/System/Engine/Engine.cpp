//*****************************************************************************
//
// ３Ｄエンジンクラス（２Ｄ含む）
//
// Engine.cpp
//
// K_Yamaoka
//
// 2013/03/04
//
// 2013/06/18 WireframeON,OFFおよびCullingON,OFFを追加
//
// 2014/10/24 DrawNumber関数を追加
//
// 2014/11/28 エラーメッセージを追加
//
// 2014/12/04 SCREENSHOT_FOLDERのstaticを外し、初期化リストで初期化
//            DrawPrintf,DebugPrintf関数の内容を変更(vsprintf_sを使用)
//            ウィンドウモードフラグbWindowedを追加
//            ScreenShot関数ウィンドウ枠の調整
//
// 2016/11/14 Initialize関数の引数hAppWndを参照へ、hInstanceを追加
//            InputWait, InputAsync, InputSync関数を削除
//            m_pClsFps -> m_pClsFpsManageに変更
//            UpdateKeyboardState, GetKeyState関数を追加
//            UpdateMouseState, GetMouseButton関数を追加
//
// 2016/11/15 UpdateInputDeviceState関数を追加
//            GetMouseMove関数を追加
//            GetWheelDelta関数をGetMouseWheelDelta関数へ変更
//            ゲームパッド関連の関数を追加
//
// 2016/11/16 GetKeyStateSync, GetMouseButtonSync, GetPadButtonSync関数を追加
//
// 2016/12/07 DrawNumberのバグ「0が表示できない」を修正
//
// 2016/12/21 フリーサイズテクスチャに対応
//
// 2017/06/20 DrawLineを追加
//
// 2017/10/13 DrawCircleを追加
//
// 2018/12/19 AddFontでのフォントの幅を0に変更
//
// 2019/07/16 DrawStringの引数の順番を変更
//
// 2020/08/26 Initialize関数にシェーダーバージョンのチェックを追加
//
// 2020/11/26 DrawLine（３Ｄをシェーダー対応に変更）
//
// 2021/01/26 GetPadCrossKeyStateSync関数を追加
//
// 2021/04/13 ImGui(Immediate Mode GUI)用関数を追加
//
// 2021/10/06 SEにDirectSoundを使用できるよう変更
//
// 2021/10/11 GetWindowHandle, GetInstanceHandle関数を追加
//
// 2021/10/28 GetFPS関数を追加
//
// 2021/11/16 ３Ｄのフレームレートを上げるためImGuiを外したバージョンを作成
//
// 2021/11/30 １年生用に直線描画関数を追加
//
// 2021/12/01 ポストエフェクト用にバックバッファをロック可能に変更
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "engine.h"

#include <sstream>
#include <cassert>
#include <ctime>
#include <iomanip>
#include <shlwapi.h>

//DrawPrintf用最大文字数
const int Engine::MAX_MESSAGE = 1024;

//=============================================================================
// コンストラクタ
//=============================================================================
Engine::Engine()
	: SCREENSHOT_FOLDER("ScreenShot")
	, m_bWindowed(WindowSetting::MODE_WINDOW)
	, m_pDirect3D9(nullptr)
	, m_pDevice(nullptr)
	, m_pClsInputManage(nullptr)
	, m_pSprite(nullptr)
	, m_pClsFpsManage(nullptr)
	, m_pClsTextureManage(nullptr)
	, m_pClsFontManage(nullptr)
	, m_pClsBGMManage(nullptr)
	, m_pClsSEManage(nullptr)
	, m_pClsModelManage(nullptr)
	, m_pClsLine(nullptr)
	, m_bFreeSizeTexture(false)
	, m_pBackBuffer(nullptr)
	, m_pOffScreenSurface(nullptr)
	, m_d3dparam({ 0 })
{

}

//=============================================================================
// デストラクタ
//=============================================================================
Engine::~Engine()
{
#ifdef USE_IMGUI
	//Imgui関係オブジェクトの解放
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif

	//線描画クラスの解放
	if (m_pClsLine) {
		delete m_pClsLine;
		m_pClsLine = nullptr;
	}

	//モデル管理クラスの解放
	if (m_pClsModelManage) {
		delete m_pClsModelManage;
		m_pClsModelManage = nullptr;
	}

	//ＳＥ管理クラスの解放
	if (m_pClsSEManage) {
		delete m_pClsSEManage;
		m_pClsSEManage = nullptr;
	}

	//ＢＧＭ管理クラスの解放
	if (m_pClsBGMManage) {
		delete m_pClsBGMManage;
		m_pClsBGMManage = nullptr;
	}

	//フォント管理クラスの解放
	if (m_pClsFontManage) {
		delete m_pClsFontManage;
		m_pClsFontManage = nullptr;
	}

	//2D用テクスチャ管理クラスの解放
	if (m_pClsTextureManage) {
		delete m_pClsTextureManage;
		m_pClsTextureManage = nullptr;
	}

	//FPS管理クラスの解放
	if (m_pClsFpsManage) {
		delete m_pClsFpsManage;
		m_pClsFpsManage = nullptr;
	}

	//スプライトの解放
	if (m_pSprite) {
		m_pSprite->Release();
		m_pSprite = nullptr;
	}

	//入力管理クラスの解放
	if (m_pClsInputManage) {
		delete m_pClsInputManage;
		m_pClsInputManage = nullptr;
	}

	//バックバッファの解放
	if (m_pBackBuffer) {
		m_pBackBuffer->Release();
		m_pBackBuffer = nullptr;
	}

	//デバイスの解放
	if (m_pDevice) {
		m_pDevice->Release();
		m_pDevice = nullptr;
	}

	//Direct3Dの解放
	if (m_pDirect3D9) {
		m_pDirect3D9->Release();
		m_pDirect3D9 = nullptr;
	}
}

//=============================================================================
// ３Ｄエンジンの初期化
// 引　数：const HWND & ウインドウハンドル
// 　　　　const HINSTANCE & インスタンスハンドル
//=============================================================================
void Engine::Initialize(const HWND &hAppWnd, const HINSTANCE &hInstance)
{
	//ウインドウハンドルの保管
	m_hAppWnd = hAppWnd;

	//インスタンスハンドルの保管
	m_hInstance = hInstance;

	//Direct3D本体の作成
	m_pDirect3D9 = Direct3DCreate9(D3D_SDK_VERSION);

	if (!m_pDirect3D9) {
		throw DxSystemException(DxSystemException::OM_D3D_CREATE_ERROR);
	}

	//３Ｄデバイス（エンジン）の作成

	//ウインドウハンドル
	m_d3dparam.hDeviceWindow = m_hAppWnd;

	//ウインドウモード or フルスクリーン
	m_d3dparam.Windowed = m_bWindowed;

	//バックバッファの数（ダブルバッファ）
	m_d3dparam.BackBufferCount = 1;

	//バックバッファのサイズ
	m_d3dparam.BackBufferWidth = WindowSetting::WINDOW_WIDTH;
	m_d3dparam.BackBufferHeight = WindowSetting::WINDOW_HEIGHT;

	//ピクセルフォーマット
	m_d3dparam.BackBufferFormat = D3DFMT_X8R8G8B8;

	//深度バッファ
	m_d3dparam.EnableAutoDepthStencil = true;
	m_d3dparam.AutoDepthStencilFormat = D3DFMT_D24S8;

	//スワップエフェクト
	//Present時のバック、フロントの切り替え方法
	m_d3dparam.SwapEffect = D3DSWAPEFFECT_DISCARD;

	//プレゼンテーションインターバル
	m_d3dparam.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	//バックバッファのロック可能（ポストエフェクト用）
	m_d3dparam.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

	//結果ハンドル(DirectX関数の戻り値)
	HRESULT hResult;

	//デバイス（ビデオボード）能力構造体
	D3DCAPS9 caps;

	//デバイスの能力を取得（タイプ：ハードウェア）
	hResult = m_pDirect3D9->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_D3D_GETCAPS_ERROR);
	}

	//振る舞いフラグ
	DWORD behaviorFlag;

	//デバイスの能力チェック（頂点処理の方法を決定）
	if ((caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0 || caps.VertexShaderVersion < D3DVS_VERSION(1, 1)) {
		//頂点、ライト計算にハードウェアが使えない
		//頂点シェーダが使えない
		behaviorFlag = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	}
	else {
		if (caps.MaxVertexBlendMatrices < 2) {
			//マトリックスのブレンドが使えない
			behaviorFlag = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
		}
		else {
			behaviorFlag = D3DCREATE_MIXED_VERTEXPROCESSING;
		}
	}

	//デバイスの作成
	hResult = m_pDirect3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hAppWnd, behaviorFlag, &m_d3dparam, &m_pDevice);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_DEVICE_CREATE_ERROR);
	}

	//バックバッファの取得
	hResult = m_pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_DEVICE_GETBACKBUFFER_ERROR);
	}

	//シェーダーバージョンチェック

	if (caps.VertexShaderVersion < D3DVS_VERSION(3, 0)) {
		throw DxSystemException(DxSystemException::OM_DEVICE_CREATE_ERROR);
	}

	if (caps.PixelShaderVersion < D3DVS_VERSION(3, 0)) {
		throw DxSystemException(DxSystemException::OM_DEVICE_CREATE_ERROR);
	}

	//テクスチャ能力のチェック

	if ((caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL) == 0
		&& (caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL) == 0) {
		m_bFreeSizeTexture = true;
	}

	//入力管理クラスの作成
	try {
		m_pClsInputManage = new InputManage;
	}
	catch (std::bad_alloc) {
		throw DxSystemException(DxSystemException::OM_INPUT_ALLOCATE_ERROR);
	}

	//入力管理クラスの初期化
	try {
		m_pClsInputManage->Initialize(hAppWnd, hInstance);
	}
	catch (DxSystemException dxSystemException) {
		dxSystemException.ShowOriginalMessage();
		throw DxSystemException(DxSystemException::OM_INPUT_INITIALIZE_ERROR);
	}

	//スプライトの作成
	hResult = D3DXCreateSprite(m_pDevice, &m_pSprite);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_SPRITE_CREATE_ERROR);
	}

	//FPS管理クラスの作成
	try {
		m_pClsFpsManage = new Fps;
	}
	catch (std::bad_alloc) {
		throw DxSystemException(DxSystemException::OM_FPSMANAGE_ALLOCATE_ERROR);
	}

	//2D用テクスチャ管理クラスの作成
	try {
		m_pClsTextureManage = new TextureManage;
	}
	catch (std::bad_alloc) {
		throw DxSystemException(DxSystemException::OM_TEXTUREMANAGE_ALLOCATE_ERROR);
	}

	//2D用テクスチャ管理クラスの初期化
	try {
		m_pClsTextureManage->Initialize(m_pDirect3D9);
	}
	catch (DxSystemException dxSystemException) {
		dxSystemException.ShowOriginalMessage();
		throw DxSystemException(DxSystemException::OM_TEXTUREMANAGE_INITIALIZE_ERROR);
	}

	//フォント管理クラスの作成
	try {
		m_pClsFontManage = new FontManage;
	}
	catch (std::bad_alloc) {
		throw DxSystemException(DxSystemException::OM_FONTMANAGE_ALLOCATE_ERROR);
	}

	//ＢＧＭ管理クラスの作成
	try {
		m_pClsBGMManage = new BGMManage;
	}
	catch (std::bad_alloc) {
		throw DxSystemException(DxSystemException::OM_BGMMANAGE_ALLOCATE_ERROR);
	}

	//ＢＧＭ管理クラスの初期化
	try {
		m_pClsBGMManage->Initializie();
	}
	catch (DxSystemException dxSystemException) {
		dxSystemException.ShowOriginalMessage();
		throw DxSystemException(DxSystemException::OM_BGMMANAGE_INITIALIZE_ERROR);
	}

	//ＳＥ管理クラスの作成
	try {
#ifdef SE_USE_DMUSIC
		m_pClsSEManage = new SEManage;
#endif
#ifdef SE_USE_DSOUND
		m_pClsSEManage = new DSManage;
#endif
	}
	catch (std::bad_alloc) {
		throw DxSystemException(DxSystemException::OM_SEMANAGE_ALLOCATE_ERROR);
	}

	//ＳＥ管理クラスの初期化
	try {
		m_pClsSEManage->Initialize(m_hAppWnd);
	}
	catch (DxSystemException dxSystemException) {
		dxSystemException.ShowOriginalMessage();
		throw DxSystemException(DxSystemException::OM_SEMANAGE_INITIALIZE_ERROR);
	}

	//モデル管理クラスの作成
	m_pClsModelManage = new ModelManage();

	if (!m_pClsModelManage) {
		throw DxSystemException(DxSystemException::OM_MODELMANAGE_ALLOCATE_ERROR);
	}

	//線描画クラスの作成
	m_pClsLine = new Line();

	if (!m_pClsLine) {
		throw DxSystemException(DxSystemException::OM_CREATE_LINE_ALLOCATE_ERROR);
	}

	//線描画クラスの初期化
	try {
		m_pClsLine->Initialize(m_pDevice);
	}
	catch (DxSystemException dxSystemException) {
		dxSystemException.ShowOriginalMessage();
		throw DxSystemException(DxSystemException::OM_CREATE_LINE_OBJECT_ERROR);
	}

#ifdef USE_IMGUI
	//Imgui初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui_ImplWin32_Init(m_hAppWnd);
	ImGui_ImplDX9_Init(m_pDevice);

	//日本語フォントはメイリオ固定
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\meiryo.ttc", 18.0f, nullptr, glyphRangesJapanese);

	//スタイル
	//ImGui::StyleColorsClassic();
	ImGui::StyleColorsLight();
	//ImGui::StyleColorsDark();
#endif
}

//=============================================================================
//デバイスの取得
// 戻り値：LPDIRECT3DDEVICE9　デバイスのポインタ
//=============================================================================
LPDIRECT3DDEVICE9 Engine::GetDevice() const
{

	return m_pDevice;
}


//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw
//
// 描画関連
// 　テクスチャ、文字列の描画に必要な関数
//
//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw

//=============================================================================
// 画面のクリア
// 引　数：背景色
//=============================================================================
void Engine::Clear(D3DCOLOR backgroundColor)
{
	HRESULT	hResult = m_pDevice->Clear(
		0,                                  //クリアする矩形の数
		nullptr,                            //画面全体(nullptr)
		D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, //画面とＺバッファをクリア
		backgroundColor,                    //背景色
		1,                                  //Ｚバッファの初期値
		0                                   //ステンシルバッファの初期値
	);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_DEVICE_CLEAR_ERROR);
	}
}

#ifdef USE_IMGUI
//=============================================================================
// ImGui用　フレーム開始
// 備　考：この関数呼び出した後、必ずEndFrame関数を呼び出すこと
//=============================================================================
void Engine::BeginFrame()
{
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

#if false
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

	/*

	ImGuiBegin(pos, size, flag);

	data = ImGuiInput(Title);

	ImGuiEnd();
	*/

	ImGui::SetNextWindowPos(ImVec2(100.0f, 100.0f));
	ImGui::SetNextWindowSize(ImVec2(230.0f, 80.0f));

	{
		static bool show_another_window = true;

		if (show_another_window) {

			std::string str = "";
			static std::string save_str = "";

			static std::string last_str;

			ImGuiWindowFlags window_flags = 0;
			//window_flags |= ImGuiWindowFlags_NoBackground;
			window_flags |= ImGuiWindowFlags_NoTitleBar;
			window_flags |= ImGuiWindowFlags_NoResize;
			window_flags |= ImGuiWindowFlags_NoMove;
			//window_flags |= ImGuiWindowFlags_NoScrollbar;
			//window_flags |= ImGuiWindowFlags_NoCollapse;
			//window_flags |= ImGuiWindowFlags_MenuBar;

			ImGui::Begin(u8"Window", &show_another_window, window_flags);

			ImGui::Text(u8"名前の入力");

			if (ImGui::GetKeyPressedAmount(VK_RETURN, 0, 0))
				ImGui::SetKeyboardFocusHere();

			ImGui::InputText(u8"", &str);

			////UTF8 -> SJISに変えなければゲーム中で表示できない
			str = UTF8toSjis(str);

			if (str != save_str) {

				if (str != "") {

					//入力の変化があった
					int a = 3;
#if false
					char str2[1000] = { 0 };
					char str3[1000];
					for (int i = 0; i < str.size(); i++) {
						sprintf(str3, "%d ", (int)str[i]);
						strcat(str2, str3);
					}
					OutputDebugString(str.c_str());
					OutputDebugString(str2);
					OutputDebugString("\n");
#endif
				}
				else {

					//最後にEnter押した
					OutputDebugString("Enter : ");
					OutputDebugString(save_str.c_str());
					OutputDebugString("\n");
					if(save_str != "")
						last_str = save_str;
					int a = 3;
				}

				save_str = str;
			}

			//str = "";

			ImGui::SameLine();

			if (ImGui::Button(u8"ＯＫ")) {
				OutputDebugString("Push OK Button : ");
				OutputDebugString(last_str.c_str());
				OutputDebugString("\n");
				show_another_window = false;
			}

			ImGui::End();
		}
	}

	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
}
#endif

#ifdef USE_IMGUI
//=============================================================================
// ImGui用　フレーム終了
//=============================================================================
void Engine::EndFrame()
{
	ImGui::EndFrame();
}
#endif

//=============================================================================
// 描画の開始
// 備　考：この関数呼び出した後、必ずEndScene関数を呼び出すこと
//=============================================================================
void Engine::BeginScene()
{
	HRESULT hResult = m_pDevice->BeginScene();

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_DEVICE_BEGINSCENE_ERROR);
	}
}

//=============================================================================
// 描画の終了
//=============================================================================
void Engine::EndScene()
{
#ifdef USE_IMGUI
	//Imgui用　オブジェクトのレンダリング
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
#endif

	HRESULT	hResult = m_pDevice->EndScene();

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_DEVICE_ENDSCENE_ERROR);
	}
}

//=============================================================================
// フロントバッファへ転送
// 備　考：この関数を呼び出したタイミングで画面に描画される
//=============================================================================
void Engine::Present()
{
	HRESULT	hResult = m_pDevice->Present(nullptr, nullptr, nullptr, nullptr);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_DEVICE_PRESENT_ERROR);
	}

	m_pClsFpsManage->UpdateFPS();
	m_pClsFpsManage->FixRefreshRate();
}

//=============================================================================
// スプライトの描画開始
// 備　考：この関数が成功した場合、必ずSpriteEnd関数を呼び出すこと
//=============================================================================
void Engine::SpriteBegin()
{
	HRESULT hResult;

	//アルファブレンド設定

	hResult = m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_DEVICE_ALPHA_ENABLE_ERROR);
	}

	hResult = m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_DEVICE_RENDER_SRC_ERROR);
	}

	hResult = m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_DEVICE_RENDER_DST_ERROR);
	}

	//Ｚバッファへの書き込みを禁止

	hResult = m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, false);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_ZWRITEDISABLE_ERROR);
	}

	//スプライトの描画開始

	hResult = m_pSprite->Begin(D3DXSPRITE_SORT_DEPTH_BACKTOFRONT);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_SPRITE_BEGIN_ERROR);
	}
}

//=============================================================================
// スプライトの描画終了
//=============================================================================
void Engine::SpriteEnd()
{
	HRESULT hResult;

	//スプライトの描画完了

	hResult = m_pSprite->End();

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_SPRITE_END_ERROR);
	}

	//Ｚバッファへの書き込みを許可

	hResult = m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, true);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_ZWRITEENABLE_ERROR);
	}

	//アルファブレンドの解除

	hResult = m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_DEVICE_ALPHA_DISABLE_ERROR);
	}
}

//=============================================================================
// 画像の転送(転送元と転送先のRECT構造体のサイズが異なる場合は拡大、縮小される)
// 引　数：RECT*  転送先情報の入ったRECT構造体へのポインタ
// 　　　　string 転送する画像のファイル名
// 　　　　RECT*  転送元情報の入ったRECT構造体へのポインタ
// 　　　　DWORD  アルファ値(０から２５５まで)
// 　　　　float  回転角度(ラジアン角)
//=============================================================================
void Engine::Blt(const RECT* pDest, std::string strFileName, const RECT* pSour, const DWORD alpha, const float angle)
{
	assert(strFileName.length() > 0);

	//拡大・縮小率の計算
	float scale_x = (float)(pDest->right - pDest->left) / (float)(pSour->right - pSour->left);
	float scale_y = (float)(pDest->bottom - pDest->top) / (float)(pSour->bottom - pSour->top);

	//転送元画像の中心を計算
	float srcCenter_x = (float)(pSour->right - pSour->left) / 2.0f;
	float srcCenter_y = (float)(pSour->bottom - pSour->top) / 2.0f;

	//転送先画像の中心を計算
	float dstCenter_x = (float)(pDest->right - pDest->left) / 2.0f;
	float dstCenter_y = (float)(pDest->bottom - pDest->top) / 2.0f;

	//回転の中心ベクトル
	D3DXVECTOR3 vecCenter(srcCenter_x, srcCenter_y, 0);

	//転送先座標を求めるための行列を作成

	D3DXMATRIX matScale;     //スケール変換
	D3DXMATRIX matRotate;    //回転
	D3DXMATRIX matTranslate; //平行移動
	D3DXMATRIX matWorld;     //ワールド座標変換

	D3DXMatrixScaling(&matScale, scale_x, scale_y, 1.0f);

	D3DXMatrixRotationZ(&matRotate, angle);

	//転送先の中心座標を加えなければ、画像の中心が描画位置になる
	D3DXMatrixTranslation(&matTranslate, (float)pDest->left + dstCenter_x, (float)pDest->top + dstCenter_y, 0);

	//ワールド座標変換行列の作成
	matWorld = matScale * matRotate * matTranslate;

	//スプライトに行列を設定
	m_pSprite->SetTransform(&matWorld);

	//テクスチャの取得
	LPDIRECT3DTEXTURE9 pTexture = m_pClsTextureManage->SearchTexture(strFileName);

	if (!pTexture) {
		throw DxSystemException(DxSystemException::OM_TEXTUREMANAGE_NOTFOUND_ERROR);
	}

	//スプライトの描画
	HRESULT hResult = m_pSprite->Draw(pTexture, pSour, &vecCenter, nullptr, D3DCOLOR_ARGB(alpha & 0x000000FF, 255, 255, 255));

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_SPRITE_DRAW_ERROR);
	}

	//スプライトに単位行列を設定

	D3DXMatrixIdentity(&matWorld);

	m_pSprite->SetTransform(&matWorld);
}

// 丸形ステンシル ON（2D用）
void Engine::BeginMiniMap(const D3DXVECTOR2& center, float radius, int div)
{
	m_pDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE);
	m_pDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
	m_pDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
	m_pDevice->SetRenderState(D3DRS_STENCILREF, 1);

	// 色は書かない
	m_pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0);

	DrawFilledCircle(center, radius, div, D3DCOLOR_ARGB(255, 255, 255, 255));

	// 色バッファ戻す
	m_pDevice->SetRenderState(D3DRS_COLORWRITEENABLE,
		D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

	// ステンシル条件を有効に
	m_pDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
	m_pDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
}

void Engine::EndMiniMap()
{
	m_pDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
}

void Engine::DrawFilledCircle(const D3DXVECTOR2& center, float radius, int div, D3DCOLOR color)
{
	struct Vertex
	{
		D3DXVECTOR4 pos;
		D3DCOLOR color;
	};

	std::vector<Vertex> vertices;
	vertices.reserve(div + 2);

	vertices.push_back({ D3DXVECTOR4(center.x, center.y, 0.0f, 1.0f), color });

	for (int i = 0; i <= div; ++i)
	{
		float angle = (float)i / div * D3DX_PI * 2.0f;
		float x = center.x + cosf(angle) * radius;
		float y = center.y + sinf(angle) * radius;
		vertices.push_back({ D3DXVECTOR4(x, y, 0.0f, 1.0f), color });
	}

	m_pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, div, vertices.data(), sizeof(Vertex));
}

//=============================================================================
// オフスクリーンレンダーターゲット開始
//=============================================================================
LPDIRECT3DSURFACE9 Engine::BeginOffScreenRender(int width, int height)
{
	LPDIRECT3DSURFACE9 pOldBackBuffer = nullptr;
	m_pDevice->GetRenderTarget(0, &pOldBackBuffer);

	if (m_pOffScreenSurface) {
		m_pOffScreenSurface->Release();
		m_pOffScreenSurface = nullptr;
	}

	HRESULT hr = m_pDevice->CreateRenderTarget(
		width, height,
		D3DFMT_A8R8G8B8,
		D3DMULTISAMPLE_NONE, 0, TRUE,
		&m_pOffScreenSurface, NULL);

	if (FAILED(hr)) {
		throw DxSystemException(DxSystemException::OM_SS_CANTCREATE_OFFSCREENSURFACE_ERROR);
	}

	// レンダーターゲットを設定
	m_pDevice->SetRenderTarget(0, m_pOffScreenSurface);

	// ビューポートをオフスクリーンサイズに合わせて設定
	D3DVIEWPORT9 viewport = {};
	viewport.X = 0;
	viewport.Y = 0;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinZ = 0.0f;
	viewport.MaxZ = 1.0f;

	m_pDevice->SetViewport(&viewport);

	return pOldBackBuffer;
}

//=============================================================================
// オフスクリーンレンダーターゲット終了・保存・復帰
//=============================================================================
void Engine::EndOffScreenRender(LPDIRECT3DSURFACE9 pOldBackBuffer, const std::string& fileName)
{
	// サーフェスをファイルに保存
	HRESULT hr = D3DXSaveSurfaceToFile(fileName.c_str(), D3DXIFF_PNG, m_pOffScreenSurface, NULL, NULL);

	if (FAILED(hr)) {
		throw DxSystemException(DxSystemException::OM_SS_CANT_SAVE_ERROR);
	}

	// レンダーターゲットを元に戻す
	m_pDevice->SetRenderTarget(0, pOldBackBuffer);

	// ビューポートを元に戻す（ウィンドウサイズに再設定する）
	D3DVIEWPORT9 viewport = {};
	viewport.X = 0;
	viewport.Y = 0;
	viewport.Width = WindowSetting::WINDOW_WIDTH;
	viewport.Height = WindowSetting::WINDOW_HEIGHT;
	viewport.MinZ = 0.0f;
	viewport.MaxZ = 1.0f;

	m_pDevice->SetViewport(&viewport);

	// 後処理
	if (m_pOffScreenSurface) {
		m_pOffScreenSurface->Release();
		m_pOffScreenSurface = nullptr;
	}

	if (pOldBackBuffer) {
		pOldBackBuffer->Release();
	}
}


//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw
//
// 文字列表示
// 　文字列を画面、出力ウィンドウに表示
//
//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw

//=============================================================================
// 指定座標への文字転送
// 引　数：int      表示位置のx座標
// 　　　　int      表示位置のy座標
// 　　　　string   使用するフォント名
// 　　　　D3DCOLOR 文字色
// 　　　　string   表示文字
//=============================================================================
void Engine::DrawString(const int x, const int y, std::string strFontName, const D3DCOLOR argb, std::string strMessage)
{
	assert(strFontName.length() > 0);

	Font* pFontClass = m_pClsFontManage->SearchFont(strFontName);

	if (!pFontClass) {
		throw DxSystemException(DxSystemException::OM_FONTMANAGE_NOTFOUND_ERROR);
	}

	RECT rect = { x, y, WindowSetting::WINDOW_WIDTH, y + pFontClass->GetFontSize() };

	LPD3DXFONT pFont = pFontClass->GetFontPointer();

	HRESULT hResult = pFont->DrawText(m_pSprite, (LPCSTR)strMessage.c_str(), -1, &rect, DT_LEFT, argb);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_DRAWSTRING_ERROR);
	}
}

//=============================================================================
// 指定領域への文字転送
// 引　数：RECT*    表示領域を示すRECT構造体へのポインタ
// 　　　　string   表示文字
// 　　　　string   使用するフォント名
// 　　　　D3DCOLOR 文字色
//=============================================================================
void Engine::DrawString(RECT* pDest, std::string strFontName, const D3DCOLOR argb, std::string strMessage)
{
	assert(strFontName.length() > 0);

	Font* pFontClass = m_pClsFontManage->SearchFont(strFontName);

	if (!pFontClass) {
		throw DxSystemException(DxSystemException::OM_FONTMANAGE_NOTFOUND_ERROR);
	}

	LPD3DXFONT pFont = pFontClass->GetFontPointer();

	HRESULT hResult = pFont->DrawText(m_pSprite, (LPCSTR)strMessage.c_str(), -1, pDest, DT_LEFT | DT_WORDBREAK, argb);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_DRAWSTRING_ERROR);
	}
}

//=============================================================================
// 画面へフォーマット付文字列を表示
// 引　数：int      表示位置のx座標
// 　　　　int      表示位置のy座標
// 　　　　string   フォーマット付文字列
// 　　　　D3DCOLOR 文字色
// 　　　　...      可変引数
// 備　考：最大文字数半角1023文字まで
//=============================================================================
void Engine::DrawPrintf(const int x, const int y, std::string strFontName, const D3DCOLOR argb, std::string strFormat, ...)
{
	assert(strFontName.length() > 0 && strFormat.length() > 0);

	char message[MAX_MESSAGE];

	va_list list;

	va_start(list, strFormat);

	vsprintf_s(message, MAX_MESSAGE, strFormat.c_str(), list);

	va_end(list);

	Font* pFontClass = m_pClsFontManage->SearchFont(strFontName);

	if (!pFontClass) {
		throw DxSystemException(DxSystemException::OM_FONTMANAGE_NOTFOUND_ERROR);
	}

	RECT rect = { x, y, WindowSetting::WINDOW_WIDTH, y + pFontClass->GetFontSize() };

	LPD3DXFONT pFont = pFontClass->GetFontPointer();

	HRESULT hResult = pFont->DrawText(m_pSprite, (LPCSTR)message, -1, &rect, DT_LEFT, argb);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_DRAWSTRING_ERROR);
	}
}

void Engine::DrawPrintfCenter(const int centerX, const int y, std::string strFontName, const D3DCOLOR argb, std::string strFormat, ...)
{
	assert(strFontName.length() > 0 && strFormat.length() > 0);

	char message[MAX_MESSAGE];
	va_list list;
	va_start(list, strFormat);
	vsprintf_s(message, MAX_MESSAGE, strFormat.c_str(), list);
	va_end(list);

	Font* pFontClass = m_pClsFontManage->SearchFont(strFontName);
	if (!pFontClass) {
		throw DxSystemException(DxSystemException::OM_FONTMANAGE_NOTFOUND_ERROR);
	}

	LPD3DXFONT pFont = pFontClass->GetFontPointer();

	// 幅計算
	RECT rect = { 0, 0, 0, 0 };
	pFont->DrawTextA(nullptr, message, -1, &rect, DT_CALCRECT, argb);

	int textWidth = rect.right - rect.left;

	RECT drawRect = { centerX - textWidth / 2, y, centerX + textWidth / 2, y + pFontClass->GetFontSize()
	};

	HRESULT hResult = pFont->DrawText(m_pSprite, message, -1, &drawRect, DT_LEFT, argb);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_DRAWSTRING_ERROR);
	}
}


//=============================================================================
// 出力ウィンドウへフォーマット付文字列を表示
// 引　数：string フォーマット付文字列
// 　　　　...    可変引数
	// 備　考：最大文字数半角1023文字まで
//=============================================================================
void Engine::DebugPrintf(std::string strFormat, ...)
{
	assert(strFormat.length() > 0);

	char message[MAX_MESSAGE];

	va_list list;

	va_start(list, strFormat);

	vsprintf_s(message, MAX_MESSAGE, strFormat.c_str(), list);

	va_end(list);

	OutputDebugString((LPCSTR)message);
}

//=============================================================================
// 数字画像を描画
// 引　数：int      描画位置のx座標
// 　　　　int      描画位置のy座標
// 　　　　DWORD　　描画したい数値（正の整数のみ）
// 　　　　int　　　表示したい桁数
// 　　　　bool　　 前ゼロ制御（true 前ゼロあり、false なし）
// 　　　　int　　　転送元のx座標（画像の'０'の左上座標）
// 　　　　int　　　転送元のy座標（画像の'０'の左上座標）
// 　　　　int　　　数字画像１つ分の幅
// 　　　　int　　　数字画像１つ分の高さ
// 　　　　string   画像のファイル名
// 備　考："0123456789 "という並びの画像が必要であり、すべての数字画像の幅および
// 　　　　高さは同じでなければならない。
// 　　　　数字画像の右端の空欄は前ゼロなしの際の空白として使う。
//=============================================================================
void Engine::DrawNumber(const int destX, const int destY, const DWORD number, const int numOfDigits, const bool bUseLeadingZero, const int sourX, const int sourY, const int width, const int height, std::string strFileName)
{
	//数値をコピー
	DWORD workNumber = number;

	//実際の桁数を計算
	int digit = (int)log10(workNumber) + 1;

	//表示したい桁数より大きければ例外をスロー
	if (digit > numOfDigits) {
		throw DxSystemException(DxSystemException::OM_NUMBER_DIGITSOVER_ERROR);
	}

	//桁別数値格納領域
	int *pNumber;

	try {
		pNumber = new int[numOfDigits];
	}
	catch (std::bad_alloc) {
		throw DxSystemException(DxSystemException::OM_NUMBER_ALLOCATE_ERROR);
	}

	for (int i = 0; i < numOfDigits; i++) {
		pNumber[i] = 0;
	}

	if (number > 0) {
		int gap = numOfDigits - digit;
		DWORD divisor = (DWORD)pow(10, numOfDigits - gap - 1);

		for (int i = gap; i < numOfDigits; i++) {
			pNumber[i] = workNumber / divisor;
			workNumber %= divisor;
			divisor /= 10;
		}
	}

	bool bLeadingZero = true;

	RECT sour;
	RECT dest;

	for (int i = 0; i < numOfDigits; i++) {

		if (pNumber[i] != 0) {
			SetRect(&sour, sourX + pNumber[i] * width, sourY, sourX + pNumber[i] * width + width, sourY + height);
			SetRect(&dest, destX + width * i, destY, destX + width * i + width, destY + height);
			Blt(&dest, strFileName, &sour);

			if (bLeadingZero) {
				bLeadingZero = false;
			}
		}
		else {

			SetRect(&sour, sourX, sourY, sourX + width, sourY + height);

			if (!bUseLeadingZero) {
				if (bLeadingZero) {
					if (i != numOfDigits - 1) {
						SetRect(&sour, sourX + width * 10, sourY, sourX + width * 10 + width, sourY + height);
					}
				}
			}

			SetRect(&dest, destX + width * i, destY, destX + width * i + width, destY + height);
			Blt(&dest, strFileName, &sour);
		}
	}

	//領域解放
	if (pNumber) {
		delete[] pNumber;
		pNumber = nullptr;
	}
}

void Engine::GetTextSize(std::string strFontName, const char* message, int* pTextWidth, int* pTextHeight)
{
	Font* pFontClass = m_pClsFontManage->SearchFont(strFontName);
	if (!pFontClass) {
		*pTextWidth = 0;
		*pTextHeight = 0;
		return;
	}

	LPD3DXFONT pFont = pFontClass->GetFontPointer();

	RECT rect = { 0, 0, 0, 0 };
	pFont->DrawTextA(nullptr, message, -1, &rect, DT_CALCRECT, 0);

	*pTextWidth = rect.right - rect.left;
	*pTextHeight = pFontClass->GetFontSize();

	if (*pTextWidth == 0 && message && message[0] != '\0')
	{
		SIZE spaceSize = { 0, 0 };
		HDC hdc = pFont->GetDC();
		if (hdc)
		{
			GetTextExtentPoint32A(hdc, "A", 1, &spaceSize);
			*pTextWidth = (int)(spaceSize.cx * strlen(message));
			*pTextHeight = spaceSize.cy;
		}
	}
}



//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw
//
// テクスチャ
// 　テクスチャの追加と解放
//
//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw

//=============================================================================
// 2D用テクスチャの追加
// 引　数：string   画像のファイル名
// 　　　　D3DCOLOR カラーキー（よく使うものはTexture.hに定数化してある）
//=============================================================================
void Engine::AddTexture(std::string strFileName, D3DCOLOR colorKey)
{
	assert(strFileName.length() > 0);

	//登録済みならreturn
	if (m_pClsTextureManage->SearchTexture(strFileName)) {
		return;
	}

	LPDIRECT3DTEXTURE9 pTexture = nullptr;

	D3DXIMAGE_INFO imageInfo;

	D3DXGetImageInfoFromFile(strFileName.c_str(), &imageInfo);

	//テクスチャの作成
	HRESULT hResult = D3DXCreateTextureFromFileEx(m_pDevice, (LPCSTR)strFileName.c_str(), imageInfo.Width, imageInfo.Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, D3DX_FILTER_NONE, D3DX_FILTER_NONE, colorKey, &imageInfo, nullptr, &pTexture);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_TEXTURE_CREATE_ERROR);
	}

	//テクスチャのサイズチェック
	try {
		m_pClsTextureManage->CheckTextureSize(imageInfo, m_bFreeSizeTexture);
	}
	catch (DxSystemException dxSystemException) {
		throw dxSystemException;
	}

	//テクスチャクラスの作成
	Texture* pClsTexture;

	try {
		pClsTexture = new Texture;
	}
	catch (std::bad_alloc) {
		throw DxSystemException(DxSystemException::OM_TEXTURE_ALLOCATE_ERROR);
	}

	//テクスチャクラスへテクスチャとカラーキーを設定
	pClsTexture->SetTexturePointer(pTexture);
	pClsTexture->SetColorKey(colorKey);

	//テクスチャ管理クラスへ登録
	m_pClsTextureManage->AddTexture(strFileName, pClsTexture);
}

//=============================================================================
// 2D用テクスチャの解放（省メモリ対策）
// 引　数：string 画像のファイル名
//=============================================================================
void Engine::ReleaseTexture(std::string strFileName)
{
	m_pClsTextureManage->ReleaseTexture(strFileName);
}


//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw
//
// フォント
// 　フォントの追加と解放
//
//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw

//=============================================================================
// フォントの追加
// 引　数：string フォントサイズ＋フォント名（例 "40:ＭＳ ゴシック" ）
//=============================================================================
void Engine::AddFont(std::string strSizeAndFontName)
{
	assert(strSizeAndFontName.length() > 0);

	int index = strSizeAndFontName.find(':', 0);

	assert(index != (int)std::string::npos);

	//フォントサイズとフォント名を分ける

	int length = strSizeAndFontName.size();

	std::string strFontName = strSizeAndFontName.substr(index + 1, length - index);

	int nFontSize = atoi(strSizeAndFontName.substr(0, index).c_str());

	//登録済みならreturn
	if (m_pClsFontManage->SearchFont(strFontName)) {
		return;
	}

	LPD3DXFONT pFont = nullptr;

	//フォントの作成
	HRESULT hResult = D3DXCreateFont(m_pDevice, nFontSize, 0, FW_REGULAR, 10, false, SHIFTJIS_CHARSET, 0, 0, FIXED_PITCH | FF_MODERN, (LPCSTR)strFontName.c_str(), &pFont);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_FONT_CREATE_ERROR);
	}

	//フォントクラスの作成
	Font* pClsFont;

	try {
		pClsFont = new Font;
	}
	catch (std::bad_alloc) {
		throw DxSystemException(DxSystemException::OM_FONT_ALLOCATE_ERROR);
	}

	//フォントクラスへフォントとフォントサイズを設定
	pClsFont->SetFontPointer(pFont);
	pClsFont->SetFontSize(nFontSize);

	//フォント管理クラスへ登録
	m_pClsFontManage->AddFont(strSizeAndFontName, pClsFont);
}

//=============================================================================
// フォントの解放（省メモリ対策）
// 戻り値：成功 true、失敗 false
// 引　数：string フォント名
//=============================================================================
void Engine::ReleaseFont(std::string strFontName)
{
	assert(strFontName.length() > 0);

	m_pClsFontManage->ReleaseFont(strFontName);
}


//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw
//
// ＢＧＭ
// 　ＢＧＭ読み込み、再生、停止
//
//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw
	
//=============================================================================
// ＢＧＭの読み込み
// 引　数：strFileName ＢＧＭファイル名（midi,mp3...）
// 備　考：ＢＧＭ再生中に別のＢＧＭを読み込むと一瞬処理が停止するため、
// 　　　　プレイ中のＢＧＭ切り替えには注意すること。
//=============================================================================
void Engine::LoadBGM(std::string strFileName)
{
	assert(strFileName.length() > 0);

	m_pClsBGMManage->AddBGM(strFileName);
}

//=============================================================================
// ＢＧＭの解放（省メモリ対策）
//=============================================================================
void Engine::ReleaseBGM()
{
	m_pClsBGMManage->ReleaseBGM();
}
	
//=============================================================================
// メディアの再生
//=============================================================================
void Engine::PlayBGM()
{
	m_pClsBGMManage->PlayBGM();
}
	
//=============================================================================
// メディアの停止
//=============================================================================
void Engine::StopBGM()
{
	m_pClsBGMManage->StopBGM();
}
	
//=============================================================================
// ループ再生
// 備　考：ＢＧＭをループさせるためには、この関数を適宜呼び出す必要がある
//=============================================================================
void Engine::LoopBGM()
{
	m_pClsBGMManage->LoopBGM();
}


//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw
//
// ＳＥ
// 　ＳＥの追加、解放、再生、停止
//
//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw
	
//=============================================================================
// ＳＥの追加
// 引　数：ＳＥファイル名
//=============================================================================
void Engine::AddSE(std::string strFileName)
{
	m_pClsSEManage->AddSE(strFileName);
}

//=============================================================================
// ＳＥの削除（省メモリ対策）
// 引　数：string ＳＥファイル名
//=============================================================================
void Engine::ReleaseSE(std::string strFileName)
{
	m_pClsSEManage->ReleaseSE(strFileName);
}

#ifdef SE_USE_DMUSIC
//=============================================================================
// ＳＥの再生
// 引　数：string ＳＥファイル名
//=============================================================================
void Engine::PlaySE(std::string strFileName)
{
	m_pClsSEManage->PlaySE(strFileName);
}
#endif

#ifdef SE_USE_DSOUND
//=============================================================================
// ＳＥの再生
// 引　数：string ＳＥファイル名
// 　　　：DOWRD 優先度（0～0xffffffff）デフォルトは０
//=============================================================================
void Engine::PlaySE(std::string strFileName, DWORD priority)
{
	m_pClsSEManage->PlaySE(strFileName, priority);
}
#endif

//=============================================================================
// ＳＥの停止
// 引　数：string ＳＥファイル名
//=============================================================================
void Engine::StopSE(std::string strFileName)
{
	m_pClsSEManage->StopSE(strFileName);
}

//=============================================================================
// すべてのＳＥの停止
//=============================================================================
void Engine::StopAllSE()
{
	m_pClsSEManage->StopAllSE();
}

#ifdef SE_USE_DSOUND
//=============================================================================
// ＳＥの周波数をリセット
//=============================================================================
void Engine::SetSEStandardFrequency(std::string strFileName)
{
	m_pClsSEManage->SetStandardFrequency(strFileName);
}

//=============================================================================
// ＳＥの周波数を調整
// 引　数：int 増減させる周波数
//=============================================================================
void Engine::SetSEFrequency(std::string strFileName, int value)
{
	m_pClsSEManage->SetFrequency(strFileName, value);
}

//=============================================================================
// ＳＥのパンをリセット
//=============================================================================
void Engine::SetSEStandardPan(std::string strFileName)
{
	m_pClsSEManage->SetStandardPan(strFileName);
}

//=============================================================================
// ＳＥのパンを調整
// 引　数：int 増減させる数値
//=============================================================================
void Engine::SetSEPan(std::string strFileName, int value)
{
	m_pClsSEManage->SetPan(strFileName, value);
}

//=============================================================================
// ＳＥのボリュームをリセット
//=============================================================================
void Engine::SetSEStandardVolume(std::string strFileName)
{
	m_pClsSEManage->SetStandardVolume(strFileName);
}

//=============================================================================
// ＳＥのボリュームを調整
// 引　数：int 増減させる数値
//=============================================================================
void Engine::SetSEVolume(std::string strFileName, int value)
{
	m_pClsSEManage->SetVolume(strFileName, value);
}
#endif


//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw
//
// モデルデータ
// 　３Ｄモデル関係
//
//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw

//=============================================================================
// モデルデータの追加
// 引　数：string Ｘファイル名
// 　　　：bool   隣接データ作成フラグ（デフォルトはfalse）
// 備　考：追加するだけでは使用出来ない。GetModelを使って取得する必要がある。
//=============================================================================
void Engine::AddModel(std::string strFileName, bool bCreateAdjacencyData)
{
	m_pClsModelManage->AddModel(m_pDevice, strFileName, bCreateAdjacencyData);
}

//=============================================================================
// モデルデータの取得
// 戻り値：ModelBase* モデルベースクラスのポインタ
// 引　数：string Ｘファイル名
//=============================================================================
ModelBase* Engine::GetModel(std::string strFileName)
{
	return m_pClsModelManage->GetModel(strFileName);
}

//=============================================================================
// アニメーションコントローラーの取得
// 戻り値：ID3DXAnimationController* アニメーションコントローラーのポインタ
// 引　数：string Ｘファイル名
//=============================================================================
ID3DXAnimationController* Engine::GetAnimationController(std::string strFileName)
{
	return m_pClsModelManage->GetAnimationController(strFileName);
}

//=============================================================================
// モデルデータの解放
// 引　数：string Ｘファイル名
//=============================================================================
void Engine::ReleaseModel(std::string strFileName)
{
	m_pClsModelManage->ReleaseModel(strFileName);
}


//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw
//
// 入力
// 　マウス、キーボードからの入力
//
//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw

//=============================================================================
// 全ての入力デバイスの状態を更新する
//=============================================================================
void Engine::UpdateInputDeviceState()
{
	//キーボード
	m_pClsInputManage->UpdateKeyboardState();

	//マウス
	m_pClsInputManage->UpdateMouseState();

	//ゲームパッド
	m_pClsInputManage->UpdatePadState();
}

//=============================================================================
// キーボードの状態を更新する
//=============================================================================
void Engine::UpdateKeyboardState()
{
	m_pClsInputManage->UpdateKeyboardState();
}

//=============================================================================
// キー入力チェック（非同期：押しっぱなし）
// 戻り値：押下時true、それ以外false
	// 引　数：const BYTE DirectInputキーコード (DIK_??)
//=============================================================================
bool Engine::GetKeyState(const BYTE diKey) const
{
	return m_pClsInputManage->GetKeyboardState(diKey) != 0;
}

//=============================================================================
// キー入力チェック（同期：一度話さないと押せない）
// 戻り値：押下時true、それ以外false
// 引　数：const BYTE DirectInputキーコード (DIK_??)
//=============================================================================
bool Engine::GetKeyStateSync(const BYTE diKey) const
{
	return m_pClsInputManage->GetKeyboardStateSync(diKey) != 0;
}

//=============================================================================
// マウスの状態を更新する
//=============================================================================
void Engine::UpdateMouseState()
{
	m_pClsInputManage->UpdateMouseState();
}

//=============================================================================
// マウス座標の取得
// 戻り値：POINT構造体（クライアント座標）
//=============================================================================
POINT Engine::GetMousePosition() const
{
	POINT mousePos;

	GetCursorPos(&mousePos);

	ScreenToClient(m_hAppWnd, &mousePos);

	return mousePos;
}

//=============================================================================
// マウスの移動量を取得する
// 戻り値：POINT Ｘ,Ｙの移動量
//=============================================================================
POINT Engine::GetMouseMove() const
{
	return m_pClsInputManage->GetMouseMove();
}

//=============================================================================
// マウスのボタンの状態を取得（非同期：押しっぱなし）
// 戻り値：押下時true、それ以外false
// 引　数：const BYTE キーコード
// 　　　　列挙体として(DIK_LBUTTON, DIK_RBUTTON, DIK_MBUTTON)を用意してある
//=============================================================================
bool Engine::GetMouseButton(const BYTE diKey) const
{
	return m_pClsInputManage->GetMouseState(diKey) != 0;
}

//=============================================================================
// マウスのボタンの状態を取得（同期：一度離さないと押せない）
// 戻り値：押下時true、それ以外false
// 引　数：const BYTE キーコード
// 　　　　列挙体として(DIK_LBUTTON, DIK_RBUTTON, DIK_MBUTTON)を用意してある
//=============================================================================
bool Engine::GetMouseButtonSync(const BYTE diKey) const
{
	return m_pClsInputManage->GetMouseStateSync(diKey) != 0;
}

//=============================================================================
// マウスホイールの回転量を取得
// 戻り値：マウスホイールの回転量
// 備　考：回転方向↑　正の値を返す
// 　　　　回転方向↓　負の値を返す
// 　　　　回転していない場合は０を返す
//=============================================================================
int Engine::GetMouseWheelDelta() const
{
	return m_pClsInputManage->GetMouseWheelDelta();
}

//=============================================================================
// パッドが接続されているか確認する
// 戻り値：true 接続済み、false 接続されていない
//=============================================================================
bool Engine::IsConnectGamePad() const
{
	return m_pClsInputManage->IsConnectGamePad();
}

//=============================================================================
// 接続されているパッドの数を取得する
// 戻り値：int パッドの数
//=============================================================================
int Engine::GetConnectedGamePadNum() const
{
	return m_pClsInputManage->GetConnectedGamePadNum();
}

//=============================================================================
// ゲームパッドの状態を更新する
//=============================================================================
void Engine::UpdatePadState()
{
	m_pClsInputManage->UpdatePadState();
}

//=============================================================================
// パッドの状態を取得
// 戻り値：DIJOYSTATE パッドの状態
// 引　数：const unsigned int index ゲームパッド番号（０～）
//=============================================================================
DIJOYSTATE Engine::GetPadState(const unsigned int index)
{
	return m_pClsInputManage->GetPadState(index);
}

//=============================================================================
// パッドのボタンの状態を取得（非同期：押しっぱなし）
// 戻り値：押下時true、それ以外false
// 引　数：const unsigned int ゲームパッド番号（０～）
// 　　　　const unsigned int ボタン番号（０～３１）
//=============================================================================
bool Engine::GetPadButton(const unsigned int idxPad, const unsigned int idxButton) const
{
	return m_pClsInputManage->GetPadButtonState(idxPad, idxButton) != 0;
}

//=============================================================================
// パッドのボタンの状態を取得（同期：一度離さないと押せない）
// 戻り値：押下時true、それ以外false
// 引　数：const unsigned int ゲームパッド番号（０～）
// 　　　　const unsigned int ボタン番号（０～３１）
//=============================================================================
bool Engine::GetPadButtonSync(const unsigned int idxPad, const unsigned int idxButton) const
{
	return m_pClsInputManage->GetPadButtonStateSync(idxPad, idxButton) != 0;
}

//=============================================================================
// パッドの十字キーの状態を取得
// 戻り値：BYTE ボタンの状態
// 引　数：const unsigned int ゲームパッド番号（０～）
// 　　　　const unsigned int 方向（DIK_HORIZONTAL, DIK_VERTICAL）
//=============================================================================
int Engine::GetPadCrossKeyState(const unsigned int idxPad, const unsigned int direction) const
{
	return m_pClsInputManage->GetPadCrossKeyState(idxPad, direction);
}

//=============================================================================
// パッドの十字キーの状態を取得（同期：一度離さないと押せない）
// 戻り値：BYTE ボタンの状態
// 引　数：const unsigned int ゲームパッド番号（０～）
// 　　　　const unsigned int 方向（DIK_HORIZONTAL, DIK_VERTICAL）
//=============================================================================
int Engine::GetPadCrossKeyStateSync(const unsigned int idxPad, const unsigned int direction)
{
	return m_pClsInputManage->GetPadCrossKeyStateSync(idxPad, direction);
}

//=============================================================================
// パッドのアナログスティック（左）の状態を取得
// 戻り値：double ボタンの状態（－１～＋１）
// 引　数：const unsigned int ゲームパッド番号（０～）
// 　　　　const unsigned int 方向（DIK_HORIZONTAL, DIK_VERTICAL）
//=============================================================================
double Engine::GetPadLeftAnalogStickState(const unsigned int idxPad, const unsigned int direction) const
{
	return m_pClsInputManage->GetPadLeftAnalogStickState(idxPad, direction);
}

//=============================================================================
// パッドのアナログスティック（右）の状態を取得
// 戻り値：double ボタンの状態（－１～＋１）
// 引　数：const unsigned int ゲームパッド番号（０～）
// 　　　　const unsigned int 方向（DIK_HORIZONTAL, DIK_VERTICAL）
//=============================================================================
double Engine::GetPadRightAnalogStickState(const unsigned int idxPad, const unsigned int direction) const
{
	return m_pClsInputManage->GetPadRightAnalogStickState(idxPad, direction);
}


//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw
//
// スクリーンショット
// 　CommonConstant.hで指定したフォルダに画像を保存
// 　ファイル名は「年月日時分秒.bmp」
// 　例）2011年11月15日13時9分13秒に保存　→　20111115130913.bmp
//
//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw

//=============================================================================
// スクリーンショットの保存
//=============================================================================
void Engine::ScreenShot(std::string fileName)
{
	//オフスクリーンサーフェスの作成
	LPDIRECT3DSURFACE9 pSurface = nullptr;

	HRESULT hResult = m_pDevice->CreateOffscreenPlainSurface(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &pSurface, nullptr);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_SS_CANTCREATE_OFFSCREENSURFACE_ERROR);
	}

	//フロントバッファのデータ（映っている画像）の取得
	hResult = m_pDevice->GetFrontBufferData(0, pSurface);

	if (FAILED(hResult)) {
		if (pSurface) { pSurface->Release(); pSurface = nullptr; }
		throw DxSystemException(DxSystemException::OM_SS_CANTGET_FRONTBUFFER_ERROR);
	}

	RECT rect;

	GetWindowRect(m_hAppWnd, &rect);

	//ウィンドウモードの場合、画面に相当する箇所を指定する必要がある
	if (m_bWindowed) {
		int cxFrame = GetSystemMetrics(SM_CXFRAME);
		int cyCaption = GetSystemMetrics(SM_CYCAPTION);
		int cyFrame = GetSystemMetrics(SM_CYFRAME);
		rect.left += cxFrame * 2;
		rect.right -= cxFrame * 2;
		rect.top += cyCaption + cyFrame * 2;
		rect.bottom -= cyFrame * 2;
	}

	//スクリーンショットの保存
	hResult = D3DXSaveSurfaceToFile((LPCSTR)fileName.c_str(), D3DXIFF_BMP, pSurface, nullptr, &rect);

	if (FAILED(hResult)) {
		if (pSurface) { pSurface->Release(); pSurface = nullptr; }
		throw DxSystemException(DxSystemException::OM_SS_CANT_SAVE_ERROR);
	}

	if (pSurface) {
		pSurface->Release();
		pSurface = nullptr;
	}
}


//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw
//
// レンダリングステート関連
// 　レンダリングステート変更用
//
//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw

//=============================================================================
// ワイヤフレームモードＯＮ
//=============================================================================
void Engine::WireframeON()
{
	m_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
}

//=============================================================================
// ワイヤフレームモードＯＦＦ
//=============================================================================
void Engine::WireframeOFF()
{
	m_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
}

//=============================================================================
// バックフェースカリングＯＮ
//=============================================================================
void Engine::CullingON()
{
	m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
}

//=============================================================================
// バックフェースカリングＯＦＦ
//=============================================================================
void Engine::CullingOFF()
{
	m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
}


//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw
//
// 直線の描画
// 　レイなどの確認用
//
//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw

//=============================================================================
// 直線の描画（２Ｄ）
// 引　数：開始位置（２Ｄベクトル）
// 　　　　終了位置（２Ｄベクトル）
// 　　　　線の色
//=============================================================================
void Engine::DrawLine(const D3DXVECTOR2 startPosition, const D3DXVECTOR2 endPosition, D3DCOLOR color)
{
	D3DXVECTOR2 linePos[2] = { startPosition, endPosition };

	m_pClsLine->Draw(linePos, 2, color);
}

//=============================================================================
// 直線の描画（２Ｄ）
// 引　数：開始Ｘ座標、Ｙ座標
// 　　　　終了Ｘ座標、Ｙ座標
// 　　　　線の色
//=============================================================================
void Engine::DrawLine(const int startX, const int startY, const int endX, const int endY, D3DCOLOR color)
{
	D3DXVECTOR2 startPosition((float)startX, (float)startY);
	D3DXVECTOR2 endPosition((float)endX, (float)endY);

	D3DXVECTOR2 linePos[2] = { startPosition, endPosition };

	m_pClsLine->Draw(linePos, 2, color);
}

//=============================================================================
// 直線の描画（３Ｄ）
// 引　数：開始位置（３Ｄベクトル）
// 　　　　終了位置（３Ｄベクトル）
// 　　　　線の色
// 　　　　ビュー変換行列のポインタ
// 　　　　プロジェクション変換行列のポインタ
//=============================================================================
void Engine::DrawLine(const D3DXVECTOR3 startPosition, const D3DXVECTOR3 endPosition, D3DCOLOR color, const D3DXMATRIX* pView, const D3DXMATRIX* pProjection)
{
	m_pClsLine->Draw(startPosition, endPosition, color, pView, pProjection);
}

//=============================================================================
// 円の描画
// 引　数：中心座標（２Ｄベクトル）
// 　　　　半径（スクリーン座標）
// 　　　　分割数（３６０が割り切れる値）
// 　　　　線の色
//=============================================================================
void Engine::DrawCircle(const D3DXVECTOR2 &center, const float radius, const int segment, const D3DCOLOR color)
{
	D3DXVECTOR2 dir1(1, 0);
	D3DXVECTOR2 dir2;

	float angle = D3DXToRadian(360.0f / segment);

	dir1 *= radius;

	for (int i = 0; i < segment; i++) {

		dir2 = D3DXVec2Rotate(&dir1, angle);

		DrawLine(center + dir1, center + dir2, color);

		dir1 = dir2;
	}
}


#ifdef USE_IMGUI
//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw
//
// 日本語入力
// 　ImGui(Immediate Mode GUI)を利用した日本語入力
//
//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw

//=============================================================================
// ImGuiによるフレーム処理開始
// 引　数：位置（左上の座標）
// 　　　　サイズ（幅、高さ）
// 　　　　フラグ（デフォルトはタイトルなし、リサイズなし、移動なし）
//=============================================================================
void Engine::ImGuiBegin(ImVec2 position, ImVec2 size, ImGuiWindowFlags flags)
{
	ImGui::SetNextWindowPos(position);
	ImGui::SetNextWindowSize(size);

	bool bShow = true;

	ImGui::Begin(u8"ImGui Window", &bShow, flags);
}

//=============================================================================
// ImGuiによるフレーム処理終了
//=============================================================================
void Engine::ImGuiEnd()
{
	ImGui::End();
}

//=============================================================================
// ImGuiによる文字入力
// 戻り値：入力完了
// 引　数：文字列の参照
//=============================================================================
bool Engine::ImGuiInput(std::string caption, std::string& input)
{
	std::string str = "";
	static std::string save_str = "";
	
	ImGui::Text(caption.c_str());

	if (ImGui::GetKeyPressedAmount(VK_RETURN, 0, 0))
		ImGui::SetKeyboardFocusHere();

	ImGui::InputText(u8"", &str);

	////UTF8 -> SJISに変えなければゲーム中で表示できない
	str = UTF8toSjis(str);

	if (str != save_str) {

		if (str == "") {
			OutputDebugString("Enter : ");
			OutputDebugString(save_str.c_str());
			OutputDebugString("\n");
			if (save_str != "") {
				input = save_str;
				return true;
			}
		}

		save_str = str;
	}

	ImGui::SameLine();

	ImGui::Button(u8"ＯＫ");

	return false;
}
#endif

//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw
//
// その他
//
//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw

//=============================================================================
// アプリケーションのウインドウハンドルを取得
//=============================================================================
HWND Engine::GetWindowHandle() const
{
	return m_hAppWnd;
}

//=============================================================================
// アプリケーションのインスタンスハンドルを取得
//=============================================================================
HINSTANCE Engine::GetInstanceHandle() const
{
	return m_hInstance;
}

//=============================================================================
// バックバッファを取得
//=============================================================================
LPDIRECT3DSURFACE9 Engine::GetBackBuffer() const
{
	return m_pBackBuffer;
}

//=============================================================================
// ＦＰＳの値を取得
//=============================================================================
DWORD Engine::GetFPS() const
{
	return m_pClsFpsManage->GetFPS();
}















