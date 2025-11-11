//*****************************************************************************
//
// ゲームシーン
//
//*****************************************************************************

#pragma once

#include "..\\..\\GameBase.h"

#include "..\\Scene\\Scene.h"

#include "..\\..\\Object\\Fade\\Fade.h"
#include "..\\..\\Object\\Map\\Map.h"
#include "..\\..\\Object\\Player\\Player.h"
#include "..\\..\\Object\\Patroller\\PatrollerManager.h"

class SceneGame : public Scene
{
public:

	//=============================================================================
	// コンストラクタ
	// 引　数：Engine* エンジンクラスのアドレス
	//=============================================================================
	SceneGame(Engine *pEngine);

	//=============================================================================
	// デストラクタ
	//=============================================================================
	~SceneGame();

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
	void Initialize();
	void UpdateDebugFlag();

	enum DEBUG_FLAG 
	{
		DRAW_PLAYER_STATE = 1 << 0,
		DRAW_BOXLINE = 1 << 1,
		RELOAD_FILE = 1 << 2,
		PATROLLER_VIEW_LINE = 1 << 3,
		DISPLAY_DEBUG_STRING = 1 << 4,
		STOP_GAME = 1 << 5,
		DEBUG_MODE = 1 << 6,
	};

	enum VIEW_KIND
	{
		VIEW_GAME,
		VIEW_FIRST,
		VIEW_THIRD,
		VIEW_MAX
	};

	enum GAME_STATE
	{
		FADE_IN,
		IN_GAME,
		CHANGE_SCENE,
		GAMEOVER_ROTATION,
		FADE_OUT,
	};

	unsigned char d_debugFlag;

	int d_fpsCount;
	int d_viewPointCount;
	int m_gameState;
	float m_deltaTime;
	float f_miniMapSourHalfSize;
	DWORD m_lastTime;
	

	Camera m_camera;
	Projection m_projection;
	Viewport m_viewport;
	AmbientLight m_ambient;
	DirectionalLight m_light;
	Map m_map;
	Player m_player;
	Fade m_fade;
	PatrollerManager m_patrollerManager;

	D3DXVECTOR3 m_outPatrollerPosition;
};
