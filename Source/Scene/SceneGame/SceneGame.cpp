#define _USING_V110_SDK71_ 1

#include "SceneGame.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;

using namespace std;

SceneGame::SceneGame(Engine *pEngine)
	: Scene(pEngine)

{

}
SceneGame::~SceneGame()
{
	Exit();
}
void SceneGame::Start()
{
	m_pEngine->AddFont(FONT_GOTHIC40);

	m_camera.m_vecEye = D3DXVECTOR3(50.0f, 150.0f, 50.0f);
	m_camera.m_vecAt = D3DXVECTOR3(50.0f, 0.0f, 50.0f);
	m_camera.m_vecUp = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	m_camera.RecalculateUpDirection();
	m_camera.SetDevice(m_pEngine);

	m_projection.SetData(D3DXToRadian(90.0f), 4.0f / 3.0f, 0.1f, 100.0f);
	m_projection.SetDevice(m_pEngine);
	
	m_viewport.Add(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0f, 1.0f);
	m_viewport.SetDevice(m_pEngine, 0);

	m_ambient.SetColor(0.7f, 0.1f, 0.1f, 0.1f);
	m_light.SetDiffuse(0.1f, 0.1f, 0.1f, 0.1f);
	D3DXVECTOR3 direction;
	direction = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
	direction = D3DXVec3Normalize(&direction);
	m_light.SetDirection(direction);

	while (ShowCursor(FALSE) >= 0);
	Initialize();
}

void SceneGame::Update()
{
	DWORD nowTime = timeGetTime();
	m_deltaTime = (nowTime - m_lastTime) / 1000.0f;
	m_lastTime = nowTime;

#if _DEBUG
	UpdateDebugFlag();
	if (d_debugFlag & STOP_GAME)return;
#endif

	switch (m_gameState)
	{
	case IN_GAME:
		m_gameData.m_gameTime += m_deltaTime;
		m_map.UpdateGoalEffect();
		m_player.Update(m_pEngine, m_map, m_camera, m_light, m_deltaTime);
		if (m_map.CheckGoal(m_player.GetPosition()) && !(d_debugFlag & DEBUG_MODE))
		{
			m_gameState = FADE_OUT;
			m_fade.SetFadeOut();
			m_gameData.m_nextSceneNumber = SCENE_CLEAR;
		}

		//敵の更新
		if (m_patrollerManager.Update(m_pEngine, m_map, m_player.GetPosition(), &m_outPatrollerPosition, m_deltaTime))
		{
			//ゲームオーバー
			if (!(d_debugFlag & DEBUG_MODE))
			{
				m_gameState = GAMEOVER_ROTATION;
				SoundManager::StopAll(ID_BGM);
				m_patrollerManager.ReleaseSE();
			}
		}
		break;

	case CHANGE_SCENE:
		//シーン変更があった場合の処理
		m_lastTime = timeGetTime();
		while (ShowCursor(FALSE) >= 0);

		switch (m_gameData.m_nextSceneNumber)
		{
		case Common::RESTART:
			SoundManager::StopAll(ID_BGM);
			m_patrollerManager.ReleaseSE();
			Initialize();
			break;
		case Common::SCENE_GAME:
			m_gameState = IN_GAME;
			break;
		default:
			SoundManager::StopAll(ID_BGM);
			m_patrollerManager.ReleaseSE();
			m_nowSceneData.Set(m_gameData.m_nextSceneNumber, false, nullptr);
			break;
		}

	case FADE_IN:
		if (m_fade.Update(m_deltaTime))
		{
			m_gameState = IN_GAME;
		}
		break;

		//フェードアウトとゲームオーバーの回転の処理をしたらそのまま終了する
	case FADE_OUT:
		if (m_fade.Update(m_deltaTime))
		{
			m_gameData.m_alertCount = m_patrollerManager.GetAlertCount();
			m_nowSceneData.Set(m_gameData.m_nextSceneNumber, false, nullptr);
		}
		return;

	case GAMEOVER_ROTATION:
		if (m_gameState == GAMEOVER_ROTATION && !(d_debugFlag & DEBUG_MODE))
		{
			if (m_player.RotateToTarget(m_light, m_outPatrollerPosition, m_deltaTime))
			{
				m_gameState = FADE_OUT;
				m_gameData.m_nextSceneNumber = SCENE_GAMEOVER;
				m_fade.SetFadeOut();
				SoundManager::Play(AK::EVENTS::PLAY_SE_GAMEOVER, ID_PALYER);
			}
			m_player.SetFirstPersonCamera(m_pEngine, m_camera);
		}			
		return;
	}

	//ポーズ画面
	if (m_pEngine->GetKeyStateSync(DIK_ESCAPE) || m_pEngine->GetKeyStateSync(DIK_P))
	{
		m_pEngine->ScreenShot(TEXTURE_PAUSE);
		m_gameState = CHANGE_SCENE;
		m_nowSceneData.Set(Common::SCENE_PAUSE, true, this);
		return;
	}

#if _DEBUG
	if (d_debugFlag & RELOAD_FILE)
	{
		Exit();//マップなどを再読み込みするためモデルなどはいったんリリースさせる
		Initialize();
	}

	switch (d_viewPointCount)
	{
	case VIEW_GAME:break;
	case VIEW_FIRST:
		m_player.SetFirstPersonCamera(m_pEngine, m_camera);
		break;
	case VIEW_THIRD:
		m_player.SetThirdPersonFromBehind(m_pEngine, m_camera, m_map);
		break;
	}
#endif
}

void SceneGame::Draw()
{
	vector<SpotLight>* lights = m_patrollerManager.GetLights(m_player.GetPosition(), m_player.GetDepth(), m_player.GetFov());

	m_map.DrawMap(m_pEngine, &m_camera, &m_projection, &m_ambient, &m_light, lights);
	m_map.DrawGoalEffect(&m_camera, &m_projection);
	m_player.Draw(&m_camera, &m_projection, &m_ambient, &m_light); 
	m_patrollerManager.Draw(m_pEngine, &m_camera, &m_projection, &m_ambient, &m_light);

	if (d_debugFlag & DRAW_BOXLINE) 
	{
		m_map.DebugBoxLine(m_pEngine, &m_camera, &m_projection);
	}

	if (d_debugFlag & PATROLLER_VIEW_LINE) 
	{
		m_patrollerManager.DebugViewLine(m_pEngine, &m_camera, &m_projection);
	}

	m_map.DrawMiniMap(m_pEngine, m_player.GetPosition2D(), m_player.GetArrowAngle(), m_patrollerManager);

	m_pEngine->SpriteBegin();

	m_patrollerManager.DrawHeartBeat(m_pEngine, m_player.GetPosition());
	m_player.DrawStaminaGauge(m_pEngine);

#if _DEBUG
	//画面上の文字が表示状態ならデバッグ用のテキストを表示する
	if (!(d_debugFlag & DISPLAY_DEBUG_STRING))
	{
		m_pEngine->DrawPrintf(50, 1000, FONT_GOTHIC40, Color::WHITE, "%f", (float)m_pEngine->GetFPS());

		if (d_debugFlag & DRAW_PLAYER_STATE) 
		{
			m_player.DebugPrint(m_pEngine);
			m_patrollerManager.DebugPrint(m_pEngine);
		}

		if (m_gameState >= GAMEOVER_ROTATION)m_pEngine->DrawPrintf(1300, 30, FONT_GOTHIC40, Color::BLUE, "ゲームステータス: GameOver");
		else m_pEngine->DrawPrintf(1300, 30, FONT_GOTHIC40, Color::BLUE, "ゲームステータス: In Game");
		m_pEngine->DrawPrintf(1300, 80, FONT_GOTHIC40, Color::BLUE, "F1: プレイヤーのステータス表示");
		m_pEngine->DrawPrintf(1300, 130, FONT_GOTHIC40, Color::BLUE, "F2: マップのボックスライン表示");
		m_pEngine->DrawPrintf(1300, 180, FONT_GOTHIC40, Color::BLUE, "F3: プレイヤーの視点変更");
		switch (d_viewPointCount)
		{
		case VIEW_GAME:
			m_pEngine->DrawPrintf(1400, 230, FONT_GOTHIC40, Color::BLUE, "現在の視点：ゲーム画面");
			break;
		case VIEW_FIRST:
			m_pEngine->DrawPrintf(1400, 230, FONT_GOTHIC40, Color::BLUE, "現在の視点：一人称固定");
			break;
		case VIEW_THIRD:
			m_pEngine->DrawPrintf(1400, 230, FONT_GOTHIC40, Color::BLUE, "現在の視点：三人称固定");
			break;
		}
		m_pEngine->DrawPrintf(1300, 300, FONT_GOTHIC40, Color::BLUE, "F4: ファイルデータの再読み込み");
		m_pEngine->DrawPrintf(1300, 350, FONT_GOTHIC40, Color::BLUE, "F5: 敵の視線の表示");

		if (d_debugFlag & STOP_GAME)m_pEngine->DrawPrintf(1300, 400, FONT_GOTHIC40, Color::BLUE, "F6: 一時停止解除");
		else m_pEngine->DrawPrintf(1300, 400, FONT_GOTHIC40, Color::BLUE, "F6: 一時停止");

		m_pEngine->DrawPrintf(1300, 450, FONT_GOTHIC40, Color::BLUE, "F7:ゲームモード変更");
		if (d_debugFlag & DEBUG_MODE)m_pEngine->DrawPrintf(1300, 500, FONT_GOTHIC40, Color::BLUE, "現在: デバッグモード");
		else m_pEngine->DrawPrintf(1300, 500, FONT_GOTHIC40, Color::BLUE, "現在 : ゲームモード");

		m_pEngine->DrawPrintf(1400, 1000, FONT_GOTHIC40, Color::BLUE, "F11: 画面の文字を非表示");
	}
	else 
	{
		m_pEngine->DrawPrintf(1400, 1000, FONT_GOTHIC40, Color::BLUE, "F11: 画面の文字を表示");
	}
#endif
	m_fade.Draw(m_pEngine);
	m_pEngine->SpriteEnd();
}
 

void SceneGame::PostEffect()
{

}

void SceneGame::Exit()
{
	m_patrollerManager.Release(m_pEngine);
	m_map.Release(m_pEngine);
	m_player.Release(m_pEngine);
	m_fade.Release(m_pEngine);
	m_pEngine->ReleaseFont(FONT_GOTHIC40); 
	m_pEngine->ReleaseModel(MODEL_CHARACTER);
}

#ifdef USE_IMGUI
//=============================================================================
// 日本語入力用
//=============================================================================
void SceneGame::ImGuiFrameProcess()
{

}
#endif

void SceneGame::Initialize()
{
	SetBackColor(0x00000000);
	d_debugFlag = 0;
	d_viewPointCount = 0;
	d_fpsCount = 60;
	m_pEngine->AddModel(MODEL_CHARACTER);

	m_gameData.m_alertCount = 0;
	m_gameData.m_gameTime = 0;

	m_fade.Initialize(m_pEngine);
	m_gameState = FADE_IN;
	m_fade.SetFadeIn();
	m_patrollerManager.Initialize(m_pEngine, m_nowSceneData.GetNowScene());

	Camera miniMapCamera = m_camera;
	m_map.Initialize(m_pEngine, m_patrollerManager, &miniMapCamera, &m_projection, &m_ambient, &m_light, 1);
	m_player.Initialize(m_pEngine, m_map, &m_projection, m_camera, m_light);
	m_lastTime = timeGetTime();

	//カメラの位置をただすために一度描画する
	m_player.Update(m_pEngine, m_map, m_camera, m_light, 0);
	m_player.SetFirstPersonCamera(m_pEngine, m_camera);

	SoundManager::Play(AK::EVENTS::PLAY_BGM_GAME, ID_BGM);
	m_pEngine->AddFont(FONT_GOTHIC40);
}

void SceneGame::UpdateDebugFlag()
{
	if (m_pEngine->GetKeyStateSync(DIK_F1))
	{
		d_debugFlag ^= DRAW_PLAYER_STATE;
	}

	if (m_pEngine->GetKeyStateSync(DIK_F2))
	{
		d_debugFlag ^= DRAW_BOXLINE;
	}

	if (m_pEngine->GetKeyStateSync(DIK_F3))
	{
		d_viewPointCount = (d_viewPointCount + 1) % VIEW_MAX;
	}

	if (m_pEngine->GetKeyStateSync(DIK_F4)) 
	{
		d_debugFlag |= RELOAD_FILE;
	}
	else {
		d_debugFlag &= ~RELOAD_FILE;
	}

	if (m_pEngine->GetKeyStateSync(DIK_F5)) 
	{
		d_debugFlag ^= PATROLLER_VIEW_LINE;
	}

	if (m_pEngine->GetKeyStateSync(DIK_F6)) 
	{
		d_debugFlag ^= STOP_GAME;
	}

	if (m_pEngine->GetKeyStateSync(DIK_F7))
	{
		d_debugFlag ^= DEBUG_MODE;
	}
	
	if (m_pEngine->GetKeyStateSync(DIK_F11)) 
	{
		d_debugFlag ^= DISPLAY_DEBUG_STRING;
	}
}