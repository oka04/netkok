//*****************************************************************************
//
// ゲームオーバー
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "SceneGameover.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;

SceneGameover::SceneGameover(Engine *pEngine)
	: Scene(pEngine)
{

}

SceneGameover::~SceneGameover()
{
	Exit();
}

void SceneGameover::Start()
{
	m_buttons.clear();
	m_buttons.push_back(RESTART_BUTTON);
	m_buttons.push_back(TITLE_BUTTON);
	m_buttons.push_back(EXIT_BUTTON);

	SoundManager::Play(AK::EVENTS::PLAY_BGM_GAMEOVER, ID_BGM);

	m_nowSceneNumber = m_nowSceneData.GetNowScene();
	Initialize(m_pEngine);
	m_lastTime = timeGetTime();
}

void SceneGameover::Update()
{
	DWORD nowTime = timeGetTime();
	float deltaTime = (nowTime - m_lastTime) / 1000.0f;
	m_lastTime = nowTime;

	if (!MenuManager::Update(m_pEngine, m_gameData, deltaTime)) return;


	if (m_gameData.m_nextSceneNumber == Common::RESTART) m_gameData.m_nextSceneNumber = Common::SCENE_GAME;
	m_nowSceneData.Set(m_gameData.m_nextSceneNumber, false, nullptr);
}

void SceneGameover::Draw()
{
	m_pEngine->SpriteBegin();
	MenuManager::Draw(m_pEngine, m_gameData);
	m_pEngine->SpriteEnd();
}

void SceneGameover::PostEffect()
{

}

void SceneGameover::Exit()
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
