#define _USING_V110_SDK71_ 1

#include "SceneGame.h"
#include "..\\..\\Object\\Network\\NetworkLogger.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;
using namespace std;

SceneGame::SceneGame(Engine* pEngine)
	: Scene(pEngine)
	, m_pLocalPlayer(nullptr)
	, m_localClientId(0)
	, m_bIsHost(false)
	, m_lastNetworkSend(0)
	, m_lastWorldBroadcast(0)
	, m_pClient(nullptr)
	, m_pServer(nullptr)
	, d_debugFlag(0)
	, d_fpsCount(60)
	, d_viewPointCount(0)
	, m_gameState(FADE_IN)
	, m_deltaTime(0)
	, f_miniMapSourHalfSize(0)
	, m_lastTime(0)
	, m_bInitialSyncDone(false)
	, m_bFirstPerson(true)
	, m_bEnablePrediction(true)          
	, m_bEnableJitterReduction(true)
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
	D3DXVec3Normalize(&direction, &direction);
	m_light.SetDirection(direction);

	while (ShowCursor(FALSE) >= 0);
	Initialize();
}

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

	if (m_bIsHost)
	{
		m_localClientId = 1;
	}
	else
	{
		m_localClientId = m_pClient->GetAssignedClientId();
		if (m_localClientId == 0) m_localClientId = 2;
	}

	NET_LOG_F("[SceneGame] Initialize: IsHost=%d, ClientID=%u", m_bIsHost, m_localClientId);

	// ローカルプレイヤーを生成
	D3DXVECTOR3 startPos = m_map.GetPlayerStartPosition();
	m_pLocalPlayer = new Player();
	m_pLocalPlayer->SetIsLocal(true);
	m_pLocalPlayer->SetClientId(m_localClientId);
	m_pLocalPlayer->SetCharacterName(m_pClient->GetPlayerName());  // ★ 修正
	m_pLocalPlayer->Initialize(m_pEngine, m_map, &m_projection, m_camera, m_light);

	m_players[m_localClientId] = m_pLocalPlayer;

	//ホストの場合、自分のスポーン情報をブロードキャスト
	if (m_bIsHost && m_pServer)
	{
		NetPlayerSpawn spawn;
		spawn.clientId = m_localClientId;
		spawn.startX = startPos.x;
		spawn.startY = startPos.y;
		spawn.startZ = startPos.z;
		// ★ strncpy_s の正しい使い方
		strncpy_s(spawn.name, sizeof(spawn.name), m_pLocalPlayer->GetCharacterName().c_str(), _TRUNCATE);
		spawn.name[sizeof(spawn.name) - 1] = '\0';

		m_pServer->BroadcastPlayerSpawn(spawn);
		NET_LOG_F("[SceneGame] ホストのスポーン情報をブロードキャスト: ID=%u", m_localClientId);
	}

	m_lastTime = timeGetTime();
	m_lastNetworkSend = m_lastTime;
	m_lastWorldBroadcast = m_lastTime;
	m_bInitialSyncDone = false;
	m_bFirstPerson = true;
	m_pLocalPlayer->Update(m_pEngine, m_map, m_camera, m_light, 0);
	m_pLocalPlayer->SetFirstPersonCamera(m_pEngine, m_camera);

	SoundManager::Play(AK::EVENTS::PLAY_BGM_GAME, ID_BGM);

	NET_LOG("[SceneGame] 初期化完了 - 他プレイヤーのスポーン待機中");
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
		UpdateLocalPlayer();
		UpdateRemotePlayers();

		if (m_pLocalPlayer && m_map.CheckGoal(m_pLocalPlayer->GetPosition()) && !(d_debugFlag & DEBUG_MODE))
		{
			m_gameState = FADE_OUT;
			m_fade.SetFadeOut();
			m_gameData.m_nextSceneNumber = SCENE_CLEAR;
		}
		break;

	case CHANGE_SCENE:
		m_lastTime = timeGetTime();
		while (ShowCursor(FALSE) >= 0);

		switch (m_gameData.m_nextSceneNumber)
		{
		case Common::RESTART:
			SoundManager::StopAll(ID_BGM);
			Exit();
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
		// フェードイン中も同期を開始
		UpdateNetwork();

		if (m_fade.Update(m_deltaTime))
		{
			m_gameState = IN_GAME;
			NET_LOG("[SceneGame] フェードイン完了 - ゲーム開始");
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
}

void SceneGame::UpdateNetwork()
{
	if (m_pClient) m_pClient->Update();
	if (m_bIsHost && m_pServer) m_pServer->Update();

	if (!m_bIsHost && m_pClient)
	{
		static bool wasConnected = true;
		bool isConnected = m_pClient->IsConnected();

		if (!isConnected && wasConnected)
		{
			NET_LOG("[SceneGame] サーバーとの接続が失われました - タイトルに戻ります");
			m_pClient->Disconnect();
			m_nowSceneData.Set(Common::SCENE_TITLE, false, nullptr);
			return;
		}
		wasConnected = isConnected;
	}

	DWORD now = timeGetTime();

	if (!m_bInitialSyncDone && now - m_lastTime > 500)
	{
		NET_LOG("[SceneGame] 初期同期開始");

		// クライアントの場合、サーバーから既存プレイヤー情報を受信
		if (!m_bIsHost)
		{
			// スポーン情報を処理
			NetPlayerSpawn spawn;
			while (m_pClient->PopPlayerSpawn(spawn))
			{
				if (spawn.clientId != m_localClientId)
				{
					NET_LOG_F("[SceneGame] 既存プレイヤーを生成: ID=%u, Name=%s",
						spawn.clientId, spawn.name);
					SpawnPlayer(spawn.clientId, spawn.name,
						D3DXVECTOR3(spawn.startX, spawn.startY, spawn.startZ));
				}
			}
		}

		m_bInitialSyncDone = true;
		NET_LOG_F("[SceneGame] 初期同期完了 - プレイヤー数: %d", (int)m_players.size());
	}

	// 定期的にプレイヤー状態を送信
	if (now - m_lastNetworkSend >= NETWORK_SEND_INTERVAL)
	{
		SyncToServer();
		m_lastNetworkSend = now;
	}

	// ホストの場合、ワールド状態をブロードキャスト
	if (m_bIsHost && m_pServer && now - m_lastWorldBroadcast >= WORLD_BROADCAST_INTERVAL)
	{
		m_pServer->BroadcastWorldState();
		m_lastWorldBroadcast = now;
	}

	// サーバーからの更新を受信
	ReceiveFromServer();
}

void SceneGame::UpdateLocalPlayer()
{
	if (!m_pLocalPlayer) return;

	m_pLocalPlayer->Update(m_pEngine, m_map, m_camera, m_light, m_deltaTime);

#if _DEBUG
	switch (d_viewPointCount)
	{
	case VIEW_GAME:
		m_bFirstPerson = true;
		break;
	case VIEW_FIRST:
		m_pLocalPlayer->SetFirstPersonCamera(m_pEngine, m_camera);
		m_bFirstPerson = true;
		break;
	case VIEW_THIRD:
		m_pLocalPlayer->SetThirdPersonFromBehind(m_pEngine, m_camera, m_map);
		m_bFirstPerson = false;
		break;
	}
#else
	// リリースビルドではデフォルトで一人称視点
	m_bFirstPerson = true;
#endif
}

void SceneGame::UpdateRemotePlayers()
{
	if (m_bEnablePrediction)
	{
		for (auto& kv : m_players)
		{
			if (kv.second && !kv.second->IsLocal())
			{
				kv.second->PredictMovement(m_deltaTime);
			}
		}
	}
}
void SceneGame::SyncToServer()
{
	if (!m_pLocalPlayer || !m_pClient) return;

	NetPlayerState state = m_pLocalPlayer->GetNetState();

	// ★★★ デバッグログの頻度を下げる（3秒に1回）★★★
	static DWORD lastLogTime = 0;
	DWORD now = timeGetTime();
	if (now - lastLogTime > 3000)
	{
		NET_LOG_F("[SceneGame] 送信: ID=%u Pos=(%.1f,%.1f,%.1f) Interval=%dms",
			state.clientId, state.posX, state.posY, state.posZ, NETWORK_SEND_INTERVAL);
		lastLogTime = now;
	}

	if (m_bIsHost && m_pServer)
	{
		// ホストの場合、サーバーに直接状態を設定
		m_pServer->SetHostState(state);
	}
	else
	{
		// クライアントの場合、サーバーに送信
		m_pClient->SendPlayerState(state);
	}
}

void SceneGame::ReceiveFromServer()
{
	if (!m_pClient) return;

	// プレイヤーのスポーン処理
	NetPlayerSpawn spawn;
	while (m_pClient->PopPlayerSpawn(spawn))
	{
		if (spawn.clientId != m_localClientId)
		{
			NET_LOG_F("[SceneGame] 新規プレイヤー参加: ID=%u, Name=%s",
				spawn.clientId, spawn.name);
			SpawnPlayer(spawn.clientId, spawn.name,
				D3DXVECTOR3(spawn.startX, spawn.startY, spawn.startZ));
		}
	}

	// プレイヤーの削除処理
	uint32_t despawnId;
	while (m_pClient->PopPlayerDespawn(despawnId))
	{
		if (despawnId != m_localClientId)
		{
			NET_LOG_F("[SceneGame] プレイヤー退出: ID=%u", despawnId);
			DespawnPlayer(despawnId);
		}
	}

	// ★★★ ワールド状態の同期 ★★★
	NetWorldState world;
	if (m_pClient->GetWorldState(world))
	{
		// ★★★ デバッグログの頻度を下げる（3秒に1回）★★★
		static DWORD lastLogTime = 0;
		DWORD now = timeGetTime();
		if (now - lastLogTime > 3000)
		{
			NET_LOG_F("[SceneGame] ワールド状態受信: プレイヤー数=%d Interval=%dms",
				(int)world.playerCount, WORLD_BROADCAST_INTERVAL);
			lastLogTime = now;
		}

		for (int i = 0; i < world.playerCount; ++i)
		{
			const NetPlayerState& ps = world.players[i];

			// 自分自身はスキップ
			if (ps.clientId == m_localClientId) continue;

			auto it = m_players.find(ps.clientId);
			if (it != m_players.end() && it->second)
			{
				// ★★★ 既存プレイヤーの状態を更新 ★★★
				it->second->UpdateFromNetwork(ps, m_light, m_deltaTime);
			}
			else
			{
				// ★★★ まだ生成されていないプレイヤーを生成 ★★★
				SpawnPlayer(ps.clientId, "Player", D3DXVECTOR3(ps.posX, ps.posY, ps.posZ));

				// 生成直後に状態を更新
				if (m_players.find(ps.clientId) != m_players.end())
				{
					m_players[ps.clientId]->UpdateFromNetwork(ps, m_light, m_deltaTime);
				}
			}
		}
	}
}

void SceneGame::SpawnPlayer(uint32_t clientId, const std::string& name, const D3DXVECTOR3& pos)
{
	if (m_players.find(clientId) != m_players.end())
	{
		NET_LOG_F("[SceneGame] プレイヤー %u は既に存在 - スキップ", clientId);
		return;
	}

	Player* p = new Player();
	p->SetIsLocal(false);
	p->SetClientId(clientId);
	p->SetCharacterName(name);  // ★ SetPlayerName → SetCharacterName

								// 位置が(0,0,0)の場合はマップのスタート位置を使用
	D3DXVECTOR3 spawnPos = pos;
	if (pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f)
	{
		spawnPos = m_map.GetPlayerStartPosition();
		NET_LOG_F("[SceneGame] デフォルト位置を使用: (%.1f, %.1f, %.1f)",
			spawnPos.x, spawnPos.y, spawnPos.z);
	}

	p->InitializeAtPosition(m_pEngine, spawnPos, &m_projection, m_camera, m_light);
	p->SetPosition(spawnPos);

	m_players[clientId] = p;

	NET_LOG_F("[SceneGame] プレイヤー生成完了: ID=%u, Name=%s, Pos=(%.1f, %.1f, %.1f)",
		clientId, name.c_str(), spawnPos.x, spawnPos.y, spawnPos.z);
}
void SceneGame::DespawnPlayer(uint32_t clientId)
{
	auto it = m_players.find(clientId);
	if (it == m_players.end())
	{
		NET_LOG_F("[SceneGame] プレイヤー %u は存在しない - 削除スキップ", clientId);
		return;
	}

	if (it->second)
	{
		it->second->Release(m_pEngine);
		delete it->second;
	}
	m_players.erase(it);

	NET_LOG_F("[SceneGame] プレイヤー削除完了: ID=%u", clientId);
}

void SceneGame::Draw()
{
	vector<SpotLight>* lights = nullptr;

	m_map.DrawMap(m_pEngine, &m_camera, &m_projection, &m_ambient, &m_light, lights);
	m_map.DrawGoalEffect(&m_camera, &m_projection);

	// すべてのプレイヤーを描画
	int drawnCount = 0;
	for (auto& kv : m_players)
	{
		if (kv.second)
		{
			kv.second->Draw(&m_camera, &m_projection, &m_ambient, &m_light);
			drawnCount++;
		}
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
		m_pEngine->DrawPrintf(50, 950, FONT_GOTHIC40, Color::WHITE, "DEL : %f", m_deltaTime);
		m_pEngine->DrawPrintf(50, 1000, FONT_GOTHIC40, Color::WHITE, "FPS : %f", (float)m_pEngine->GetFPS());
		m_pEngine->DrawPrintf(50, 900, FONT_GOTHIC40, Color::CYAN, "Players: %d (Drawn: %d)",
			(int)m_players.size(), drawnCount);

		if (d_debugFlag & DRAW_PLAYER_STATE && m_pLocalPlayer)
		{
			m_pLocalPlayer->DebugPrint(m_pEngine);
		}

		// すべてのプレイヤー情報を表示（位置情報付き）
		int yOffset = 500;
		for (auto& kv : m_players)
		{
			D3DCOLOR color = (kv.first == m_localClientId) ? Color::YELLOW : Color::GREEN;
			const char* prefix = (kv.first == m_localClientId) ? "Local" : "Remote";
			D3DXVECTOR3 pos = kv.second->GetPosition();

			m_pEngine->DrawPrintf(0, yOffset, FONT_GOTHIC40, color,
				"%s[%u]: %s Pos=(%.1f,%.1f,%.1f)",
				prefix, kv.first, kv.second->GetCharacterName().c_str(),  // ★ GetPlayerName → GetCharacterName
				pos.x, pos.y, pos.z);
			yOffset += 50;
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
	// すべてのプレイヤーを解放
	for (auto& kv : m_players)
	{
		if (kv.second)
		{
			kv.second->Release(m_pEngine);
			delete kv.second;
		}
	}
	m_players.clear();
	m_pLocalPlayer = nullptr;

	m_map.Release(m_pEngine);
	m_fade.Release(m_pEngine);
	m_pEngine->ReleaseFont(FONT_GOTHIC40);
	m_pEngine->ReleaseModel(MODEL_CHARACTER);
}

void SceneGame::UpdateDebugFlag()
{
	if (m_pEngine->GetKeyStateSync(DIK_F1)) d_debugFlag ^= DRAW_PLAYER_STATE;
	if (m_pEngine->GetKeyStateSync(DIK_F2)) d_debugFlag ^= DRAW_BOXLINE;
	if (m_pEngine->GetKeyStateSync(DIK_F3)) d_viewPointCount = (d_viewPointCount + 1) % VIEW_MAX;
	if (m_pEngine->GetKeyStateSync(DIK_F4)) d_debugFlag |= RELOAD_FILE;
	else d_debugFlag &= ~RELOAD_FILE;
	if (m_pEngine->GetKeyStateSync(DIK_F5)) d_debugFlag ^= PATROLLER_VIEW_LINE;
	if (m_pEngine->GetKeyStateSync(DIK_F6)) d_debugFlag ^= STOP_GAME;
	if (m_pEngine->GetKeyStateSync(DIK_F7)) d_debugFlag ^= DEBUG_MODE;
	if (m_pEngine->GetKeyStateSync(DIK_F11)) d_debugFlag ^= DISPLAY_DEBUG_STRING;
}

#ifdef USE_IMGUI
void SceneGame::ImGuiFrameProcess()
{
}
#endif