//*****************************************************************************
//
// ゲームクリア
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "SceneClear.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;

//=============================================================================
// コンストラクタ
// 引　数：Engine* エンジンクラスのアドレス
//=============================================================================
SceneClear::SceneClear(Engine *pEngine)
	: Scene(pEngine)
{

}

//=============================================================================
// デストラクタ
//=============================================================================
SceneClear::~SceneClear()
{
	Exit();
}

//=============================================================================
// シーンの実行時に１度だけ呼び出される開始処理関数
//=============================================================================
void SceneClear::Start()
{
	m_buttons.clear();
	m_buttons.push_back(RESTART_BUTTON);
	m_buttons.push_back(TITLE_BUTTON);
	m_buttons.push_back(EXIT_BUTTON);

	SoundManager::Play(AK::EVENTS::PLAY_BGM_CLEAR, ID_BGM);

	m_nowSceneNumber = m_nowSceneData.GetNowScene();
	Initialize(m_pEngine);
	m_lastTime = timeGetTime();
}

//=============================================================================
// シーンの実行時に繰り返し呼び出される更新処理関数
//=============================================================================
void SceneClear::Update()
{
	DWORD nowTime = timeGetTime();
	float deltaTime = (nowTime - m_lastTime) / 1000.0f;
	m_lastTime = nowTime;

	if (!MenuManager::Update(m_pEngine, m_gameData, deltaTime)) return;

	if (m_gameData.m_nextSceneNumber == Common::RESTART) m_gameData.m_nextSceneNumber = Common::SCENE_GAME;
	m_nowSceneData.Set(m_gameData.m_nextSceneNumber, false, nullptr);
}

//=============================================================================
// シーンの実行時に繰り返し呼び出される描画処理関数
//=============================================================================
void SceneClear::Draw()
{
	m_pEngine->SpriteBegin();
	MenuManager::Draw(m_pEngine, m_gameData);
	m_pEngine->SpriteEnd();
}

//=============================================================================
// シーンの実行時に繰り返し呼び出されるポストエフェクト関数
//=============================================================================
void SceneClear::PostEffect()
{

}

//=============================================================================
// シーンの終了時に呼び出される終了処理関数
//=============================================================================
void SceneClear::Exit()
{
	SoundManager::StopAll(ID_BGM);
	Release(m_pEngine);
}

#ifdef USE_IMGUI
//=============================================================================
// 日本語入力用
//=============================================================================
void SceneTitle::ImGuiFrameProcess()
{

}
#endif
