//*****************************************************************************
//
// タイトルシーン
//
//*****************************************************************************

#pragma once

#include "..\\..\\GameBase.h"

#include "..\\Scene\\Scene.h"
#include "..\\..\\Object\\MenuManager\\MenuManager.h"

#include "..\\..\\Object\\Map\\Map.h"
#include "..\\..\\Object\\Patroller\\PatrollerManager.h"
#include "..\\Scene\\SceneLobby\\SceneLobby.h" 
class SceneTitle : public Scene, MenuManager
{
public:

	//=============================================================================
	// コンストラクタ
	// 引　数：Engine* エンジンクラスのアドレス
	//=============================================================================
	SceneTitle(Engine *pEngine);

	//=============================================================================
	// デストラクタ
	//=============================================================================
	~SceneTitle();

	//=============================================================================
	// シーンの実行時に１度だけ呼び出される開始処理関数
	//=============================================================================
	void Start();

	//=============================================================================
	// シーンの実行時に繰り返し呼び出される更新処理関数
	//=============================================================================
	void Update();

	//=============================================================================
	// シーンの実行時に繰り返し呼び出される描画処理関数
	//=============================================================================
	void Draw();

	//=============================================================================
	// シーンの実行時に繰り返し呼び出されるポストエフェクト関数
	//=============================================================================
	void PostEffect();

	//=============================================================================
	// シーンの終了時に呼び出される終了処理関数
	//=============================================================================
	void Exit();

#ifdef USE_IMGUI
	//=============================================================================
	// 日本語入力用
	//=============================================================================
	void ImGuiFrameProcess();
#endif

private:
	// プレイヤー名関連
	std::string m_playerName;
	int m_cursorPos;
	bool m_bInputActive;
	bool m_cursorVisible;
	float m_playerTimer;
	DWORD m_lastTime;
	std::string f_playerNameLabelText;
	
	IntVector2 f_playerNameLabelPos;
	IntVector2 f_playerNameInputPos;
	int f_playerNameMaxCount;
	float f_clickMargin;
	int m_playerTextWidth;
	std::vector<int> m_playerCharCumulative;
	int m_playerMaxWidth;

	// プレイヤー名自動生成用の定数
	const char* f_defaultPlayerNamePrefix;
	int f_randomNumberDigits;
	int f_randomNumberMin;
	int f_randomNumberMax;
	const char* f_emptyNamePlaceholder;

	void DrawPlayerNameInput();
	void UpdatePlayerNameInput();
	void GenerateDefaultPlayerName();

	float m_deltaTime;
	Camera m_camera;
	Projection m_projection;
	Viewport m_viewport;
	AmbientLight m_ambient;
	DirectionalLight m_light;
	Map m_map;
	PatrollerManager m_patrollerManager;
};