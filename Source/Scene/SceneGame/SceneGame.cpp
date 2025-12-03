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

	// ★ ローカルプレイヤーは役割が決まるまで生成しない
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

		// ★★★ 鬼のライトを毎フレーム更新 ★★★
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

	// ★★★ 修正: 初期同期で役割がなくても仮生成する ★★★
	if (!m_bInitialSyncDone && now - m_lastTime > 500)
	{
		NET_LOG("[SceneGame] 初期同期開始");

		if (!m_bIsHost)
		{
			NetPlayerSpawn spawn;
			while (m_pClient->PopPlayerSpawn(spawn))
			{
				if (spawn.clientId != m_localClientId)
				{
					NET_LOG_F("[SceneGame] 既存プレイヤーを仮生成: ID=%u, Name=%s",
						spawn.clientId, spawn.name);

					// ★★★ 役割がなくてもRUNNERとして仮生成 ★★★
					PlayerRole role = ROLE_RUNNER;  // デフォルト
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
				}
			}
		}

		m_bInitialSyncDone = true;
		NET_LOG_F("[SceneGame] 初期同期完了 - プレイヤー数: %d", (int)m_players.size());
	}

	if (now - m_lastNetworkSend >= NETWORK_SEND_INTERVAL)
	{
		SyncToServer();
		m_lastNetworkSend = now;
	}

	if (m_bIsHost && m_pServer && now - m_lastWorldBroadcast >= WORLD_BROADCAST_INTERVAL)
	{
		m_pServer->BroadcastWorldState();
		m_lastWorldBroadcast = now;
	}

	ReceiveFromServer();
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
	if (m_bEnablePrediction)
	{
		for (auto& kv : m_players)
		{
			if (kv.second && !kv.second->IsLocal())
			{
				kv.second->PredictMovement(m_deltaTime);

				// ★★★ 追加: リモートの鬼のライトを更新 ★★★
				if (m_playerRoles[kv.first] == ROLE_CHASER)
				{
					Chaser* chaser = dynamic_cast<Chaser*>(kv.second);
					if (chaser)
					{
						// ネットワーク経由で受け取った位置・方向でライトを更新
						chaser->UpdateLight(m_pEngine);
					}
				}
			}
		}
	}
	else
	{
		// ★★★ 予測なしの場合もライトは更新 ★★★
		for (auto& kv : m_players)
		{
			if (kv.second && !kv.second->IsLocal())
			{
				if (m_playerRoles[kv.first] == ROLE_CHASER)
				{
					Chaser* chaser = dynamic_cast<Chaser*>(kv.second);
					if (chaser)
					{
						chaser->UpdateLight(m_pEngine);
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

	// 現在のレンダーターゲットと深度バッファを退避
	LPDIRECT3DSURFACE9 pOldBackBuffer = nullptr;
	LPDIRECT3DSURFACE9 pOldDepthBuffer = nullptr;
	pDevice->GetRenderTarget(0, &pOldBackBuffer);
	pDevice->GetDepthStencilSurface(&pOldDepthBuffer);

	// 各鬼のシャドウマップを生成
	for (auto& kv : m_players)
	{
		if (!kv.second || m_playerRoles[kv.first] != ROLE_CHASER)
			continue;

		Chaser* chaser = dynamic_cast<Chaser*>(kv.second);
		if (!chaser || !chaser->IsShadowMapEnabled())
			continue;

		// シャドウマップ用のレンダーターゲットに切り替え
		LPDIRECT3DTEXTURE9 pShadowTex = chaser->GetShadowTexture();
		LPDIRECT3DSURFACE9 pShadowSurface = nullptr;
		LPDIRECT3DSURFACE9 pShadowDepth = chaser->GetShadowDepthSurface();

		HRESULT hr = pShadowTex->GetSurfaceLevel(0, &pShadowSurface);
		if (FAILED(hr))
		{
			NET_LOG("[SceneGame] シャドウサーフェス取得失敗");
			continue;
		}

		// レンダーターゲットを設定
		pDevice->SetRenderTarget(0, pShadowSurface);
		pDevice->SetDepthStencilSurface(pShadowDepth);

		// ビューポートをシャドウマップサイズに設定
		D3DVIEWPORT9 shadowViewport;
		shadowViewport.X = 0;
		shadowViewport.Y = 0;
		shadowViewport.Width = 512;
		shadowViewport.Height = 512;
		shadowViewport.MinZ = 0.0f;
		shadowViewport.MaxZ = 1.0f;
		pDevice->SetViewport(&shadowViewport);

		// クリア（白で塗りつぶす = 影なし）
		pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFFFFFFFF, 1.0f, 0);

		// ライトのビュー・プロジェクション行列を取得
		D3DXMATRIX matLightView = chaser->GetLightViewMatrix();
		D3DXMATRIX matLightProj = chaser->GetLightProjectionMatrix();

		// ★★★ マップの深度レンダリング ★★★
		m_map.DrawMapDepth(m_pEngine);

		// ★★★ 他のプレイヤーも影を落とす ★★★
		for (auto& kv2 : m_players)
		{
			if (kv2.second && kv2.first != kv.first)
			{
				kv2.second->DrawDepth(m_pEngine);
			}
		}

		// サーフェスを解放
		if (pShadowSurface) pShadowSurface->Release();
	}

	// 元のレンダーターゲットに戻す
	pDevice->SetRenderTarget(0, pOldBackBuffer);
	pDevice->SetDepthStencilSurface(pOldDepthBuffer);

	// ビューポートを元に戻す
	D3DVIEWPORT9 mainViewport;
	mainViewport.X = 0;
	mainViewport.Y = 0;
	mainViewport.Width = WINDOW_WIDTH;
	mainViewport.Height = WINDOW_HEIGHT;
	mainViewport.MinZ = 0.0f;
	mainViewport.MaxZ = 1.0f;
	pDevice->SetViewport(&mainViewport);

	if (pOldBackBuffer) pOldBackBuffer->Release();
	if (pOldDepthBuffer) pOldDepthBuffer->Release();

	static DWORD lastLog = 0;
	DWORD now = timeGetTime();
	if (now - lastLog > 2000)
	{
		int shadowCount = 0;
		for (auto& kv : m_players)
		{
			if (kv.second && m_playerRoles[kv.first] == ROLE_CHASER)
			{
				Chaser* chaser = dynamic_cast<Chaser*>(kv.second);
				if (chaser && chaser->IsShadowMapEnabled()) shadowCount++;
			}
		}
		NET_LOG_F("[SceneGame] シャドウマップ生成完了: %d 個", shadowCount);
		lastLog = now;
	}
}

void SceneGame::UpdateChaserLights()
{
	m_chaserLights.clear();

	for (auto& kv : m_players)
	{
		if (kv.second && m_playerRoles[kv.first] == ROLE_CHASER)
		{
			Chaser* chaser = dynamic_cast<Chaser*>(kv.second);
			if (chaser)
			{
				chaser->UpdateLight(m_pEngine);

				SpotLight* light = chaser->GetLights();
				if (light)
				{
					m_chaserLights.push_back(light);
				}
			}
		}
	}

	static DWORD lastLog = 0;
	DWORD now = timeGetTime();
	if (now - lastLog > 3000)
	{
		NET_LOG_F("[SceneGame] 鬼のライト更新: %d 個", (int)m_chaserLights.size());
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
	m_players[clientId] = p;
	m_playerRoles[clientId] = role;

	NET_LOG_F("[SceneGame] プレイヤー生成完了: ID=%u, Role=%s, Pos=(%.1f,%.1f,%.1f)",
		clientId, (role == ROLE_CHASER) ? "鬼" : (role == ROLE_RUNNER) ? "逃げる側" : "未定",
		spawnPos.x, spawnPos.y, spawnPos.z);
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
	// ★★★ 1. シャドウマップを生成 ★★★
	RenderShadowMaps();

	std::vector<SpotLight> spotLights;
	std::vector<LPDIRECT3DTEXTURE9> shadowMaps;
	std::vector<D3DXMATRIX> lightViewProjs;

	// スポットライトとシャドウマップを収集
	for (auto& kv : m_players)
	{
		if (!kv.second || m_playerRoles[kv.first] != ROLE_CHASER)
			continue;

		Chaser* chaser = dynamic_cast<Chaser*>(kv.second);
		if (!chaser)
			continue;

		// ライト情報を追加
		SpotLight* light = chaser->GetLights();
		if (light)
		{
			spotLights.push_back(*light);

			// ★★★ 修正: シャドウマップが有効な場合のみ追加（ライト数と一致させる） ★★★
			if (chaser->IsShadowMapEnabled())
			{
				shadowMaps.push_back(chaser->GetShadowTexture());
				lightViewProjs.push_back(chaser->GetLightViewProjectionMatrix());
			}
			else
			{
				// シャドウマップが無効でもnullptrを追加してライト数と一致させる
				shadowMaps.push_back(nullptr);
				D3DXMATRIX identity;
				D3DXMatrixIdentity(&identity);
				lightViewProjs.push_back(identity);
			}
		}
	}

	// ★★★ 修正: nullptrのチェックを追加 ★★★
	std::vector<SpotLight>* pLights = spotLights.empty() ? nullptr : &spotLights;

	// シャドウマップ配列に有効なテクスチャが1つでもあるかチェック
	bool hasValidShadowMaps = false;
	for (auto tex : shadowMaps) {
		if (tex != nullptr) {
			hasValidShadowMaps = true;
			break;
		}
	}

	std::vector<LPDIRECT3DTEXTURE9>* pShadowMaps = (shadowMaps.empty() || !hasValidShadowMaps) ? nullptr : &shadowMaps;
	std::vector<D3DXMATRIX>* pLightViewProjs = (lightViewProjs.empty() || !hasValidShadowMaps) ? nullptr : &lightViewProjs;

	// マップ描画（影付き）
	m_map.DrawMap(m_pEngine, &m_camera, &m_projection, &m_ambient, &m_light,
		pLights, pShadowMaps, pLightViewProjs);

	// ゴールエフェクト描画
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

	if (m_pLocalPlayer)
	{
		if (m_localRole == ROLE_RUNNER)
		{
			Runner* runner = dynamic_cast<Runner*>(m_pLocalPlayer);
			if (runner) runner->DrawStaminaGauge(m_pEngine);
		}
	}

#if _DEBUG
	if (!(d_debugFlag & DISPLAY_DEBUG_STRING))
	{
		m_pEngine->DrawPrintf(50, 950, FONT_GOTHIC40, Color::WHITE, "DEL : %f", m_deltaTime);
		m_pEngine->DrawPrintf(50, 1000, FONT_GOTHIC40, Color::WHITE, "FPS : %f", (float)m_pEngine->GetFPS());
		m_pEngine->DrawPrintf(50, 900, FONT_GOTHIC40, Color::CYAN, "Players: %d (Drawn: %d) Lights: %d Shadows: %d",
			(int)m_players.size(), drawnCount, (int)spotLights.size(), hasValidShadowMaps ? (int)shadowMaps.size() : 0);

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

		// 全プレイヤー情報を表示
		int yOffset = 500;
		for (auto& kv : m_players)
		{
			D3DCOLOR color = (kv.first == m_localClientId) ? Color::YELLOW : Color::GREEN;

			if (m_playerRoles[kv.first] == ROLE_CHASER)
			{
				color = Color::RED;
			}

			const char* prefix = (kv.first == m_localClientId) ? "Local" : "Remote";
			const char* roleStr = (m_playerRoles[kv.first] == ROLE_CHASER) ? "鬼" : "逃げる側";
			D3DXVECTOR3 pos = kv.second->GetPosition();

			m_pEngine->DrawPrintf(0, yOffset, FONT_GOTHIC40, color,
				"%s[%u]: %s [%s] Pos=(%.1f,%.1f,%.1f)",
				prefix, kv.first, kv.second->GetCharacterName().c_str(), roleStr,
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