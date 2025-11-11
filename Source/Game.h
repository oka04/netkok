//*****************************************************************************
//
// ゲームクラス
//
// Game.h
//
// K_Yamaoka
//
// 2013/03/04
//
// 2016/11/14 Initialize関数の引数hAppWndを参照へ、hInstanceを追加
//
//*****************************************************************************

#pragma once
#include "GameBase.h"

//-----------------------------------------------------------------------------
//シーンクラスのヘッダファイルをインクルードします。
//-----------------------------------------------------------------------------

#include "Scene\\Scene\Scene.h"
#include "Scene\\SceneLobby\\SceneLobby.h"
#include "Scene\\SceneInit\\SceneInit.h"
#include "Scene\\SceneTitle\\SceneTitle.h"
#include "Scene\\SceneGame\\SceneGame.h"
#include "Scene\\ScenePause\\ScenePause.h"
#include "Scene\\SceneClear\\SceneClear.h"
#include "Scene\\SceneSelectClient\\SceneSelectClient.h"
#include "Scene\\SceneSelectHost\\SceneSelectHost.h"
#include "Scene\\SceneGameover\\SceneGameover.h"

class Engine;

class Game
{
public:
	
	//=============================================================================
	// コンストラクタ
	//=============================================================================
	Game();

	//=============================================================================
	// デストラクタ
	//=============================================================================
	~Game();

	//=============================================================================
	// ゲームの初期化
	// 引　数：const HWND & ウィンドウハンドル
	// 　　　　const HINSTANCE & インスタンスハンドル
	//=============================================================================
	void Initialize(const HWND &hAppWnd, const HINSTANCE &hInstance);

	//=============================================================================
	// 実行
	//=============================================================================
	bool Run();

private:
	
	//３Ｄエンジン
	Engine* m_pEngine;
	
	//現在のシーンクラスへのポインタ
	Scene *m_pNowScene;

	//-----------------------------------------------------------------------------
	// シーン切り替え
	//-----------------------------------------------------------------------------
	bool ChangeScene();

	//コピー防止
	Game( const Game& );
	Game& operator=( const Game& );
};























