#define _USING_V110_SDK71_ 1

#include "SceneGame.h"
#include "..\\..\\Object\\Network\\NetworkLogger.h"
#include "..\\..\\Object\\Chaser\\Chaser.h"
#include "..\\..\\Object\\Runner\\Runner.h"

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
	, m_localRole(ROLE_NONE)
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
	SetBackColor(0x00008000);
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

	m_localRole = ROLE_NONE;

	m_lastTime = timeGetTime();
	m_lastNetworkSend = m_lastTime;
	m_lastWorldBroadcast = m_lastTime;
	m_bInitialSyncDone = false;
	m_bFirstPerson = true;

	SoundManager::Play(AK::EVENTS::PLAY_BGM_GAME, ID_BGM);

	NET_LOG("[SceneGame] 初期化完了 - 役割割り当て待機中");
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

		// ★★★ 修正: 鬼のライト更新を毎フレーム呼び出し ★★★
		UpdateChaserLights();

		if (m_pLocalPlayer && m_localRole == ROLE_RUNNER)
		{
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

	NetRoleAssignment roleAssign;
	bool roleReceived = false;

	while (m_pClient->PopRoleAssignment(roleAssign))
	{
		roleReceived = true;
		m_playerRoles[roleAssign.clientId] = roleAssign.role;

		NET_LOG_F("[SceneGame] 役割受信: ID=%u Role=%s",
			roleAssign.clientId,
			(roleAssign.role == ROLE_CHASER) ? "鬼" : "逃げる側");

		if (roleAssign.clientId == m_localClientId && !m_pLocalPlayer)
		{
			m_localRole = roleAssign.role;
			D3DXVECTOR3 startPos = m_map.GetPlayerStartPosition();
			std::string myName = m_pClient->GetPlayerName();

			NET_LOG_F("[SceneGame] ローカルプレイヤー即座に生成: Role=%s",
				(m_localRole == ROLE_CHASER) ? "鬼" : "逃げる側");

			SpawnPlayerWithRole(m_localClientId, myName, startPos, m_localRole);

			if (m_pLocalPlayer)
			{
				m_pLocalPlayer->Update(m_pEngine, m_map, m_camera, m_light, 0);
				m_pLocalPlayer->SetFirstPersonCamera(m_pEngine, m_camera);
			}

			m_bInitialSyncDone = true;
			NET_LOG("[SceneGame] 初期同期完了（役割受信）");
		}
	}

	// ★★★ 修正: 役割が更新されたら即座にライト更新 ★★★
	if (roleReceived)
	{
		UpdateChaserLights();
	}

	if (!m_bInitialSyncDone)
	{
		if (m_bIsHost)
		{
			m_bInitialSyncDone = true;
			NET_LOG("[SceneGame] 初期同期完了（ホスト）");
		}
		else if (now - m_lastTime > 2000)
		{
			NET_LOG("[SceneGame] 初期同期タイムアウト - 仮同期開始");
			m_bInitialSyncDone = true;
		}
	}

	if (m_bInitialSyncDone)
	{
		NetPlayerSpawn spawn;
		while (m_pClient->PopPlayerSpawn(spawn))
		{
			if (spawn.clientId != m_localClientId)
			{
				NET_LOG_F("[SceneGame] リモートプレイヤー参加: ID=%u, Name=%s",
					spawn.clientId, spawn.name);

				if (m_players.find(spawn.clientId) != m_players.end())
				{
					continue;
				}

				PlayerRole role = ROLE_RUNNER;
				auto roleIt = m_playerRoles.find(spawn.clientId);
				if (roleIt != m_playerRoles.end())
				{
					role = roleIt->second;
				}

				SpawnPlayerWithRole(spawn.clientId, spawn.name,
					D3DXVECTOR3(spawn.startX, spawn.startY, spawn.startZ),
					role);

				// ★★★ 修正: プレイヤー追加後すぐにライト更新 ★★★
				if (role == ROLE_CHASER)
				{
					UpdateChaserLights();
				}
			}
		}

		uint32_t despawnId;
		while (m_pClient->PopPlayerDespawn(despawnId))
		{
			if (despawnId != m_localClientId)
			{
				NET_LOG_F("[SceneGame] プレイヤー退出: ID=%u", despawnId);
				if (m_playerRoles[despawnId] == ROLE_CHASER)
				{
					DespawnPlayer(despawnId);
					UpdateChaserLights();
				}
				else
				{
					DespawnPlayer(despawnId);
				}
			}
		}
	}

	if (m_bInitialSyncDone && now - m_lastNetworkSend >= NETWORK_SEND_INTERVAL)
	{
		SyncToServer();
		m_lastNetworkSend = now;
	}

	if (m_bIsHost && m_pServer && now - m_lastWorldBroadcast >= WORLD_BROADCAST_INTERVAL)
	{
		m_pServer->BroadcastWorldState();
		m_lastWorldBroadcast = now;
	}

	if (m_bInitialSyncDone)
	{
		ReceiveWorldState();
	}
}

void SceneGame::UpdateLocalPlayer()
{
	if (!m_pLocalPlayer) return;

	if (m_localRole == ROLE_RUNNER)
	{
		Runner* runner = dynamic_cast<Runner*>(m_pLocalPlayer);
		if (runner)
		{
			runner->Update(m_pEngine, m_map, m_camera, m_light, m_deltaTime);
		}
	}
	else if (m_localRole == ROLE_CHASER)
	{
		Chaser* chaser = dynamic_cast<Chaser*>(m_pLocalPlayer);
		if (chaser)
		{
			chaser->Update(m_pEngine, m_map, m_camera, m_light, m_deltaTime);
			// ★★★ 修正: ローカルの鬼のライトを毎フレーム更新 ★★★
			chaser->UpdateLight(m_pEngine);
		}
	}

#if _DEBUG
	switch (d_viewPointCount)
	{
	case VIEW_GAME:
		m_bFirstPerson = true;
		break;
	case VIEW_FIRST:
		if (m_pLocalPlayer)
			m_pLocalPlayer->SetFirstPersonCamera(m_pEngine, m_camera);
		m_bFirstPerson = true;
		break;
	case VIEW_THIRD:
		if (m_pLocalPlayer)
			m_pLocalPlayer->SetThirdPersonFromBehind(m_pEngine, m_camera, m_map);
		m_bFirstPerson = false;
		break;
	}
#else
	m_bFirstPerson = true;
#endif
	}

void SceneGame::UpdateRemotePlayers()
{
	for (auto& kv : m_players)
	{
		if (!kv.second || kv.second->IsLocal())
			continue;

		// 予測移動
		if (m_bEnablePrediction)
		{
			kv.second->PredictMovement(m_deltaTime);
		}

		// ★★★ 重要修正: リモートの鬼のライトを毎フレーム強制的にデバイスに設定 ★★★
		if (m_playerRoles[kv.first] == ROLE_CHASER)
		{
			Chaser* chaser = dynamic_cast<Chaser*>(kv.second);
			if (chaser)
			{
				// ライトをエンジンのデバイスに設定
				SpotLight* light = chaser->GetLights();
				if (light)
				{
					// ★★★ 追加: ライトインデックスを動的に割り当て ★★★
					// ローカル鬼が0番、リモート鬼は1番以降
					int lightIndex = 0;
					if (m_localRole == ROLE_CHASER && m_pLocalPlayer)
					{
						// ローカルが鬼なら、リモートは1番から
						lightIndex = 1;
						for (auto& kv2 : m_players)
						{
							if (kv2.first < kv.first && m_playerRoles[kv2.first] == ROLE_CHASER)
							{
								lightIndex++;
							}
						}
					}

					// ★★★ 重要: デバイスにライトを設定 ★★★
					light->SetDevice(m_pEngine, lightIndex);

					// ライトの更新
					chaser->UpdateLight(m_pEngine);

					// デバッグログ
					static std::map<uint32_t, DWORD> lastLog;
					DWORD now = timeGetTime();
					if (now - lastLog[kv.first] > 3000)
					{
						const D3DLIGHT9& l = light->GetLight();
						NET_LOG_F("[SceneGame::UpdateRemotePlayers] リモート鬼 ID=%u LightIndex=%d Pos=(%.1f,%.1f,%.1f) Dir=(%.2f,%.2f,%.2f)",
							kv.first, lightIndex,
							l.Position.x, l.Position.y, l.Position.z,
							l.Direction.x, l.Direction.y, l.Direction.z);
						lastLog[kv.first] = now;
					}
				}
			}
		}
	}
}

void SceneGame::SyncToServer()
{
	if (!m_pLocalPlayer || !m_pClient) return;

	NetPlayerState state = m_pLocalPlayer->GetNetState();

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
		m_pServer->SetHostState(state);
	}
	else
	{
		m_pClient->SendPlayerState(state);
	}
}

void SceneGame::ReceiveWorldState()
{
	if (!m_pClient) return;

	NetWorldState world;
	if (m_pClient->GetWorldState(world))
	{
		static DWORD lastLogTime = 0;
		DWORD now = timeGetTime();
		if (now - lastLogTime > 3000)
		{
			NET_LOG_F("[SceneGame] ワールド状態受信: プレイヤー数=%d", (int)world.playerCount);
			lastLogTime = now;
		}

		for (int i = 0; i < world.playerCount; ++i)
		{
			const NetPlayerState& ps = world.players[i];

			if (ps.clientId == m_localClientId) continue;

			auto it = m_players.find(ps.clientId);
			if (it != m_players.end() && it->second)
			{
				// 基本状態の更新
				it->second->UpdateFromNetwork(ps, m_light, m_deltaTime);

				// ★★★ 重要修正: 鬼のライト情報を即座に適用 ★★★
				if (m_playerRoles[ps.clientId] == ROLE_CHASER)
				{
					Chaser* chaser = dynamic_cast<Chaser*>(it->second);
					if (chaser)
					{
						// ライト情報をネットワークデータから設定
						D3DXVECTOR3 lightPos(ps.lightPosX, ps.lightPosY, ps.lightPosZ);
						D3DXVECTOR3 lightDir(ps.lightDirX, ps.lightDirY, ps.lightDirZ);

						SpotLight* light = chaser->GetLights();
						if (light)
						{
							light->SetPosition(lightPos);
							light->SetDirection(lightDir);
							light->SetRange(ps.lightRange);

							// ★★★ 追加: ライトをデバイスに即座に設定 ★★★
							int lightIndex = 0;
							if (m_localRole == ROLE_CHASER && m_pLocalPlayer)
							{
								lightIndex = 1;
								for (auto& kv2 : m_players)
								{
									if (kv2.first < ps.clientId && m_playerRoles[kv2.first] == ROLE_CHASER)
									{
										lightIndex++;
									}
								}
							}
							light->SetDevice(m_pEngine, lightIndex);
						}

						// ライトの更新
						chaser->UpdateLight(m_pEngine);

						// デバッグログ
						if (now - lastLogTime > 3000)
						{
							NET_LOG_F("[SceneGame::ReceiveWorldState] ID=%u にライト適用: Pos=(%.1f,%.1f,%.1f) Dir=(%.2f,%.2f,%.2f)",
								ps.clientId, lightPos.x, lightPos.y, lightPos.z, lightDir.x, lightDir.y, lightDir.z);
						}
					}
				}
			}
		}
	}
}

void SceneGame::ReceiveFromServer()
{
	if (!m_pClient) return;

	// ★ 役割割り当ての処理（最優先）
	NetRoleAssignment roleAssign;
	bool roleUpdated = false;

	while (m_pClient->PopRoleAssignment(roleAssign))
	{
		roleUpdated = true;
		m_playerRoles[roleAssign.clientId] = roleAssign.role;

		NET_LOG_F("[SceneGame] 役割受信: ID=%u Role=%s",
			roleAssign.clientId,
			(roleAssign.role == ROLE_CHASER) ? "鬼" : "逃げる側");

		// 既存プレイヤーの役割変更は行わない
		auto it = m_players.find(roleAssign.clientId);
		if (it != m_players.end() && it->second)
		{
			NET_LOG_F("[SceneGame] プレイヤー %u は既に生成済み", roleAssign.clientId);
			continue;
		}

		// 自分の役割が決定したらローカルプレイヤーを生成
		if (roleAssign.clientId == m_localClientId && !m_pLocalPlayer)
		{
			m_localRole = roleAssign.role;

			D3DXVECTOR3 startPos = m_map.GetPlayerStartPosition();
			std::string myName = m_pClient->GetPlayerName();

			NET_LOG_F("[SceneGame] ローカルプレイヤー生成: Role=%s",
				(m_localRole == ROLE_CHASER) ? "鬼" : "逃げる側");

			SpawnPlayerWithRole(m_localClientId, myName, startPos, m_localRole);

			// 初期カメラ設定
			if (m_pLocalPlayer)
			{
				m_pLocalPlayer->Update(m_pEngine, m_map, m_camera, m_light, 0);
				m_pLocalPlayer->SetFirstPersonCamera(m_pEngine, m_camera);
			}
		}
	}

	// 役割が更新されたらライトを再収集
	if (roleUpdated)
	{
		UpdateChaserLights();
	}

	// プレイヤーのスポーン処理
	NetPlayerSpawn spawn;
	while (m_pClient->PopPlayerSpawn(spawn))
	{
		if (spawn.clientId != m_localClientId)
		{
			NET_LOG_F("[SceneGame] 新規プレイヤー参加: ID=%u, Name=%s",
				spawn.clientId, spawn.name);

			if (m_players.find(spawn.clientId) != m_players.end())
			{
				NET_LOG_F("[SceneGame] プレイヤー %u は既に存在 - スキップ", spawn.clientId);
				continue;
			}

			PlayerRole role = ROLE_RUNNER;
			auto roleIt = m_playerRoles.find(spawn.clientId);
			if (roleIt != m_playerRoles.end())
			{
				role = roleIt->second;
				NET_LOG_F("[SceneGame] 役割情報あり: %s",
					(role == ROLE_CHASER) ? "鬼" : "逃げる側");
			}

			SpawnPlayerWithRole(spawn.clientId, spawn.name,
				D3DXVECTOR3(spawn.startX, spawn.startY, spawn.startZ),
				role);

			// 鬼が追加されたらライトを更新
			if (role == ROLE_CHASER)
			{
				UpdateChaserLights();
			}
		}
	}

	// プレイヤーの削除処理
	uint32_t despawnId;
	while (m_pClient->PopPlayerDespawn(despawnId))
	{
		if (despawnId != m_localClientId)
		{
			NET_LOG_F("[SceneGame] プレイヤー退出: ID=%u", despawnId);

			// 鬼が退出したらライトを更新
			if (m_playerRoles[despawnId] == ROLE_CHASER)
			{
				DespawnPlayer(despawnId);
				UpdateChaserLights();
			}
			else
			{
				DespawnPlayer(despawnId);
			}
		}
	}

	// ワールド状態の同期
	NetWorldState world;
	if (m_pClient->GetWorldState(world))
	{
		static DWORD lastLogTime = 0;
		DWORD now = timeGetTime();
		if (now - lastLogTime > 3000)
		{
			NET_LOG_F("[SceneGame] ワールド状態受信: プレイヤー数=%d", (int)world.playerCount);
			lastLogTime = now;
		}

		for (int i = 0; i < world.playerCount; ++i)
		{
			const NetPlayerState& ps = world.players[i];

			if (ps.clientId == m_localClientId) continue;

			auto it = m_players.find(ps.clientId);
			if (it != m_players.end() && it->second)
			{
				it->second->UpdateFromNetwork(ps, m_light, m_deltaTime);

				// ★★★ 追加: ネットワーク更新後もライトを更新 ★★★
				if (m_playerRoles[ps.clientId] == ROLE_CHASER)
				{
					Chaser* chaser = dynamic_cast<Chaser*>(it->second);
					if (chaser)
					{
						chaser->UpdateLight(m_pEngine);
					}
				}
			}
			else
			{
				NET_LOG_F("[SceneGame] 未生成プレイヤーを発見: ID=%u - 仮生成", ps.clientId);

				PlayerRole role = ROLE_RUNNER;
				auto roleIt = m_playerRoles.find(ps.clientId);
				if (roleIt != m_playerRoles.end())
				{
					role = roleIt->second;
				}

				SpawnPlayerWithRole(ps.clientId, "Player",
					D3DXVECTOR3(ps.posX, ps.posY, ps.posZ),
					role);

				if (role == ROLE_CHASER)
				{
					UpdateChaserLights();
				}

				if (m_players.find(ps.clientId) != m_players.end())
				{
					m_players[ps.clientId]->UpdateFromNetwork(ps, m_light, m_deltaTime);

					// ★★★ 新規生成直後もライトを更新 ★★★
					if (role == ROLE_CHASER)
					{
						Chaser* chaser = dynamic_cast<Chaser*>(m_players[ps.clientId]);
						if (chaser)
						{
							chaser->UpdateLight(m_pEngine);
						}
					}
				}
			}
		}
	}
}

void SceneGame::RenderShadowMaps()
{
	LPDIRECT3DDEVICE9 pDevice = m_pEngine->GetDevice();

	// ★★★ デバッグログ追加 ★★★
	static DWORD lastDebugLog = 0;
	DWORD now = timeGetTime();
	if (now - lastDebugLog > 3000)
	{
		NET_LOG_F("[RenderShadowMaps] 開始: m_players数=%d ローカルRole=%s",
			(int)m_players.size(),
			m_localRole == ROLE_CHASER ? "鬼" : "逃げる側");
		for (auto& kv : m_players)
		{
			auto roleIt = m_playerRoles.find(kv.first);
			NET_LOG_F("  Player[%u]: 存在=%s 役割=%s",
				kv.first,
				kv.second ? "Yes" : "No",
				roleIt != m_playerRoles.end() ? (roleIt->second == ROLE_CHASER ? "鬼" : "逃げる側") : "未設定");
		}
		lastDebugLog = now;
	}

	// 現在のレンダーターゲットと深度バッファを退避
	LPDIRECT3DSURFACE9 pOldBackBuffer = nullptr;
	LPDIRECT3DSURFACE9 pOldDepthBuffer = nullptr;
	pDevice->GetRenderTarget(0, &pOldBackBuffer);
	pDevice->GetDepthStencilSurface(&pOldDepthBuffer);

	// 現在のビューポートも退避
	D3DVIEWPORT9 oldViewport;
	pDevice->GetViewport(&oldViewport);

	// レンダーステートを退避
	DWORD oldCullMode, oldZEnable, oldZWriteEnable, oldColorWriteEnable;
	pDevice->GetRenderState(D3DRS_CULLMODE, &oldCullMode);
	pDevice->GetRenderState(D3DRS_ZENABLE, &oldZEnable);
	pDevice->GetRenderState(D3DRS_ZWRITEENABLE, &oldZWriteEnable);
	pDevice->GetRenderState(D3DRS_COLORWRITEENABLE, &oldColorWriteEnable);

	// ★★★ 修正: 各鬼のシャドウマップを生成 ★★★
	int shadowMapCount = 0;
	for (auto& kv : m_players)
	{
		// ★★★ デバッグログ ★★★
		if (now - lastDebugLog > 3000)
		{
			NET_LOG_F("[RenderShadowMaps] チェック中: Player[%u] Exists=%s",
				kv.first, kv.second ? "Yes" : "No");
		}

		if (!kv.second)
		{
			if (now - lastDebugLog > 3000)
			{
				NET_LOG_F("[RenderShadowMaps] スキップ: Player[%u] - プレイヤーが存在しない", kv.first);
			}
			continue;
		}

		auto roleIt = m_playerRoles.find(kv.first);
		if (roleIt == m_playerRoles.end())
		{
			if (now - lastDebugLog > 3000)
			{
				NET_LOG_F("[RenderShadowMaps] スキップ: Player[%u] - 役割が未設定", kv.first);
			}
			continue;
		}

		if (roleIt->second != ROLE_CHASER)
		{
			if (now - lastDebugLog > 3000)
			{
				NET_LOG_F("[RenderShadowMaps] スキップ: Player[%u] - 役割が鬼ではない", kv.first);
			}
			continue;
		}

		Chaser* chaser = dynamic_cast<Chaser*>(kv.second);
		if (!chaser)
		{
			if (now - lastDebugLog > 3000)
			{
				NET_LOG_F("[RenderShadowMaps] スキップ: Player[%u] - Chaserへのキャスト失敗", kv.first);
			}
			continue;
		}

		if (!chaser->IsShadowMapEnabled())
		{
			if (now - lastDebugLog > 3000)
			{
				NET_LOG_F("[RenderShadowMaps] スキップ: Player[%u] - シャドウマップ無効", kv.first);
			}
			continue;
		}

		// ★★★ ここまで来たらシャドウマップを生成 ★★★
		if (now - lastDebugLog > 3000)
		{
			NET_LOG_F("[RenderShadowMaps] シャドウマップ生成開始: Player[%u]", kv.first);
		}

		LPDIRECT3DTEXTURE9 pShadowTex = chaser->GetShadowTexture();
		LPDIRECT3DSURFACE9 pShadowSurface = nullptr;
		LPDIRECT3DSURFACE9 pShadowDepth = chaser->GetShadowDepthSurface();

		HRESULT hr = pShadowTex->GetSurfaceLevel(0, &pShadowSurface);
		if (FAILED(hr))
		{
			NET_LOG_F("[RenderShadowMaps] エラー: Player[%u] - シャドウサーフェス取得失敗", kv.first);
			continue;
		}

		// レンダーターゲットを設定
		pDevice->SetRenderTarget(0, pShadowSurface);
		pDevice->SetDepthStencilSurface(pShadowDepth);

		// ビューポートをシャドウマップサイズに設定
		D3DVIEWPORT9 shadowViewport;
		shadowViewport.X = 0;
		shadowViewport.Y = 0;
		shadowViewport.Width = 1024;
		shadowViewport.Height = 1024;
		shadowViewport.MinZ = 0.0f;
		shadowViewport.MaxZ = 1.0f;
		pDevice->SetViewport(&shadowViewport);

		// シャドウマップ用のレンダーステート設定
		pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
		pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
		pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0000000F);

		// クリア（深度バッファは1.0、カラーは白）
		pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFFFFFFFF, 1.0f, 0);

		// ライト行列取得
		D3DXMATRIX matLightVP = chaser->GetLightViewProjectionMatrix();

		// ★★★ 重要: マップの深度レンダリング ★★★
		m_map.DrawMapDepth(m_pEngine, &matLightVP);

		// ★★★ 修正: すべてのプレイヤーの影を描画（鬼自身以外）★★★
		for (auto& kv2 : m_players)
		{
			if (kv2.second && kv2.first != kv.first)
			{
				kv2.second->DrawDepth(m_pEngine, &matLightVP);
			}
		}

		// ★★★ 重要修正: ローカルプレイヤーの影も描画（鬼以外）★★★
		if (m_pLocalPlayer && m_localClientId != kv.first)
		{
			// ローカルプレイヤーが既にm_playersに含まれているか確認
			bool alreadyDrawn = false;
			for (auto& kv2 : m_players)
			{
				if (kv2.first == m_localClientId)
				{
					alreadyDrawn = true;
					break;
				}
			}

			// ★★★ 修正: m_playersに含まれていない場合のみ描画 ★★★
			if (!alreadyDrawn)
			{
				if (now - lastDebugLog > 3000)
				{
					NET_LOG_F("[RenderShadowMaps] ローカルプレイヤー描画: ID=%u Role=%s",
						m_localClientId,
						m_localRole == ROLE_CHASER ? "鬼" : "逃げる側");
				}
				m_pLocalPlayer->DrawDepth(m_pEngine, &matLightVP);
			}
		}

		if (pShadowSurface) pShadowSurface->Release();
		shadowMapCount++;

		if (now - lastDebugLog > 3000)
		{
			NET_LOG_F("[RenderShadowMaps] シャドウマップ生成完了: Player[%u] (合計: %d個)", kv.first, shadowMapCount);
		}
	}

	// ★★★ 追加: ローカルプレイヤーが鬼の場合もシャドウマップ生成 ★★★
	if (m_pLocalPlayer && m_localRole == ROLE_CHASER)
	{
		Chaser* localChaser = dynamic_cast<Chaser*>(m_pLocalPlayer);
		if (localChaser && localChaser->IsShadowMapEnabled())
		{
			// ローカル鬼が既にm_playersに含まれているか確認
			bool alreadyRendered = false;
			for (auto& kv : m_players)
			{
				if (kv.first == m_localClientId)
				{
					alreadyRendered = true;
					break;
				}
			}

			if (!alreadyRendered)
			{
				if (now - lastDebugLog > 3000)
				{
					NET_LOG_F("[RenderShadowMaps] ローカル鬼のシャドウマップ生成: ID=%u", m_localClientId);
				}

				LPDIRECT3DTEXTURE9 pShadowTex = localChaser->GetShadowTexture();
				LPDIRECT3DSURFACE9 pShadowSurface = nullptr;
				LPDIRECT3DSURFACE9 pShadowDepth = localChaser->GetShadowDepthSurface();

				HRESULT hr = pShadowTex->GetSurfaceLevel(0, &pShadowSurface);
				if (SUCCEEDED(hr))
				{
					pDevice->SetRenderTarget(0, pShadowSurface);
					pDevice->SetDepthStencilSurface(pShadowDepth);

					D3DVIEWPORT9 shadowViewport;
					shadowViewport.X = 0;
					shadowViewport.Y = 0;
					shadowViewport.Width = 1024;
					shadowViewport.Height = 1024;
					shadowViewport.MinZ = 0.0f;
					shadowViewport.MaxZ = 1.0f;
					pDevice->SetViewport(&shadowViewport);

					pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
					pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
					pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
					pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0000000F);

					pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFFFFFFFF, 1.0f, 0);

					D3DXMATRIX matLightVP = localChaser->GetLightViewProjectionMatrix();

					// マップの深度描画
					m_map.DrawMapDepth(m_pEngine, &matLightVP);

					// すべてのプレイヤーの影を描画（ローカル鬼以外）
					for (auto& kv : m_players)
					{
						if (kv.second && kv.first != m_localClientId)
						{
							kv.second->DrawDepth(m_pEngine, &matLightVP);
						}
					}

					if (pShadowSurface) pShadowSurface->Release();
					shadowMapCount++;
				}
			}
		}
	}

	if (now - lastDebugLog > 3000)
	{
		NET_LOG_F("[RenderShadowMaps] 完了: 生成数=%d", shadowMapCount);
	}

	// すべてのステートを元に戻す
	pDevice->SetRenderTarget(0, pOldBackBuffer);
	pDevice->SetDepthStencilSurface(pOldDepthBuffer);
	pDevice->SetViewport(&oldViewport);
	pDevice->SetRenderState(D3DRS_CULLMODE, oldCullMode);
	pDevice->SetRenderState(D3DRS_ZENABLE, oldZEnable);
	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, oldZWriteEnable);
	pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, oldColorWriteEnable);

	if (pOldBackBuffer) pOldBackBuffer->Release();
	if (pOldDepthBuffer) pOldDepthBuffer->Release();
}

// SceneGame.cpp の UpdateChaserLights() メソッド - 完全修正版
void SceneGame::UpdateChaserLights()
{
	m_chaserLights.clear();

	static DWORD lastLog = 0;
	DWORD now = timeGetTime();

	// ★★★ 修正1: ローカルプレイヤーが鬼の場合を追加 ★★★
	if (m_pLocalPlayer && m_localRole == ROLE_CHASER)
	{
		Chaser* localChaser = dynamic_cast<Chaser*>(m_pLocalPlayer);
		if (localChaser && localChaser->IsShadowMapEnabled())
		{
			SpotLight* light = localChaser->GetLights();
			if (light)
			{
				m_chaserLights.push_back(light);

				if (now - lastLog > 3000)
				{
					const D3DLIGHT9& l = light->GetLight();
					NET_LOG_F("[UpdateChaserLights] ローカル鬼追加: ID=%u LightIndex=%d Pos=(%.1f,%.1f,%.1f)",
						m_localClientId, (int)m_chaserLights.size() - 1,
						l.Position.x, l.Position.y, l.Position.z);
				}
			}
		}
	}

	// ★★★ 修正2: リモートプレイヤーの鬼を追加 ★★★
	for (auto& kv : m_players)
	{
		// ローカルプレイヤーはスキップ（既に追加済み）
		if (kv.first == m_localClientId)
			continue;

		// プレイヤーが存在しない場合はスキップ
		if (!kv.second)
			continue;

		// 役割が鬼でない場合はスキップ
		auto roleIt = m_playerRoles.find(kv.first);
		if (roleIt == m_playerRoles.end() || roleIt->second != ROLE_CHASER)
			continue;

		Chaser* chaser = dynamic_cast<Chaser*>(kv.second);
		if (chaser && chaser->IsShadowMapEnabled())
		{
			SpotLight* light = chaser->GetLights();
			if (light)
			{
				m_chaserLights.push_back(light);

				if (now - lastLog > 3000)
				{
					const D3DLIGHT9& l = light->GetLight();
					NET_LOG_F("[UpdateChaserLights] リモート鬼追加: ID=%u LightIndex=%d Pos=(%.1f,%.1f,%.1f) Dir=(%.2f,%.2f,%.2f)",
						kv.first, (int)m_chaserLights.size() - 1,
						l.Position.x, l.Position.y, l.Position.z,
						l.Direction.x, l.Direction.y, l.Direction.z);
				}
			}
		}
	}

	if (now - lastLog > 3000)
	{
		NET_LOG_F("[UpdateChaserLights] ライト収集完了: %d 個", (int)m_chaserLights.size());
		lastLog = now;
	}
}
void SceneGame::SpawnPlayerWithRole(uint32_t clientId, const std::string& name,
	const D3DXVECTOR3& pos, PlayerRole role)
{
	if (m_players.find(clientId) != m_players.end())
	{
		NET_LOG_F("[SceneGame] プレイヤー %u は既に存在 - スキップ", clientId);
		return;
	}

	CharacterBase* p = nullptr;

	// 役割に応じてRunnerまたはChaserを生成
	if (role == ROLE_RUNNER)
	{
		p = new Runner();
		NET_LOG_F("[SceneGame] Runnerを生成: ID=%u, Name=%s", clientId, name.c_str());
	}
	else if (role == ROLE_CHASER)
	{
		p = new Chaser();
		NET_LOG_F("[SceneGame] Chaserを生成: ID=%u, Name=%s", clientId, name.c_str());
	}
	else
	{
		// デフォルトでRunnerを生成
		p = new Runner();
		NET_LOG_F("[SceneGame] 役割未定だがRunnerとして仮生成: ID=%u, Name=%s", clientId, name.c_str());
	}

	bool isLocal = (clientId == m_localClientId);
	p->SetIsLocal(isLocal);
	p->SetClientId(clientId);
	p->SetCharacterName(name);

	D3DXVECTOR3 spawnPos = (pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f)
		? m_map.GetPlayerStartPosition() : pos;

	if (isLocal)
	{
		// ローカルプレイヤー
		if (role == ROLE_RUNNER)
		{
			((Runner*)p)->Initialize(m_pEngine, m_map, &m_projection, m_camera, m_light);
		}
		else
		{
			((Chaser*)p)->Initialize(m_pEngine, m_map, &m_projection, m_camera, m_light);
		}
		m_pLocalPlayer = p;
	}
	else
	{
		// リモートプレイヤー
		if (role == ROLE_RUNNER)
		{
			((Runner*)p)->InitializeAtPosition(m_pEngine, spawnPos, &m_projection,
				m_camera, m_light);
		}
		else
		{
			((Chaser*)p)->InitializeAtPosition(m_pEngine, spawnPos, &m_projection,
				m_camera, m_light);
		}
	}

	p->SetPosition(spawnPos);

	// ★★★ 重要: プレイヤーをマップに追加 ★★★
	m_players[clientId] = p;

	// ★★★ 重要: 役割を確実に設定 ★★★
	m_playerRoles[clientId] = role;

	NET_LOG_F("[SceneGame] プレイヤー生成完了: ID=%u, Role=%s, Pos=(%.1f,%.1f,%.1f)",
		clientId,
		(role == ROLE_CHASER) ? "鬼" : (role == ROLE_RUNNER) ? "逃げる側" : "未定",
		spawnPos.x, spawnPos.y, spawnPos.z);

	// ★★★ デバッグ: 役割マップの内容を確認 ★★★
	NET_LOG_F("[SceneGame] 現在の役割マップ: 総数=%d", (int)m_playerRoles.size());
	for (auto& kv : m_playerRoles)
	{
		NET_LOG_F("  ID=%u -> %s", kv.first, (kv.second == ROLE_CHASER) ? "鬼" : "逃げる側");
	}
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
		if (m_playerRoles[clientId] == ROLE_RUNNER)
		{
			((Runner*)it->second)->Release(m_pEngine);
		}
		else if (m_playerRoles[clientId] == ROLE_CHASER)
		{
			((Chaser*)it->second)->Release(m_pEngine);
		}
		delete it->second;
	}
	m_players.erase(it);
	m_playerRoles.erase(clientId);

	NET_LOG_F("[SceneGame] プレイヤー削除完了: ID=%u", clientId);
}

void SceneGame::Draw()
{
	// ★★★ 修正: シャドウマップ生成前にリモート鬼のライトを強制的に更新 ★★★
	for (auto& kv : m_players)
	{
		if (!kv.second || m_playerRoles[kv.first] != ROLE_CHASER)
			continue;

		Chaser* chaser = dynamic_cast<Chaser*>(kv.second);
		if (chaser)
		{
			// ★★★ 重要: ライトを強制的に更新 ★★★
			chaser->UpdateLight(m_pEngine);
		}
	}

	// ローカルプレイヤーが鬼の場合も更新
	if (m_pLocalPlayer && m_localRole == ROLE_CHASER)
	{
		Chaser* localChaser = dynamic_cast<Chaser*>(m_pLocalPlayer);
		if (localChaser)
		{
			localChaser->UpdateLight(m_pEngine);
		}
	}

	// シャドウマップ生成
	RenderShadowMaps();

	// ★★★ 修正: シャドウマップ生成直後にライト情報を強制的に更新 ★★★
	// すべての鬼のライトをデバイスに正しく設定
	int lightIndex = 0;

	// ローカルプレイヤーが鬼の場合
	if (m_pLocalPlayer && m_localRole == ROLE_CHASER)
	{
		Chaser* localChaser = dynamic_cast<Chaser*>(m_pLocalPlayer);
		if (localChaser && localChaser->IsShadowMapEnabled())
		{
			SpotLight* light = localChaser->GetLights();
			if (light)
			{
				light->SetDevice(m_pEngine, lightIndex);
				lightIndex++;
			}
		}
	}

	// リモートプレイヤーの鬼
	for (auto& kv : m_players)
	{
		if (kv.first == m_localClientId) continue;
		if (!kv.second || m_playerRoles[kv.first] != ROLE_CHASER) continue;

		Chaser* chaser = dynamic_cast<Chaser*>(kv.second);
		if (chaser && chaser->IsShadowMapEnabled())
		{
			SpotLight* light = chaser->GetLights();
			if (light)
			{
				// ★★★ 重要: デバイスに設定 ★★★
				light->SetDevice(m_pEngine, lightIndex);
				lightIndex++;
			}
		}
	}

	// ★★★ 最新のライト情報を収集 ★★★
	UpdateChaserLights();

	// ★★★ SpotLightの値のベクターを作成 ★★★
	std::vector<SpotLight> spotLights;
	std::vector<LPDIRECT3DTEXTURE9> shadowMaps;
	std::vector<D3DXMATRIX> lightViewProjs;
	std::vector<D3DXMATRIX> scaleBiases;

	// ★★★ 重要: 各鬼のライトとシャドウマップ情報を収集 ★★★
	for (size_t i = 0; i < m_chaserLights.size(); ++i)
	{
		if (!m_chaserLights[i])
			continue;

		// ★★★ デバッグ: ライトの値を確認 ★★★
		const D3DLIGHT9& lightCheck = m_chaserLights[i]->GetLight();
		if (lightCheck.Position.x == 0.0f && lightCheck.Position.y == 0.0f && lightCheck.Position.z == 0.0f)
		{
			NET_LOG_F("[SceneGame::Draw] 警告: Light[%d]の位置がゼロ！", (int)i);
		}

		// ライトの値をコピー
		spotLights.push_back(*m_chaserLights[i]);

		// 対応するChaserを探してシャドウマップ情報を取得
		Chaser* chaser = nullptr;

		// ローカルプレイヤーが鬼かチェック
		if (m_pLocalPlayer && m_localRole == ROLE_CHASER)
		{
			Chaser* localChaser = dynamic_cast<Chaser*>(m_pLocalPlayer);
			if (localChaser && localChaser->GetLights() == m_chaserLights[i])
			{
				chaser = localChaser;
			}
		}

		// リモートプレイヤーから探す
		if (!chaser)
		{
			for (auto& kv : m_players)
			{
				if (m_playerRoles[kv.first] != ROLE_CHASER)
					continue;

				Chaser* c = dynamic_cast<Chaser*>(kv.second);
				if (c && c->GetLights() == m_chaserLights[i])
				{
					chaser = c;
					break;
				}
			}
		}

		if (chaser && chaser->IsShadowMapEnabled())
		{
			shadowMaps.push_back(chaser->GetShadowTexture());
			lightViewProjs.push_back(chaser->GetLightViewProjectionMatrix());
			scaleBiases.push_back(chaser->GetScaleBiasMatrix());
		}
		else
		{
			shadowMaps.push_back(nullptr);
			D3DXMATRIX identity;
			D3DXMatrixIdentity(&identity);
			lightViewProjs.push_back(identity);
			scaleBiases.push_back(identity);
		}
	}

	// ★★★ デバッグログ ★★★
	static DWORD lastLog = 0;
	DWORD now = timeGetTime();
	if (now - lastLog > 3000)
	{
		NET_LOG_F("[SceneGame::Draw] 鬼のライト数: %d シャドウマップ数: %d",
			(int)spotLights.size(), (int)shadowMaps.size());

		// 各ライトの情報を出力
		for (size_t i = 0; i < spotLights.size(); ++i)
		{
			const D3DLIGHT9& l = spotLights[i].GetLight();
			NET_LOG_F("  Light[%d] Pos=(%.1f,%.1f,%.1f) Dir=(%.2f,%.2f,%.2f) ShadowMap=%s",
				(int)i,
				l.Position.x, l.Position.y, l.Position.z,
				l.Direction.x, l.Direction.y, l.Direction.z,
				shadowMaps[i] ? "有効" : "無効");
		}
		lastLog = now;
	}

	// ★★★ マップ描画（ポインタで渡す） ★★★
	std::vector<SpotLight>* pLights = spotLights.empty() ? nullptr : &spotLights;
	std::vector<LPDIRECT3DTEXTURE9>* pShadowMaps = shadowMaps.empty() ? nullptr : &shadowMaps;
	std::vector<D3DXMATRIX>* pLightViewProjs = lightViewProjs.empty() ? nullptr : &lightViewProjs;
	std::vector<D3DXMATRIX>* pScaleBiases = scaleBiases.empty() ? nullptr : &scaleBiases;

	m_pEngine->Clear(D3DCOLOR_XRGB(0, 0, 0));

	m_map.DrawMap(m_pEngine, &m_camera, &m_projection, &m_ambient, &m_light,
		pLights, pShadowMaps, pLightViewProjs, pScaleBiases);

	m_map.DrawGoalEffect(&m_camera, &m_projection);

	// プレイヤー描画
	int drawnCount = 0;
	for (auto& kv : m_players)
	{
		if (kv.second)
		{
			kv.second->Draw(&m_camera, &m_projection, &m_ambient, &m_light);
			drawnCount++;
		}
	}

	// ローカルプレイヤーも描画
	if (m_pLocalPlayer)
	{
		bool alreadyDrawn = false;
		for (auto& kv : m_players)
		{
			if (kv.first == m_localClientId)
			{
				alreadyDrawn = true;
				break;
			}
		}

		if (!alreadyDrawn)
		{
			m_pLocalPlayer->Draw(&m_camera, &m_projection, &m_ambient, &m_light);
			drawnCount++;
		}
	}

#if _DEBUG
	if (d_debugFlag & DRAW_BOXLINE)
	{
		m_map.DebugBoxLine(m_pEngine, &m_camera, &m_projection);
	}
#endif

	// ミニマップ描画
	if (m_pLocalPlayer)
	{
		m_map.DrawMiniMap(m_pEngine, m_pLocalPlayer->GetPosition2D(), m_pLocalPlayer->GetArrowAngle());
	}

	// UI描画
	m_pEngine->SpriteBegin();

	if (m_pLocalPlayer && m_localRole == ROLE_RUNNER)
	{
		Runner* runner = dynamic_cast<Runner*>(m_pLocalPlayer);
		if (runner)
		{
			runner->DrawStaminaGauge(m_pEngine);
		}
	}

#if _DEBUG
	if (!(d_debugFlag & DISPLAY_DEBUG_STRING))
	{
		m_pEngine->DrawPrintf(50, 950, FONT_GOTHIC40, Color::WHITE, "DEL : %f", m_deltaTime);
		m_pEngine->DrawPrintf(50, 1000, FONT_GOTHIC40, Color::WHITE, "FPS : %f", (float)m_pEngine->GetFPS());
		m_pEngine->DrawPrintf(50, 900, FONT_GOTHIC40, Color::CYAN, "Players: %d (Drawn: %d) Lights: %d",
			(int)m_players.size() + (m_pLocalPlayer ? 1 : 0), drawnCount, (int)spotLights.size());

		if (d_debugFlag & DRAW_PLAYER_STATE && m_pLocalPlayer)
		{
			if (m_localRole == ROLE_RUNNER)
			{
				Runner* runner = dynamic_cast<Runner*>(m_pLocalPlayer);
				if (runner) runner->DebugPrint(m_pEngine);
			}
			else if (m_localRole == ROLE_CHASER)
			{
				Chaser* chaser = dynamic_cast<Chaser*>(m_pLocalPlayer);
				if (chaser) chaser->DebugPrint(m_pEngine);
			}
		}

		int yOffset = 500;

		if (m_pLocalPlayer)
		{
			D3DCOLOR color = (m_localRole == ROLE_CHASER) ? Color::RED : Color::YELLOW;
			const char* roleStr = (m_localRole == ROLE_CHASER) ? "鬼" : "逃げる側";
			D3DXVECTOR3 pos = m_pLocalPlayer->GetPosition();

			m_pEngine->DrawPrintf(0, yOffset, FONT_GOTHIC40, color,
				"Local[%u]: %s [%s] Pos=(%.1f,%.1f,%.1f)",
				m_localClientId, m_pLocalPlayer->GetCharacterName().c_str(), roleStr,
				pos.x, pos.y, pos.z);
			yOffset += 50;
		}

		for (auto& kv : m_players)
		{
			if (kv.first == m_localClientId)
				continue;

			if (!kv.second)
				continue;

			D3DCOLOR color = (m_playerRoles[kv.first] == ROLE_CHASER) ? Color::RED : Color::GREEN;
			const char* roleStr = (m_playerRoles[kv.first] == ROLE_CHASER) ? "鬼" : "逃げる側";
			D3DXVECTOR3 pos = kv.second->GetPosition();

			m_pEngine->DrawPrintf(0, yOffset, FONT_GOTHIC40, color,
				"Remote[%u]: %s [%s] Pos=(%.1f,%.1f,%.1f)",
				kv.first, kv.second->GetCharacterName().c_str(), roleStr,
				pos.x, pos.y, pos.z);
			yOffset += 50;
		}
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
	for (auto& kv : m_players)
	{
		if (kv.second)
		{
			if (m_playerRoles[kv.first] == ROLE_RUNNER)
			{
				((Runner*)kv.second)->Release(m_pEngine);
			}
			else if (m_playerRoles[kv.first] == ROLE_CHASER)
			{
				((Chaser*)kv.second)->Release(m_pEngine);
			}
			delete kv.second;
		}
	}
	m_players.clear();
	m_playerRoles.clear();
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