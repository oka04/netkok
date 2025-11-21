#define _USING_V110_SDK71_ 1

#include "SceneGame.h"
#include "..\\..\\Object\\Network\\NetworkLogger.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;
using namespace std;

enum NetworkMessage : uint8_t
{
	MSG_PLAYER_UPDATE = 10,
};

SceneGame::SceneGame(Engine *pEngine)
	: Scene(pEngine)
	, m_pLocalPlayer(nullptr)
	, m_localClientId(0)
	, m_bIsHost(false)
	, m_lastNetworkUpdate(0)
	, m_pClient(nullptr)
	, m_pServer(nullptr)
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
	D3DXVECTOR3 direction = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
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
	if (d_debugFlag & STOP_GAME) return;
#endif

	switch (m_gameState)
	{
	case IN_GAME:
		m_gameData.m_gameTime += m_deltaTime;
		m_map.UpdateGoalEffect();

		UpdateNetwork();

		if (m_pLocalPlayer)
		{
			m_pLocalPlayer->Update(m_pEngine, m_map, m_camera, m_light, m_deltaTime);

			if (nowTime - m_lastNetworkUpdate > 50)
			{
				SyncPlayers();
				m_lastNetworkUpdate = nowTime;
			}

			if (m_map.CheckGoal(m_pLocalPlayer->GetPosition()) && !(d_debugFlag & DEBUG_MODE))
			{
				m_gameState = FADE_OUT;
				m_fade.SetFadeOut();
				m_gameData.m_nextSceneNumber = SCENE_CLEAR;
			}
		}
		break;

	case CHANGE_SCENE:
		m_lastTime = timeGetTime();
		while (ShowCursor(FALSE) >= 0);

		switch (m_gameData.m_nextSceneNumber)
		{
		case Common::RESTART:
			SoundManager::StopAll(ID_BGM);
			Initialize();
			break;
		case Common::SCENE_GAME:
			m_gameState = IN_GAME;
			break;
		default:
			SoundManager::StopAll(ID_BGM);
			m_nowSceneData.Set(m_gameData.m_nextSceneNumber, false, nullptr);
			break;
		}
		break;

	case FADE_IN:
		if (m_fade.Update(m_deltaTime))
		{
			m_gameState = IN_GAME;
		}
		break;

	case FADE_OUT:
		if (m_fade.Update(m_deltaTime))
		{
			m_nowSceneData.Set(m_gameData.m_nextSceneNumber, false, nullptr);
		}
		return;
	}

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
		Exit();
		Initialize();
	}

	switch (d_viewPointCount)
	{
	case VIEW_GAME: break;
	case VIEW_FIRST:
		if (m_pLocalPlayer)
			m_pLocalPlayer->SetFirstPersonCamera(m_pEngine, m_camera);
		break;
	case VIEW_THIRD:
		if (m_pLocalPlayer)
			m_pLocalPlayer->SetThirdPersonFromBehind(m_pEngine, m_camera, m_map);
		break;
	}
#endif
}

void SceneGame::Draw()
{
	vector<SpotLight>* lights = nullptr;

	m_map.DrawMap(m_pEngine, &m_camera, &m_projection, &m_ambient, &m_light, lights);
	m_map.DrawGoalEffect(&m_camera, &m_projection);

	if (m_pLocalPlayer)
	{
		m_pLocalPlayer->Draw(&m_camera, &m_projection, &m_ambient, &m_light);
	}

	for (auto& kv : m_remotePlayers)
	{
		kv.second->Draw(&m_camera, &m_projection, &m_ambient, &m_light);
	}

	if (d_debugFlag & DRAW_BOXLINE)
	{
		m_map.DebugBoxLine(m_pEngine, &m_camera, &m_projection);
	}

	if (m_pLocalPlayer)
	{
		m_map.DrawMiniMap(m_pEngine, m_pLocalPlayer->GetPosition2D(), m_pLocalPlayer->GetArrowAngle());
	}

	m_pEngine->SpriteBegin();

	if (m_pLocalPlayer)
	{
		m_pLocalPlayer->DrawStaminaGauge(m_pEngine);
	}

#if _DEBUG
	if (!(d_debugFlag & DISPLAY_DEBUG_STRING))
	{
		m_pEngine->DrawPrintf(50, 950, FONT_GOTHIC40, Color::WHITE, "DEL : %f", (float)m_deltaTime);
		m_pEngine->DrawPrintf(50, 1000, FONT_GOTHIC40, Color::WHITE, "FPS : %f", (float)m_pEngine->GetFPS());
		m_pEngine->DrawPrintf(50, 900, FONT_GOTHIC40, Color::CYAN, "Players: %d", (int)m_remotePlayers.size() + 1);

		if (d_debugFlag & DRAW_PLAYER_STATE && m_pLocalPlayer)
		{
			m_pLocalPlayer->DebugPrint(m_pEngine);
		}

		m_pEngine->DrawPrintf(1300, 30, FONT_GOTHIC40, Color::BLUE, "ゲームステータス: In Game");
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

		if (d_debugFlag & STOP_GAME)
			m_pEngine->DrawPrintf(1300, 400, FONT_GOTHIC40, Color::BLUE, "F6: 一時停止解除");
		else
			m_pEngine->DrawPrintf(1300, 400, FONT_GOTHIC40, Color::BLUE, "F6: 一時停止");

		m_pEngine->DrawPrintf(1300, 450, FONT_GOTHIC40, Color::BLUE, "F7:ゲームモード変更");

		if (d_debugFlag & DEBUG_MODE)
			m_pEngine->DrawPrintf(1300, 500, FONT_GOTHIC40, Color::BLUE, "現在: デバッグモード");
		else
			m_pEngine->DrawPrintf(1300, 500, FONT_GOTHIC40, Color::BLUE, "現在 : ゲームモード");

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
	for (auto& kv : m_remotePlayers)
	{
		kv.second->Release(m_pEngine);
		delete kv.second;
	}
	m_remotePlayers.clear();

	if (m_pLocalPlayer)
	{
		m_pLocalPlayer->Release(m_pEngine);
		delete m_pLocalPlayer;
		m_pLocalPlayer = nullptr;
	}

	m_map.Release(m_pEngine);
	m_fade.Release(m_pEngine);
	m_pEngine->ReleaseFont(FONT_GOTHIC40);
	m_pEngine->ReleaseModel(MODEL_CHARACTER);
}

#ifdef USE_IMGUI
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

	Camera miniMapCamera = m_camera;
	m_map.Initialize(m_pEngine, &miniMapCamera, &m_projection, &m_ambient, &m_light, 1);

	m_pClient = ClientManager::GetInstance();
	m_pServer = ServerManager::GetInstance();

	m_bIsHost = m_pClient->IsHost();
	m_localClientId = m_bIsHost ? 1 : 2;

	NET_LOG_F("[SceneGame] Initialize: IsHost=%d, ClientID=%u", m_bIsHost, m_localClientId);

	m_pLocalPlayer = new Player();
	m_pLocalPlayer->Initialize(m_pEngine, m_map, &m_projection, m_camera, m_light);
	m_pLocalPlayer->SetClientId(m_localClientId);
	m_pLocalPlayer->SetPlayerName(m_pClient->GetPlayerName());

	m_lastTime = timeGetTime();
	m_lastNetworkUpdate = timeGetTime();

	m_pLocalPlayer->Update(m_pEngine, m_map, m_camera, m_light, 0);
	m_pLocalPlayer->SetFirstPersonCamera(m_pEngine, m_camera);

	SoundManager::Play(AK::EVENTS::PLAY_BGM_GAME, ID_BGM);
	m_pEngine->AddFont(FONT_GOTHIC40);
}

void SceneGame::UpdateNetwork()
{
	if (m_pClient) m_pClient->Update();
	if (m_bIsHost && m_pServer) m_pServer->Update();
}

void SceneGame::SyncPlayers()
{
	if (!m_pLocalPlayer) return;

	PlayerNetworkData localData = m_pLocalPlayer->GetNetworkData();

	std::vector<uint8_t> buffer;
	buffer.push_back((uint8_t)MSG_PLAYER_UPDATE);

	const uint8_t* dataPtr = reinterpret_cast<const uint8_t*>(&localData);
	buffer.insert(buffer.end(), dataPtr, dataPtr + sizeof(PlayerNetworkData));

	if (m_bIsHost)
	{
		NET_LOG_F("[SceneGame] Host broadcasting: ClientID=%u", localData.clientId);
	}
	else
	{
		NET_LOG_F("[SceneGame] Client sending to host: ClientID=%u", localData.clientId);
	}
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
	else
	{
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