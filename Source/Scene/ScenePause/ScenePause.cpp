#define _USING_V110_SDK71_ 1

#include "ScenePause.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;
ScenePause::ScenePause(Engine *pEngine)
	: Scene(pEngine)
{

}
ScenePause::~ScenePause()
{
	Exit();
}

void ScenePause::Start()
{
	m_buttons.clear();
	m_buttons.push_back(BACK_BUTTON);
	m_buttons.push_back(RESTART_BUTTON);
	m_buttons.push_back(OPERATION_BUTTON);
	m_buttons.push_back(TITLE_BUTTON);

	SoundManager::Play(AK::EVENTS::PLAY_SE_PAUSE, ID_UI);
	m_nowSceneNumber = m_nowSceneData.GetNowScene();
	Initialize(m_pEngine);
	m_lastTime = timeGetTime();
}

void ScenePause::Update()
{
	DWORD nowTime = timeGetTime();
	float deltaTime = (nowTime - m_lastTime) / 1000.0f;
	m_lastTime = nowTime;

	if (!MenuManager::Update(m_pEngine, m_gameData, deltaTime)) return;

	m_nowSceneData.Set(Common::SCENE_GAME, false, nullptr);
}

void ScenePause::Draw()
{
	m_pEngine->SpriteBegin();
	MenuManager::Draw(m_pEngine, m_gameData);
	m_pEngine->SpriteEnd();
}

void ScenePause::PostEffect()
{

}

void ScenePause::Exit()
{
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
