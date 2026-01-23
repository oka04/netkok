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
	LoadGameParameter();

	SetBackColor(0x00008000);
	d_debugFlag = 0;
	d_viewPointCount = 0;
	d_fpsCount = 60;
	m_pEngine->AddModel(MODEL_CHARACTER);
	m_pEngine->AddTexture(TEXTURE_VICTORY);
	m_pEngine->AddTexture(TEXTURE_DEFEAT);
	m_pEngine->AddTexture(TEXTURE_FADE);

	m_pEngine->AddFont(FONT_GOTHIC60);
	m_pEngine->AddFont(FONT_GOTHIC40);

	m_gameTime = 0.0f;
	m_bGameEnded = false;
	m_winnerTeam = -1;
	m_resultDisplayStart = 0;
	m_resultImageAlpha = 0.0f;
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
		NET_LOG_F("[SceneGame::Initialize] ホストとして初期化: ClientID=%u", m_localClientId);
	}
	else
	{
		m_localClientId = m_pClient->GetAssignedClientId();

		NET_LOG_F("[SceneGame::Initialize] クライアントとして初期化: GetAssignedClientId()=%u", m_localClientId);

		if (m_localClientId == 0)
		{
			NET_LOG("[SceneGame::Initialize] 警告: ClientIDが0です。接続待機中の可能性があります。");
			m_localClientId = 2;
			NET_LOG_F("[SceneGame::Initialize] 一時ID設定: ClientID=%u", m_localClientId);
		}
	}

	NET_LOG_F("[SceneGame] Initialize完了: IsHost=%d, ClientID=%u", m_bIsHost, m_localClientId);

	m_localRole = ROLE_NONE;

	m_lastTime = timeGetTime();
	m_lastNetworkSend = m_lastTime;
	m_lastWorldBroadcast = m_lastTime;
	m_bInitialSyncDone = false;
	m_bFirstPerson = true;

	// ★★★ 修正: BGMをリスナーID (ID_LISTENER = 1) で再生 ★★★
	AkPlayingID bgmId = SoundManager::Play(AK::EVENTS::PLAY_BGM_GAME, SoundManager::ID_LISTENER);

	NET_LOG_F("[SceneGame] BGM再生: PlayingID=%u", bgmId);

	if (bgmId == AK_INVALID_PLAYING_ID)
	{
		NET_LOG("[SceneGame] ★★★エラー★★★ BGM再生に失敗！");
	}

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

		// ★★★ 修正: 処理順序を整理 ★★★
		// 1. ネットワーク受信（他プレイヤーの状態を取得）
		UpdateNetwork();

		// 2. ローカルプレイヤーの更新（入力→移動→カメラ）
		UpdateLocalPlayer();

		// 3. リモートプレイヤーの更新（補間・予測のみ、Update()は呼ばない）
		UpdateRemotePlayers();

		// 4. 鬼のライト更新
		UpdateChaserLights();

		// 5. ブレスの当たり判定
		CheckBreathHitPlayers();

		if (!m_bGameEnded)
		{
			UpdateGameTimer();

			if (m_bIsHost)
			{
				CheckGameEnd();
			}
		}

		if (m_pEngine->GetKeyStateSync(DIK_ESCAPE) || m_pEngine->GetKeyStateSync(DIK_P))
		{
			m_pEngine->ScreenShot(TEXTURE_PAUSE);
			m_gameState = CHANGE_SCENE;
			m_nowSceneData.Set(Common::SCENE_PAUSE, true, this);
			return;
		}
		break;
	case RESULT_DISPLAY:
		// ★★★ リザルト表示中はゲーム結果の受信のみ処理 ★★★
		if (m_pClient) m_pClient->Update();
		if (m_bIsHost && m_pServer) m_pServer->Update();

		// ★★★ リザルト画像のフェードイン処理 ★★★
		if (m_resultImageAlpha < 255.0f)
		{
			m_resultImageAlpha += f_resultFadeSpeed * m_deltaTime;
			if (m_resultImageAlpha > 255.0f) m_resultImageAlpha = 255.0f;
		}

		// ★★★ ゲーム結果の受信処理（クライアントの場合） ★★★
		NetGameResult result;
		if (m_pClient && m_pClient->PopGameResult(result))
		{
			NET_LOG_F("[SceneGame] ゲーム結果受信: winner=%d", (int)result.winnerTeam);
			if (!m_bGameEnded)
			{
				ProcessGameResult(result.winnerTeam);
			}
		}

		// ★★★ リザルト表示時間終了チェック ★★★
		if (timeGetTime() - m_resultDisplayStart > f_resultDisplayDuration)
		{
			NET_LOG("[SceneGame] リザルト表示終了 - フェードアウト開始");

			// ★★★ 修正: プレイヤー情報のクリアはフェードアウト完了後に延期 ★★★
			// （m_winnerTeam や m_localRole はここではリセットしない）

			// ★★★ フェードアウト開始 ★★★
			m_fade.SetFadeOut();
			m_gameState = FADE_OUT;
			m_gameData.m_nextSceneNumber = Common::SCENE_LOBBY;
			NET_LOG("[SceneGame] ロビーへのフェードアウト開始");
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
			m_pEngine->AddFont(FONT_GOTHIC60);
			m_pEngine->AddFont(FONT_GOTHIC40);
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
		// ★★★ フェード中もネットワーク更新を継続 ★★★
		if (m_pClient) m_pClient->Update();
		if (m_bIsHost && m_pServer) m_pServer->Update();

		// ★★★ リザルト画像のアルファ値を維持（フェード中も表示し続ける）★★★
		if (m_resultImageAlpha < 255.0f)
		{
			m_resultImageAlpha += f_resultFadeSpeed * m_deltaTime;
			if (m_resultImageAlpha > 255.0f) m_resultImageAlpha = 255.0f;
		}

		if (m_fade.Update(m_deltaTime))
		{
			NET_LOG("[SceneGame] フェードアウト完了 - クリーンアップ開始");

			// ★★★ 修正: フェードアウト完了後にプレイヤー情報をクリア ★★★
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
			NET_LOG("[SceneGame] 全プレイヤー情報をクリア");

			// ★★★ ゲーム終了フラグのリセット ★★★
			m_bGameEnded = false;
			m_gameTime = 0.0f;
			m_winnerTeam = -1;
			m_resultImageAlpha = 0.0f;
			m_localRole = ROLE_NONE;
			m_bInitialSyncDone = false;
			NET_LOG("[SceneGame] ゲーム関連フラグをリセット");

			// ★★★ サーバーの状態を「待機中」に戻す（ホストのみ）★★★
			if (m_bIsHost && m_pServer)
			{
				m_pServer->SetGameState(0);
				NET_LOG("[SceneGame] サーバーゲーム状態を待機中に設定");
			}

			NET_LOG("[SceneGame] シーン遷移");
			m_nowSceneData.Set(m_gameData.m_nextSceneNumber, false, nullptr);
		}
		return;
	}
}
void SceneGame::UpdateGameTimer()
{
	m_gameTime += m_deltaTime;

	static DWORD lastLog = 0;
	DWORD now = timeGetTime();
	if (now - lastLog > 10000)  // 10秒ごとにログ
	{
		float remaining = f_gameDuration - m_gameTime;
		NET_LOG_F("[SceneGame] 残り時間: %.1f秒", remaining);
		lastLog = now;
	}
}

void SceneGame::CheckGameEnd()
{
	if (m_bGameEnded) return;

	// ★★★ 条件1: 制限時間終了 → 逃げる側の勝利 ★★★
	if (m_gameTime >= f_gameDuration)
	{
		NET_LOG("[SceneGame] 制限時間終了 - 逃げる側の勝利");
		m_bGameEnded = true;
		m_winnerTeam = 0;  // 0 = 逃げる側
		BroadcastGameResult(m_winnerTeam);
		ProcessGameResult(m_winnerTeam);
		return;
	}

	// ★★★ 条件2: 全Runner凍結 → 鬼側の勝利 ★★★
	if (AreAllRunnersFrozen())
	{
		NET_LOG("[SceneGame] 全員凍結 - 鬼側の勝利");
		m_bGameEnded = true;
		m_winnerTeam = 1;  // 1 = 鬼側
		BroadcastGameResult(m_winnerTeam);
		ProcessGameResult(m_winnerTeam);
		return;
	}
}

void SceneGame::SyncToServer()
{
	if (!m_pLocalPlayer || !m_pClient) return;

	uint32_t localId = m_pLocalPlayer->GetClientId();
	if (localId == 0)
	{
		NET_LOG_F("[SceneGame::SyncToServer] エラー: ローカルプレイヤーのClientIDが0です！ m_localClientId=%u",
			m_localClientId);
		return;
	}

	NetPlayerState state = m_pLocalPlayer->GetNetState();

	if (state.clientId != localId)
	{
		NET_LOG_F("[SceneGame::SyncToServer] 警告: GetNetState()のIDが一致しません！ GetClientId()=%u, state.clientId=%u",
			localId, state.clientId);
	}

	static uint32_t lastMeltTarget = 0;
	static DWORD lastLogTime = 0;
	DWORD now = timeGetTime();

	if (state.meltTargetId != 0 || lastMeltTarget != 0)
	{
		if (state.meltTargetId != lastMeltTarget || now - lastLogTime > 1000)
		{
			NET_LOG_F("[SceneGame::SyncToServer] ID=%u meltTarget=%u を送信",
				state.clientId, state.meltTargetId);
			lastMeltTarget = state.meltTargetId;
			lastLogTime = now;
		}
	}

	static DWORD lastPositionLog = 0;
	if (now - lastPositionLog > 3000)
	{
		NET_LOG_F("[SceneGame] 送信: ID=%u Pos=(%.1f,%.1f,%.1f) Interval=%dms",
			state.clientId, state.posX, state.posY, state.posZ, f_networkSendInterval);
		lastPositionLog = now;
	}

	// ★★★ 修正: ホスト・クライアント共通の送信処理 ★★★
	if (m_bIsHost && m_pServer)
	{
		// ホストの場合：サーバーに直接状態を設定
		m_pServer->SetHostState(state);
		m_pServer->BroadcastWorldState();
	}
	else
	{
		// クライアントの場合：サーバーに送信
		m_pClient->SendPlayerState(state);
	}
}

bool SceneGame::AreAllRunnersFrozen()
{
	int totalRunners = 0;
	int frozenRunners = 0;

	// ローカルプレイヤー
	if (m_pLocalPlayer && m_localRole == ROLE_RUNNER)
	{
		totalRunners++;
		Runner* runner = dynamic_cast<Runner*>(m_pLocalPlayer);
		if (runner && runner->IsFrozen())
		{
			frozenRunners++;
		}
	}

	// リモートプレイヤー
	for (auto& kv : m_players)
	{
		if (m_playerRoles[kv.first] == ROLE_RUNNER && kv.second)
		{
			totalRunners++;
			Runner* runner = dynamic_cast<Runner*>(kv.second);
			if (runner && runner->IsFrozen())
			{
				frozenRunners++;
			}
		}
	}

	// Runnerが0人の場合はfalse（ゲーム成立しない）
	if (totalRunners == 0) return false;

	bool allFrozen = (frozenRunners == totalRunners);

	static DWORD lastLog = 0;
	DWORD now = timeGetTime();
	if (now - lastLog > 3000)
	{
		NET_LOG_F("[SceneGame] Runner状態: 凍結=%d / 全体=%d", frozenRunners, totalRunners);
		lastLog = now;
	}

	return allFrozen;
}

void SceneGame::BroadcastGameResult(int winnerTeam)
{
	if (!m_bIsHost || !m_pServer) return;

	m_pServer->BroadcastGameResult(winnerTeam);
}

void SceneGame::ProcessGameResult(int winnerTeam)
{
	m_winnerTeam = winnerTeam;
	m_gameState = RESULT_DISPLAY;
	m_resultDisplayStart = timeGetTime();
	m_resultImageAlpha = 0.0f;  // ★ フェードイン用にアルファ値を0から開始

	NET_LOG_F("[SceneGame] リザルト画面表示開始: 勝者=%s",
		(winnerTeam == 0) ? "逃げる側" : "鬼側");
}
void SceneGame::UpdateNetwork()
{
	if (m_pClient) m_pClient->Update();
	if (m_bIsHost && m_pServer) m_pServer->Update();

	// ★★★ ゲーム結果の受信処理 ★★★
	NetGameResult result;
	if (m_pClient && m_pClient->PopGameResult(result))
	{
		NET_LOG_F("[SceneGame] ゲーム結果受信: winner=%d", (int)result.winnerTeam);
		ProcessGameResult(result.winnerTeam);
		return;  // すぐにリザルト表示に移行
	}

	// 接続チェック（クライアントのみ）
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

	// 役割割り当て受信
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
			D3DXVECTOR3 startPos = m_map.GetRunnerStartPosition();
			std::string myName = m_pClient->GetPlayerName();

			NET_LOG_F("[SceneGame] ローカルプレイヤー即座に生成: Role=%s",
				(m_localRole == ROLE_CHASER) ? "鬼" : "逃げる側");

			SpawnPlayerWithRole(m_localClientId, myName, startPos, m_localRole);

			if (m_pLocalPlayer)
			{
				NET_LOG_F("[SceneGame] ローカルプレイヤー生成完了: m_clientId=%u, GetClientId()=%u",
					m_localClientId, m_pLocalPlayer->GetClientId());

				m_pLocalPlayer->Update(m_pEngine, m_map, m_camera, m_light, 0);
				m_pLocalPlayer->SetFirstPersonCamera(m_pEngine, m_camera);
			}
			else
			{
				NET_LOG_F("[SceneGame] エラー: ローカルプレイヤーの生成に失敗");
			}

			m_bInitialSyncDone = true;
			NET_LOG("[SceneGame] 初期同期完了（役割受信）");
		}
	}

	if (roleReceived)
	{
		UpdateChaserLights();
	}

	// 初期同期処理
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
		// プレイヤー生成通知
		NetPlayerSpawn spawn;
		while (m_pClient->PopPlayerSpawn(spawn))
		{
			NET_LOG_F("[SceneGame::UpdateNetwork] プレイヤー生成通知受信: ID=%u, Name=%s",
				spawn.clientId, spawn.name);

			// ★★★ 修正: ローカルプレイヤーも含めて処理 ★★★
			if (m_players.find(spawn.clientId) != m_players.end())
			{
				NET_LOG_F("[SceneGame] プレイヤー ID=%u は既に存在 - スキップ", spawn.clientId);
				continue;
			}

			// ★★★ 修正: ローカルプレイヤーの場合も既に生成済みならスキップ ★★★
			if (spawn.clientId == m_localClientId)
			{
				if (m_pLocalPlayer)
				{
					NET_LOG_F("[SceneGame] ローカルプレイヤー ID=%u は既に生成済み - スキップ",
						spawn.clientId);
					continue;
				}
				// まだ生成していない場合は処理を続行
			}

			// 役割を取得
			PlayerRole role = ROLE_RUNNER;
			auto roleIt = m_playerRoles.find(spawn.clientId);
			if (roleIt != m_playerRoles.end())
			{
				role = roleIt->second;
			}
			else
			{
				NET_LOG_F("[SceneGame] 警告: プレイヤー ID=%u の役割が未設定 - デフォルトでRunner",
					spawn.clientId);
			}

			// プレイヤー生成
			if (spawn.clientId == m_localClientId)
			{
				// ローカルプレイヤー（通常は役割受信時に生成済み）
				NET_LOG_F("[SceneGame] ローカルプレイヤー遅延生成: ID=%u Role=%s",
					spawn.clientId, (role == ROLE_CHASER) ? "鬼" : "逃げる側");
			}
			else
			{
				// リモートプレイヤー
				NET_LOG_F("[SceneGame] リモートプレイヤー生成: ID=%u, Name=%s Role=%s",
					spawn.clientId, spawn.name,
					(role == ROLE_CHASER) ? "鬼" : "逃げる側");
			}

			SpawnPlayerWithRole(spawn.clientId, spawn.name,
				D3DXVECTOR3(spawn.startX, spawn.startY, spawn.startZ),
				role);

			if (role == ROLE_CHASER)
			{
				UpdateChaserLights();
			}
		}


		// プレイヤー削除通知
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

	// ローカルプレイヤーの状態送信
	if (m_bInitialSyncDone && m_pLocalPlayer && m_pLocalPlayer->GetClientId() != 0)
	{
		if (now - m_lastNetworkSend >= f_networkSendInterval)
		{
			SyncToServer();
			m_lastNetworkSend = now;
		}
	}
	else if (m_pLocalPlayer && m_pLocalPlayer->GetClientId() == 0)
	{
		static DWORD lastWarning = 0;
		if (now - lastWarning > 1000)
		{
			NET_LOG_F("[SceneGame::UpdateNetwork] 警告: ローカルプレイヤーのClientIDが0です！");
			lastWarning = now;
		}
	}

	// ホストのワールド状態ブロードキャスト
	if (m_bIsHost && m_pServer && now - m_lastWorldBroadcast >= f_worldBroadcastInterval)
	{
		m_pServer->BroadcastWorldState();
		m_lastWorldBroadcast = now;
	}

	// ワールド状態受信
	if (m_bInitialSyncDone)
	{
		ReceiveWorldState();
	}
}


void SceneGame::UpdateLocalPlayer()
{
	if (!m_pLocalPlayer) return;

	// ★★★ 修正: m_deltaTimeを確実に渡す ★★★
	m_pLocalPlayer->Update(m_pEngine, m_map, m_camera, m_light, m_deltaTime);

	if (m_localRole == ROLE_RUNNER)
	{
		Runner* localRunner = dynamic_cast<Runner*>(m_pLocalPlayer);
		if (localRunner && !localRunner->IsFrozen())
		{
			std::vector<std::pair<uint32_t, CharacterBase*>> playerList;

			for (auto& kv : m_players)
			{
				if (kv.first != m_localClientId && kv.second)
				{
					playerList.push_back(kv);
				}
			}

			localRunner->UpdateMeltTarget(playerList);
		}
	}

	DWORD now = timeGetTime();
	if (now - m_lastNetworkSend >= f_networkSendInterval)
	{
		SyncToServer();
		m_lastNetworkSend = now;
	}
}
void SceneGame::UpdateRemotePlayers()
{
	static DWORD lastLog = 0;
	DWORD now = timeGetTime();
	bool shouldLog = (now - lastLog > 2000);

	static std::map<uint32_t, AkPlayingID> footSoundMap;
	static std::map<uint32_t, AkPlayingID> breathSoundMap;
	static std::map<uint32_t, AkPlayingID> remoteMeltSounds;
	static std::map<uint32_t, uint32_t> lastMeltTarget;
	static std::map<uint32_t, bool> wasFrozenMap;

	for (auto& kv : m_players)
	{
		if (!kv.second) continue;

		uint32_t remoteId = kv.first;
		CharacterBase* pRemote = kv.second;

		if (remoteId == m_localClientId) continue;
		if (pRemote->IsLocal()) continue;

		if (m_bEnablePrediction)
		{
			pRemote->PredictMovement(m_deltaTime);
		}

		D3DXVECTOR3 remotePos = pRemote->GetPosition();
		D3DXVECTOR3 remoteDir = pRemote->GetDepth();
		SoundManager::SetPosition(remotePos, remoteDir, CharacterBase::UP_DIRECTION, remoteId);

		unsigned char keyFlag = pRemote->GetKeyFlag();
		bool isMoving = (keyFlag & 0x0F) != 0;
		bool isDashing = (keyFlag & 0x20) != 0;
		bool isCrouching = (keyFlag & 0x10) != 0;

		PlayerRole localRole = m_localRole;
		PlayerRole remoteRole = ROLE_NONE;
		{
			auto it = m_playerRoles.find(remoteId);
			if (it != m_playerRoles.end()) remoteRole = it->second;
		}

		// ★★★ 修正: 足音だけ役割で判定、解凍音は全員が聞く ★★★
		bool shouldPlayFootsteps = (localRole != remoteRole);

		bool isBreathing = false;
		if (remoteRole == ROLE_CHASER)
		{
			Chaser* chaser = dynamic_cast<Chaser*>(pRemote);
			if (chaser) isBreathing = chaser->IsBreathing();
		}

		if (shouldLog)
		{
			NET_LOG_F("[UpdateRemotePlayers] ========== Player[%u] ==========", remoteId);
			NET_LOG_F("  Position: (%.1f, %.1f, %.1f)", remotePos.x, remotePos.y, remotePos.z);
			NET_LOG_F("  KeyFlag: 0x%02X (Moving=%s Dash=%s Crouch=%s)",
				keyFlag, isMoving ? "Yes" : "No", isDashing ? "Yes" : "No", isCrouching ? "Yes" : "No");
			NET_LOG_F("  Role: Local=%s Remote=%s",
				(localRole == ROLE_CHASER) ? "鬼" : "逃げる側",
				(remoteRole == ROLE_CHASER) ? "鬼" : "逃げる側");
		}

		// ===== 足音処理（役割が異なる場合のみ） =====
		if (isMoving && shouldPlayFootsteps && !isBreathing)
		{
			float footspeedParam = 0.6f;
			if (isDashing) footspeedParam = 1.0f;
			else if (isCrouching) footspeedParam = 0.3f;

			AK::SoundEngine::SetRTPCValue(AK::GAME_PARAMETERS::FOOTSPEED, footspeedParam, remoteId);

			if (footSoundMap[remoteId] == AK_INVALID_PLAYING_ID)
			{
				SoundManager::SetPosition(remotePos, remoteDir, CharacterBase::UP_DIRECTION, remoteId);
				footSoundMap[remoteId] = SoundManager::Play(AK::EVENTS::PLAY_SE_FOOT, remoteId);

				if (shouldLog)
				{
					NET_LOG_F("[UpdateRemotePlayers] ★足音再生開始★ Player[%u] PlayingID=%u Speed=%.2f",
						remoteId, footSoundMap[remoteId], footspeedParam);
				}
			}
		}
		else
		{
			if (footSoundMap[remoteId] != AK_INVALID_PLAYING_ID)
			{
				SoundManager::StopEvent(footSoundMap[remoteId]);
				footSoundMap[remoteId] = AK_INVALID_PLAYING_ID;

				if (shouldLog)
				{
					NET_LOG_F("[UpdateRemotePlayers] ★足音停止★ Player[%u]", remoteId);
				}
			}
		}

		// ===== ブレス（鬼）処理 =====
		if (remoteRole == ROLE_CHASER)
		{
			Chaser* chaser = dynamic_cast<Chaser*>(pRemote);
			if (chaser)
			{
				if (isBreathing)
				{
					if (breathSoundMap[remoteId] == AK_INVALID_PLAYING_ID)
					{
						SoundManager::SetPosition(remotePos, remoteDir, CharacterBase::UP_DIRECTION, remoteId);
						breathSoundMap[remoteId] = SoundManager::Play(AK::EVENTS::PLAY_SE_BRACELET, remoteId);

						if (shouldLog)
						{
							NET_LOG_F("[UpdateRemotePlayers] ★ブレス音再生開始★ Chaser[%u] PlayingID=%u",
								remoteId, breathSoundMap[remoteId]);
						}
					}
				}
				else
				{
					if (breathSoundMap[remoteId] != AK_INVALID_PLAYING_ID)
					{
						SoundManager::StopEvent(breathSoundMap[remoteId]);
						breathSoundMap[remoteId] = AK_INVALID_PLAYING_ID;

						if (shouldLog)
						{
							NET_LOG_F("[UpdateRemotePlayers] ★ブレス音停止★ Chaser[%u]", remoteId);
						}
					}
				}
			}
		}

		// ===== Runner（解凍/凍結）処理 ★★★ 役割に関係なく全員が聞く ★★★ =====
		if (remoteRole == ROLE_RUNNER)
		{
			Runner* runner = dynamic_cast<Runner*>(pRemote);
			if (runner)
			{
				uint32_t meltTarget = runner->GetMeltTargetId();
				bool isFrozen = runner->IsFrozen();

				bool wasFrozen = wasFrozenMap[remoteId];

				// ★★★ 凍結音は全員が聞く ★★★
				if (isFrozen && !wasFrozen)
				{
					SoundManager::SetPosition(remotePos, remoteDir, CharacterBase::UP_DIRECTION, remoteId);
					SoundManager::Play(AK::EVENTS::PLAY_SE_FREEZE, remoteId);

					if (shouldLog) NET_LOG_F("[UpdateRemotePlayers] ★凍結音再生★ Runner[%u]", remoteId);
				}

				if (meltTarget != lastMeltTarget[remoteId])
				{
					if (remoteMeltSounds[remoteId] != AK_INVALID_PLAYING_ID)
					{
						SoundManager::StopEvent(remoteMeltSounds[remoteId]);
						remoteMeltSounds[remoteId] = AK_INVALID_PLAYING_ID;
						if (shouldLog) NET_LOG_F("[UpdateRemotePlayers] ★解凍音停止(ターゲット変更)★ Runner[%u]", remoteId);
					}
					lastMeltTarget[remoteId] = meltTarget;
				}

				// ★★★ 解凍音は全員が聞く（役割判定を削除） ★★★
				if (meltTarget != 0 && isFrozen)
				{
					if (remoteMeltSounds[remoteId] == AK_INVALID_PLAYING_ID)
					{
						SoundManager::SetPosition(remotePos, remoteDir, CharacterBase::UP_DIRECTION, remoteId);
						remoteMeltSounds[remoteId] = SoundManager::Play(AK::EVENTS::PLAY_SE_THAWING, remoteId);

						if (shouldLog)
						{
							NET_LOG_F("[UpdateRemotePlayers] ★解凍音再生開始★ Runner[%u] PlayingID=%u Target=%u (LocalRole=%s)",
								remoteId, remoteMeltSounds[remoteId], meltTarget,
								(localRole == ROLE_CHASER) ? "鬼" : "逃げる側");
						}
					}
				}
				else
				{
					if (remoteMeltSounds[remoteId] != AK_INVALID_PLAYING_ID)
					{
						SoundManager::StopEvent(remoteMeltSounds[remoteId]);
						remoteMeltSounds[remoteId] = AK_INVALID_PLAYING_ID;
						if (shouldLog) NET_LOG_F("[UpdateRemotePlayers] ★解凍音停止★ Runner[%u]", remoteId);
					}
				}

				wasFrozenMap[remoteId] = isFrozen;
			}
		}

		// ===== 遮蔽・遮音の計算 =====
		if (m_pLocalPlayer)
		{
			D3DXVECTOR3 localPos = m_pLocalPlayer->GetPosition();
			D3DXVECTOR3 eyePos = localPos;
			eyePos.y += 1.5f;

			D3DXVECTOR3 intersection;
			bool blocked = m_map.RayToWallIntersection(eyePos, remotePos, &intersection);

			D3DXVECTOR3 dir_toRemote = remotePos - eyePos;
			float dist = D3DXVec3Length(&dir_toRemote);
			float maxDist = 50.0f;

			float distFactor = min(dist / maxDist, 1.0f) / 2.0f;

			const float MAX_OBSTRUCTION = 0.85f;
			const float MAX_OCCLUSION = 0.80f;

			float obstruction = blocked ? (0.6f + distFactor * 0.4f) : distFactor;
			float occlusion = blocked ? (0.4f + distFactor * 0.6f) : (distFactor * 0.3f);

			obstruction = min(obstruction, MAX_OBSTRUCTION);
			occlusion = min(occlusion, MAX_OCCLUSION);

			AK::SoundEngine::SetObjectObstructionAndOcclusion(
				remoteId,
				SoundManager::ID_LISTENER,
				obstruction,
				occlusion
			);

			if (shouldLog)
			{
				NET_LOG_F("  Occlusion: Dist=%.1f Blocked=%s Obst=%.2f Occl=%.2f",
					dist, blocked ? "Yes" : "No", obstruction, occlusion);
			}
		}
	}

	if (shouldLog)
	{
		lastLog = now;
	}
}
void SceneGame::CheckBreathHitPlayers()
{
	if (m_pLocalPlayer && m_localRole == ROLE_CHASER)
	{
		Chaser* localChaser = dynamic_cast<Chaser*>(m_pLocalPlayer);
		if (localChaser && localChaser->IsBreathing())
		{
			std::vector<std::pair<uint32_t, CharacterBase*>> playerList;
			for (auto& p : m_players)
			{
				if (m_playerRoles[p.first] == ROLE_RUNNER && p.second)
				{
					playerList.push_back(p);
				}
			}

			std::map<uint32_t, bool> beforeFrozen;
			for (auto& p : playerList)
			{
				Runner* runner = dynamic_cast<Runner*>(p.second);
				if (runner)
				{
					beforeFrozen[p.first] = runner->IsFrozen();
				}
			}

			localChaser->CheckBreathHitPlayers(playerList);

			bool stateChanged = false;
			for (auto& p : playerList)
			{
				Runner* runner = dynamic_cast<Runner*>(p.second);
				if (runner && beforeFrozen[p.first] != runner->IsFrozen())
				{
					stateChanged = true;
					NET_LOG_F("[SceneGame::CheckBreathHitPlayers] プレイヤー[%u]の凍結状態が変化",
						p.first);
				}
			}

			if (stateChanged)
			{
				SyncToServer();
			}
		}
	}

	if (m_pLocalPlayer && m_localRole == ROLE_RUNNER)
	{
		Runner* localRunner = dynamic_cast<Runner*>(m_pLocalPlayer);
		if (!localRunner || localRunner->IsFrozen())
			return;

		for (auto& kv : m_players)
		{
			if (m_playerRoles[kv.first] != ROLE_CHASER)
				continue;

			Chaser* chaser = dynamic_cast<Chaser*>(kv.second);
			if (!chaser || !chaser->IsBreathing())
				continue;

			SpotLight* light = chaser->GetLights();
			if (!light)
				continue;

			const D3DLIGHT9& lightData = light->GetLight();
			D3DXVECTOR3 lightPos(lightData.Position.x, lightData.Position.y, lightData.Position.z);
			D3DXVECTOR3 lightDir(lightData.Direction.x, lightData.Direction.y, lightData.Direction.z);
			float lightRange = lightData.Range;
			float lightConeAngle = lightData.Theta;

			D3DXVECTOR3 playerPos = localRunner->GetCenterPosition();
			D3DXVECTOR3 toPlayer = playerPos - lightPos;
			float distance = D3DXVec3Length(&toPlayer);

			if (distance > lightRange)
				continue;

			D3DXVec3Normalize(&toPlayer, &toPlayer);
			float dotProduct = D3DXVec3Dot(&toPlayer, &lightDir);
			float coneThreshold = cosf(lightConeAngle * 2.0f);

			if (dotProduct < coneThreshold)
				continue;

			localRunner->SetFrozen(true);
			NET_LOG_F("[SceneGame] ★★★ローカルプレイヤー[%u]が鬼[%u]のブレスに当たった！★★★",
				m_localClientId, kv.first);

			SyncToServer();
			return;
		}
	}
}

void SceneGame::ProcessPlayerMelting()
{
	if (!m_bIsHost || !m_pServer) return;

	std::map<uint32_t, std::vector<uint32_t>> targetToHelpers;

	// ★ ホスト自身（ローカルプレイヤー）も含めて収集 ★
	auto collectRunner = [&](uint32_t id, CharacterBase* base)
	{
		if (!base) return;

		if (m_playerRoles[id] != ROLE_RUNNER) return;

		Runner* runner = dynamic_cast<Runner*>(base);
		if (!runner || runner->IsFrozen()) return;

		uint32_t targetId = runner->GetMeltTargetId();
		if (targetId != 0)
		{
			targetToHelpers[targetId].push_back(id);
		}
	};

	// ローカル（ホスト自身）
	if (m_pLocalPlayer)
	{
		collectRunner(m_localClientId, m_pLocalPlayer);
	}

	// リモート
	for (auto& kv : m_players)
	{
		collectRunner(kv.first, kv.second);
	}

	bool stateChanged = false;

	for (auto& pair : targetToHelpers)
	{
		uint32_t targetId = pair.first;
		const auto& helpers = pair.second;

		Runner* target = nullptr;

		// ターゲットがローカル（ホスト自身）の場合
		if (targetId == m_localClientId)
		{
			target = dynamic_cast<Runner*>(m_pLocalPlayer);
		}
		else
		{
			auto it = m_players.find(targetId);
			if (it != m_players.end())
			{
				target = dynamic_cast<Runner*>(it->second);
			}
		}

		if (!target || !target->IsFrozen()) continue;

		float totalSpeed = 0.0f;

		for (uint32_t helperId : helpers)
		{
			Runner* helper = nullptr;

			if (helperId == m_localClientId)
			{
				helper = dynamic_cast<Runner*>(m_pLocalPlayer);
			}
			else
			{
				auto it = m_players.find(helperId);
				if (it != m_players.end())
				{
					helper = dynamic_cast<Runner*>(it->second);
				}
			}

			if (helper)
			{
				totalSpeed += helper->GetMeltSpeed();
			}
		}

		float oldAmount = target->GetFrozenAmount();
		float newAmount = oldAmount + totalSpeed * m_deltaTime;

		if (newAmount > oldAmount)
		{
			if (newAmount >= 1.0f)
			{
				target->SetFrozenAmount(1.0f);
				target->SetFrozen(false);
			}
			else
			{
				target->SetFrozenAmount(newAmount);
			}

			stateChanged = true;
		}
	}

	// ★ 必ずサーバー状態を配信 ★
	if (stateChanged)
	{
		m_pServer->BroadcastWorldState();
	}
}

// SceneGame::ReceiveWorldState - 差し替え用：全文
void SceneGame::ReceiveWorldState()
{
	if (!m_pClient) return;

	NetWorldState world;
	if (!m_pClient->GetWorldState(world)) return;

	DWORD now = timeGetTime();

	for (int i = 0; i < world.playerCount; i++)
	{
		const NetPlayerState& ps = world.players[i];

		// ===== ローカルプレイヤー（自分自身）処理 =====
		if (ps.clientId == m_localClientId)
		{
			if (!m_pLocalPlayer) continue;

			// Runner の凍結状態だけはサーバーの情報で反映させる（ホストでも）
			if (m_localRole == ROLE_RUNNER)
			{
				Runner* localRunner = dynamic_cast<Runner*>(m_pLocalPlayer);
				if (!localRunner) continue;

				bool newFrozen = (ps.frozen != 0);
				float netAmount = ps.frozenAmount;

				bool wasFrozen = localRunner->IsFrozen();
				float oldAmount = localRunner->GetFrozenAmount();

				// 新規凍結開始
				if (!wasFrozen && newFrozen)
				{
					localRunner->SetFrozen(true);
					localRunner->SetFrozenAmount(netAmount);
				}
				// 解凍進行（増加分のみ反映）
				else if (wasFrozen && newFrozen && netAmount > oldAmount + 0.001f)
				{
					localRunner->SetFrozenAmount(netAmount);
				}
				// 完全解凍
				else if (wasFrozen && !newFrozen)
				{
					localRunner->SetFrozen(false);
					localRunner->SetFrozenAmount(1.0f);
				}
			}

			// ★ ホストは自分の位置／向き等のネットワーク適用を行わない（ローカルで処理済み）
			//    また、クライアント側でも自分には UpdateFromNetwork を通常適用しないこと
			if (m_bIsHost)
			{
				continue;
			}

			// クライアントの場合はローカルプレイヤーに対しても UpdateFromNetwork を適用する
			m_pLocalPlayer->UpdateFromNetwork(ps, m_light, m_deltaTime);
			continue;
		}

		// ===== リモートプレイヤー =====
		auto it = m_players.find(ps.clientId);
		if (it == m_players.end() || !it->second) continue;

		// 追加ガード：対象オブジェクトがローカルフラグを持っていれば適用しない（上書きを避ける）
		if (it->second->IsLocal()) continue;

		// リモートプレイヤーへネットワーク状態を適用
		it->second->UpdateFromNetwork(ps, m_light, m_deltaTime);
	}
}
void SceneGame::RenderShadowMaps()
{
	LPDIRECT3DDEVICE9 pDevice = m_pEngine->GetDevice();

	static DWORD lastLog = 0;
	DWORD now = timeGetTime();
	bool shouldLog = (now - lastLog > 5000);

	if (shouldLog)
	{
		NET_LOG_F("[RenderShadowMaps] ========== 開始 ==========");
		NET_LOG_F("[RenderShadowMaps] m_players数=%d ローカルID=%u ローカルRole=%s",
			(int)m_players.size(),
			m_localClientId,
			m_localRole == ROLE_CHASER ? "鬼" : "逃げる側");

		NET_LOG_F("[RenderShadowMaps] m_playersの内容:");
		for (auto& kv : m_players)
		{
			auto roleIt = m_playerRoles.find(kv.first);
			const char* roleName = "未設定";
			if (roleIt != m_playerRoles.end())
			{
				roleName = (roleIt->second == ROLE_CHASER) ? "鬼" : "逃げる側";
			}
			NET_LOG_F("  Player[%u]: Exists=%s Role=%s Ptr=%p",
				kv.first,
				kv.second ? "Yes" : "No",
				roleName,
				kv.second);
		}

		NET_LOG_F("[RenderShadowMaps] m_pLocalPlayer: Ptr=%p IsLocal=%s",
			m_pLocalPlayer,
			m_pLocalPlayer ? (m_pLocalPlayer->IsLocal() ? "Yes" : "No") : "N/A");
	}

	LPDIRECT3DSURFACE9 pOldBackBuffer = nullptr;
	LPDIRECT3DSURFACE9 pOldDepthBuffer = nullptr;
	pDevice->GetRenderTarget(0, &pOldBackBuffer);
	pDevice->GetDepthStencilSurface(&pOldDepthBuffer);

	D3DVIEWPORT9 oldViewport;
	pDevice->GetViewport(&oldViewport);

	DWORD oldCullMode, oldZEnable, oldZWriteEnable, oldColorWriteEnable;
	pDevice->GetRenderState(D3DRS_CULLMODE, &oldCullMode);
	pDevice->GetRenderState(D3DRS_ZENABLE, &oldZEnable);
	pDevice->GetRenderState(D3DRS_ZWRITEENABLE, &oldZWriteEnable);
	pDevice->GetRenderState(D3DRS_COLORWRITEENABLE, &oldColorWriteEnable);

	int shadowMapCount = 0;

	for (auto& kv : m_players)
	{
		if (shouldLog)
		{
			NET_LOG_F("[RenderShadowMaps] ループ開始: Player[%u]", kv.first);
		}

		if (!kv.second)
		{
			if (shouldLog) NET_LOG_F("[RenderShadowMaps] スキップ: Player[%u] - プレイヤーが存在しない", kv.first);
			continue;
		}

		auto roleIt = m_playerRoles.find(kv.first);
		if (roleIt == m_playerRoles.end())
		{
			if (shouldLog) NET_LOG_F("[RenderShadowMaps] スキップ: Player[%u] - 役割が未設定", kv.first);
			continue;
		}

		if (roleIt->second != ROLE_CHASER)
		{
			if (shouldLog) NET_LOG_F("[RenderShadowMaps] スキップ: Player[%u] - 役割が鬼ではない（%s）",
				kv.first, roleIt->second == ROLE_RUNNER ? "逃げる側" : "不明");
			continue;
		}

		Chaser* chaser = dynamic_cast<Chaser*>(kv.second);
		if (!chaser)
		{
			if (shouldLog) NET_LOG_F("[RenderShadowMaps] スキップ: Player[%u] - Chaserへのキャスト失敗", kv.first);
			continue;
		}

		if (!chaser->IsShadowMapEnabled())
		{
			if (shouldLog) NET_LOG_F("[RenderShadowMaps] スキップ: Player[%u] - シャドウマップ無効", kv.first);
			continue;
		}

		if (shouldLog)
		{
			NET_LOG_F("[RenderShadowMaps] ========== Chaser[%u]のシャドウマップ生成開始 ==========", kv.first);
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

		D3DXMATRIX matLightView = chaser->GetLightViewMatrix();
		D3DXMATRIX matLightProj = chaser->GetLightProjectionMatrix();
		D3DXMATRIX matLightVP = matLightView * matLightProj;

		pDevice->SetTransform(D3DTS_VIEW, &matLightView);
		pDevice->SetTransform(D3DTS_PROJECTION, &matLightProj);

		if (shouldLog)
		{
			NET_LOG_F("[RenderShadowMaps] ライト行列設定: View._41=%.2f, _42=%.2f, _43=%.2f",
				matLightView._41, matLightView._42, matLightView._43);
			NET_LOG_F("[RenderShadowMaps] マップの深度描画");
		}

		m_map.DrawMapDepth(m_pEngine, &matLightVP);

		if (shouldLog)
		{
			NET_LOG_F("[RenderShadowMaps] プレイヤーの影描画開始（鬼[%u]以外）", kv.first);
		}

		int drawnCount = 0;
		for (auto& kv2 : m_players)
		{
			if (!kv2.second)
			{
				if (shouldLog) NET_LOG_F("  Player[%u]: スキップ（存在しない）", kv2.first);
				continue;
			}

			if (kv2.first == kv.first)
			{
				if (shouldLog) NET_LOG_F("  Player[%u]: スキップ（鬼自身）", kv2.first);
				continue;
			}

			if (shouldLog) NET_LOG_F("  Player[%u]: DrawDepth呼び出し開始", kv2.first);
			kv2.second->DrawDepth(m_pEngine, &matLightVP);
			if (shouldLog) NET_LOG_F("  Player[%u]: DrawDepth呼び出し完了", kv2.first);
			drawnCount++;
		}

		if (m_pLocalPlayer && m_localClientId != kv.first)
		{
			bool alreadyDrawn = false;
			for (auto& kv2 : m_players)
			{
				if (kv2.first == m_localClientId)
				{
					alreadyDrawn = true;
					break;
				}
			}

			if (!alreadyDrawn)
			{
				if (shouldLog) NET_LOG_F("  LocalPlayer[%u]: DrawDepth呼び出し（m_playersに未登録）", m_localClientId);
				m_pLocalPlayer->DrawDepth(m_pEngine, &matLightVP);
				drawnCount++;
			}
			else
			{
				if (shouldLog) NET_LOG_F("  LocalPlayer[%u]: スキップ（m_playersに登録済み）", m_localClientId);
			}
		}

		if (shouldLog)
		{
			NET_LOG_F("[RenderShadowMaps] プレイヤーの影描画完了: %d体描画", drawnCount);
		}

		if (pShadowSurface) pShadowSurface->Release();
		shadowMapCount++;

		if (shouldLog)
		{
			NET_LOG_F("[RenderShadowMaps] ========== Chaser[%u]のシャドウマップ生成完了 ==========", kv.first);
		}
	}

	if (shouldLog)
	{
		NET_LOG_F("[RenderShadowMaps] 完了: シャドウマップ生成数=%d", shadowMapCount);
		NET_LOG_F("[RenderShadowMaps] ========== 終了 ==========");
		lastLog = now;
	}

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

void SceneGame::UpdateChaserLights()
	{
		m_chaserLights.clear();

		static DWORD lastLog = 0;
		DWORD now = timeGetTime();

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

		for (auto& kv : m_players)
		{
			if (kv.first == m_localClientId)
				continue;

			if (!kv.second)
				continue;

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

// SceneGame.cpp - SpawnPlayerWithRole() の修正版

void SceneGame::SpawnPlayerWithRole(uint32_t clientId, const std::string& name,
	const D3DXVECTOR3& pos, PlayerRole role)
{
	if (m_players.find(clientId) != m_players.end())
	{
		NET_LOG_F("[SceneGame] プレイヤー %u は既に存在 - スキップ", clientId);
		return;
	}

	NET_LOG_F("========================================");
	NET_LOG_F("[SceneGame] ★プレイヤー生成開始★ ID=%u Name=%s Role=%s",
		clientId, name.c_str(), (role == ROLE_CHASER) ? "鬼" : "逃げる側");

	CharacterBase* p = nullptr;

	if (role == ROLE_RUNNER)
	{
		p = new Runner();
	}
	else if (role == ROLE_CHASER)
	{
		p = new Chaser();
	}
	else
	{
		p = new Runner();
	}
	char gameObjName[64];
	sprintf_s(gameObjName, "Player_%u", clientId);
	SoundManager::RegisterGameObject(clientId, gameObjName);

	// ローカルプレイヤーであればリスナー設定や初期位置設定も行う
	if (clientId == m_localClientId)
	{
		// リスナーや位置など、必要ならここで設定
		SoundManager::SetPosition(pos, D3DXVECTOR3(0, 0, 1), CharacterBase::UP_DIRECTION, SoundManager::ID_LISTENER);
	}
	bool isLocal = (clientId == m_localClientId);

	// ★★★ 修正: 役割に応じたスタート位置を取得 ★★★
	D3DXVECTOR3 spawnPos;
	if (pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f)
	{
		// 位置が指定されていない場合、役割に応じたスタート位置を使用
		if (role == ROLE_CHASER)
		{
			spawnPos = m_map.GetChaserStartPosition();
			NET_LOG_F("[SceneGame] 鬼のスタート位置使用: (%.1f, %.1f, %.1f)",
				spawnPos.x, spawnPos.y, spawnPos.z);
		}
		else
		{
			spawnPos = m_map.GetRunnerStartPosition();
			NET_LOG_F("[SceneGame] 逃げる側のスタート位置使用: (%.1f, %.1f, %.1f)",
				spawnPos.x, spawnPos.y, spawnPos.z);
		}
	}
	else
	{
		// 位置が明示的に指定されている場合はそれを使用
		spawnPos = pos;
		NET_LOG_F("[SceneGame] 指定位置使用: (%.1f, %.1f, %.1f)",
			spawnPos.x, spawnPos.y, spawnPos.z);
	}

	// ★★★ ステップ1: Initialize() より前に ID とフラグを設定 ★★★
	NET_LOG_F("[SceneGame] ステップ1: ID設定開始");
	NET_LOG_F("  Client ID: %u", clientId);
	NET_LOG_F("  Object Name: %s", name.c_str());
	NET_LOG_F("  IsLocal: %s", isLocal ? "Yes" : "No");

	p->SetIsLocal(isLocal);
	p->SetClientId(clientId);
	p->SetCharacterName(name);
	p->SetPosition(spawnPos);

	NET_LOG_F("[SceneGame] ステップ1完了: ID=%u Name=%s 設定完了",
		p->GetClientId(), p->GetCharacterName().c_str());

	// ★★★ ステップ2: Wwiseに登録（IDが正しく設定された後）★★★
	const char* objName = (role == ROLE_CHASER) ? "Chaser" : "Runner";
	NET_LOG_F("[SceneGame] ステップ2: Wwise登録開始 GameObjectID=%u ObjName=%s",
		clientId, objName);

	SoundManager::RegisterGameObject(clientId, objName);

	NET_LOG_F("[SceneGame] ステップ2完了: Wwise登録完了");

	// ★★★ ステップ3: Initialize()呼び出し（IDは既に設定済み）★★★
	NET_LOG_F("[SceneGame] ステップ3: Initialize開始 (IsLocal=%s)", isLocal ? "Yes" : "No");

	if (isLocal)
	{
		if (role == ROLE_RUNNER)
		{
			((Runner*)p)->Initialize(m_pEngine, m_map, &m_projection, m_camera, m_light);
		}
		else
		{
			((Chaser*)p)->Initialize(m_pEngine, m_map, &m_projection, m_camera, m_light);
		}
		m_pLocalPlayer = p;
		NET_LOG_F("[SceneGame] ステップ3完了: ローカル初期化完了");
	}
	else
	{
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
		NET_LOG_F("[SceneGame] ステップ3完了: リモート初期化完了");
	}

	// ★★★ ステップ4: ID確認（Initialize後もIDが保持されているか確認）★★★
	NET_LOG_F("[SceneGame] ステップ4: ID確認");
	if (p->GetClientId() != clientId)
	{
		NET_LOG_F("[SceneGame] ★★★エラー★★★ IDが変更されている！ 期待=%u 実際=%u",
			clientId, p->GetClientId());
		NET_LOG_F("[SceneGame] IDを再設定");
		p->SetClientId(clientId);
	}
	NET_LOG_F("[SceneGame] ステップ4完了: ID確認OK GetClientId()=%u", p->GetClientId());

	// ★★★ ステップ5: マップに登録 ★★★
	m_players[clientId] = p;
	m_playerRoles[clientId] = role;
	NET_LOG_F("[SceneGame] ステップ5完了: マップ登録完了");

	// ★★★ ステップ6: リモートプレイヤーのテスト音再生 ★★★
	if (!isLocal)
	{
		NET_LOG_F("[SceneGame] ステップ6: リモートプレイヤー音テスト開始");
		NET_LOG_F("  Client ID: %u", clientId);
		NET_LOG_F("  Position: (%.1f, %.1f, %.1f)", spawnPos.x, spawnPos.y, spawnPos.z);

		// 3D位置設定
		D3DXVECTOR3 forward(0, 0, -1);
		D3DXVECTOR3 up(0, 1, 0);
		SoundManager::SetPosition(spawnPos, forward, up, clientId);
		NET_LOG_F("  3D位置設定完了");

		// テスト音再生（短く凍結音を鳴らす）
		AkPlayingID testId = SoundManager::Play(AK::EVENTS::PLAY_SE_FREEZE, clientId);
		NET_LOG_F("  テスト音再生: Event=PLAY_SE_FREEZE GameObjectID=%u PlayingID=%u",
			clientId, testId);

		if (testId == AK_INVALID_PLAYING_ID)
		{
			NET_LOG_F("  ★★★エラー★★★ テスト音再生失敗！");
		}
		else
		{
			NET_LOG_F("  ★成功★ テスト音再生成功");
			// 1秒後に停止
			Sleep(1000);
			SoundManager::StopEvent(testId);
			NET_LOG_F("  テスト音停止");
		}
	}

	NET_LOG_F("[SceneGame] ★プレイヤー生成完了★");
	NET_LOG_F("  Client ID: %u", clientId);
	NET_LOG_F("  Name: %s", name.c_str());
	NET_LOG_F("  Role: %s", (role == ROLE_CHASER) ? "鬼" : "逃げる側");
	NET_LOG_F("  IsLocal: %s", isLocal ? "Yes" : "No");
	NET_LOG_F("  Position: (%.1f, %.1f, %.1f)", spawnPos.x, spawnPos.y, spawnPos.z);
	NET_LOG_F("========================================");
}
void SceneGame::DespawnPlayer(uint32_t clientId)
{
	auto it = m_players.find(clientId);
	if (it != m_players.end())
	{
		// サウンドを止めてから Wwise の登録解除
		SoundManager::StopAll(clientId);
		SoundManager::UnregisterGameObject(clientId);

		// その後に delete 等既存処理
		if (m_playerRoles[clientId] == ROLE_RUNNER)
			((Runner*)it->second)->Release(m_pEngine);
		else if (m_playerRoles[clientId] == ROLE_CHASER)
			((Chaser*)it->second)->Release(m_pEngine);

		delete it->second;
		m_players.erase(it);
		m_playerRoles.erase(clientId);
	}
}
void SceneGame::Draw()
{
	for (auto& kv : m_players)
	{
		if (!kv.second || m_playerRoles[kv.first] != ROLE_CHASER)
			continue;

		Chaser* chaser = dynamic_cast<Chaser*>(kv.second);
		if (chaser)
		{
			chaser->UpdateLight(m_pEngine);
		}
	}

	if (m_pLocalPlayer && m_localRole == ROLE_CHASER)
	{
		Chaser* localChaser = dynamic_cast<Chaser*>(m_pLocalPlayer);
		if (localChaser)
		{
			localChaser->UpdateLight(m_pEngine);
		}
	}

	RenderShadowMaps();

	int lightIndex = 0;

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
				light->SetDevice(m_pEngine, lightIndex);
				lightIndex++;
			}
		}
	}

	UpdateChaserLights();

	std::vector<SpotLight> spotLights;
	std::vector<LPDIRECT3DTEXTURE9> shadowMaps;
	std::vector<D3DXMATRIX> lightViewProjs;
	std::vector<D3DXMATRIX> scaleBiases;

	for (size_t i = 0; i < m_chaserLights.size(); ++i)
	{
		if (!m_chaserLights[i])
			continue;

		const D3DLIGHT9& lightCheck = m_chaserLights[i]->GetLight();
		if (lightCheck.Position.x == 0.0f && lightCheck.Position.y == 0.0f && lightCheck.Position.z == 0.0f)
		{
			NET_LOG_F("[SceneGame::Draw] 警告: Light[%d]の位置がゼロ！", (int)i);
		}

		spotLights.push_back(*m_chaserLights[i]);

		Chaser* chaser = nullptr;

		if (m_pLocalPlayer && m_localRole == ROLE_CHASER)
		{
			Chaser* localChaser = dynamic_cast<Chaser*>(m_pLocalPlayer);
			if (localChaser && localChaser->GetLights() == m_chaserLights[i])
			{
				chaser = localChaser;
			}
		}

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

	static DWORD lastLog = 0;
	DWORD now = timeGetTime();
	if (now - lastLog > 3000)
	{
		NET_LOG_F("[SceneGame::Draw] 鬼のライト数: %d シャドウマップ数: %d",
			(int)spotLights.size(), (int)shadowMaps.size());

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

	std::vector<SpotLight>* pLights = spotLights.empty() ? nullptr : &spotLights;
	std::vector<LPDIRECT3DTEXTURE9>* pShadowMaps = shadowMaps.empty() ? nullptr : &shadowMaps;
	std::vector<D3DXMATRIX>* pLightViewProjs = lightViewProjs.empty() ? nullptr : &lightViewProjs;
	std::vector<D3DXMATRIX>* pScaleBiases = scaleBiases.empty() ? nullptr : &scaleBiases;

	m_pEngine->Clear(D3DCOLOR_XRGB(0, 0, 0));

	m_map.DrawMap(m_pEngine, &m_camera, &m_projection, &m_ambient, &m_light,
		pLights, pShadowMaps, pLightViewProjs, pScaleBiases);

	int drawnCount = 0;
	for (auto& kv : m_players)
	{
		if (kv.second)
		{
			kv.second->Draw(&m_camera, &m_projection, &m_ambient, &m_light);
			drawnCount++;
		}
	}

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

	static DWORD lastEffectLog = 0;
	DWORD nowEffect = timeGetTime();
	if (nowEffect - lastEffectLog > 3000)
	{
		int frozenCount = 0;
		for (auto& kv : m_players)
		{
			if (!kv.second) continue;
			Runner* runner = dynamic_cast<Runner*>(kv.second);
			if (runner && runner->IsFrozen())
			{
				frozenCount++;
				NET_LOG_F("[SceneGame::Draw] リモートプレイヤー[%u] 凍結中: amount=%.2f",
					kv.first, runner->GetFrozenAmount());
			}
		}

		if (m_pLocalPlayer && m_localRole == ROLE_RUNNER)
		{
			Runner* localRunner = dynamic_cast<Runner*>(m_pLocalPlayer);
			if (localRunner && localRunner->IsFrozen())
			{
				frozenCount++;
				NET_LOG_F("[SceneGame::Draw] ローカルプレイヤー[%u] 凍結中: amount=%.2f",
					m_localClientId, localRunner->GetFrozenAmount());
			}
		}

		NET_LOG_F("[SceneGame::Draw] エフェクト描画: 凍結プレイヤー=%d人", frozenCount);
		lastEffectLog = nowEffect;
	}

	if (m_pLocalPlayer)
	{
		m_pLocalPlayer->DrawEffects(m_pEngine, &m_camera, &m_projection, &m_ambient, &m_light);
	}

	for (auto& kv : m_players)
	{
		if (kv.first == m_localClientId) continue;
		if (!kv.second) continue;

		kv.second->DrawEffects(m_pEngine, &m_camera, &m_projection, &m_ambient, &m_light);
	}

	// ★★★ 壁越しバー描画（修正版：視線が遮られている場合のみ）★★★
	if (m_pLocalPlayer)
	{
		D3DXVECTOR3 cameraPos = m_camera.m_vecEye;

		if (m_localRole == ROLE_RUNNER)
		{
			for (auto& kv : m_players)
			{
				if (kv.first == m_localClientId) continue;
				if (!kv.second) continue;
				if (m_playerRoles[kv.first] != ROLE_RUNNER) continue;

				Runner* runner = dynamic_cast<Runner*>(kv.second);
				if (!runner || !runner->IsFrozen()) continue;

				D3DXVECTOR3 targetPos = runner->GetPosition();
				D3DXVECTOR3 diff = targetPos - cameraPos;
				float distance = D3DXVec3Length(&diff);

				D3DXVECTOR3 intersection;
				bool isBlocked = m_map.RayToWallIntersection(cameraPos, targetPos, &intersection);

				// ★★★ 修正: 視線が遮られている場合のみ壁越し描画 ★★★
				if (!isBlocked) continue;

				float alpha = f_wallAlphaNear;
				if (distance > f_wallDistanceMid) alpha = f_wallAlphaMid;
				else if (distance > f_wallDistanceFar) alpha = f_wallAlphaFar;

				IceBlock* iceBlock = runner->GetIceBlock();
				if (iceBlock)
				{
					iceBlock->DrawThroughWalls(m_pEngine, &m_camera, &m_projection, &m_ambient, &m_light, alpha);
				}

				runner->DrawMeltGaugeThroughWalls(m_pEngine, &m_camera, &m_projection, distance, alpha);
			}
		}
		else if (m_localRole == ROLE_CHASER)
		{
			for (auto& kv : m_players)
			{
				if (kv.first == m_localClientId) continue;
				if (!kv.second) continue;
				if (m_playerRoles[kv.first] != ROLE_RUNNER) continue;

				Runner* runner = dynamic_cast<Runner*>(kv.second);
				if (!runner || !runner->IsFrozen()) continue;

				D3DXVECTOR3 targetPos = runner->GetPosition();

				D3DXVECTOR2 cameraPos2D(cameraPos.x, cameraPos.z);
				D3DXVECTOR2 targetPos2D(targetPos.x, targetPos.z);
				D3DXVECTOR3 cameraPos3D_ForRay(cameraPos.x, cameraPos.y, cameraPos.z);
				D3DXVECTOR3 targetPos3D_ForRay(targetPos.x, cameraPos.y, targetPos.z);

				D3DXVECTOR3 intersection;
				bool isBlocked = m_map.RayToWallIntersection(cameraPos3D_ForRay, targetPos3D_ForRay, &intersection);

				D3DXVECTOR3 diff = targetPos - cameraPos;
				float distance = D3DXVec3Length(&diff);

				// ★★★ 修正: 距離内かつ視線が遮られている場合のみ壁越し描画 ★★★
				if (distance > f_chaserGaugeDistance || !isBlocked) continue;

				float alpha = f_wallAlphaNear;
				if (distance > 3.0f) alpha = f_wallAlphaMid;

				IceBlock* iceBlock = runner->GetIceBlock();
				if (iceBlock)
				{
					iceBlock->DrawThroughWallsFullSize(m_pEngine, &m_camera, &m_projection, &m_ambient, &m_light, alpha);
				}

				runner->DrawMeltGaugeThroughWalls(m_pEngine, &m_camera, &m_projection, distance, alpha);
			}
		}
	}

	// ★★★ 通常バー描画（壁がない場合のみ）★★★
	if (m_pLocalPlayer)
	{
		D3DXVECTOR3 localPos = m_pLocalPlayer->GetPosition();

		if (m_localRole == ROLE_RUNNER)
		{
			Runner* localRunner = dynamic_cast<Runner*>(m_pLocalPlayer);

			if (localRunner && localRunner->IsFrozen())
			{
				localRunner->DrawMeltGauge(m_pEngine, &m_camera, &m_projection, 0.0f);
			}

			for (auto& kv : m_players)
			{
				if (kv.first == m_localClientId) continue;
				if (!kv.second) continue;
				if (m_playerRoles[kv.first] != ROLE_RUNNER) continue;

				Runner* runner = dynamic_cast<Runner*>(kv.second);
				if (!runner) continue;
				if (!runner->IsFrozen()) continue;

				D3DXVECTOR3 targetPos = runner->GetPosition();
				D3DXVECTOR3 diff = targetPos - localPos;
				float distance = D3DXVec3Length(&diff);

				// ★★★ 修正: 視線が通っている場合のみ通常描画 ★★★
				D3DXVECTOR3 intersection;
				bool isBlocked = m_map.RayToWallIntersection(m_camera.m_vecEye, targetPos, &intersection);
				if (isBlocked) continue;

				runner->DrawMeltGauge(m_pEngine, &m_camera, &m_projection, distance);
			}
		}
		else if (m_localRole == ROLE_CHASER)
		{
			for (auto& kv : m_players)
			{
				if (kv.first == m_localClientId) continue;
				if (!kv.second) continue;
				if (m_playerRoles[kv.first] != ROLE_RUNNER) continue;

				Runner* runner = dynamic_cast<Runner*>(kv.second);
				if (!runner) continue;
				if (!runner->IsFrozen()) continue;

				D3DXVECTOR3 targetPos = runner->GetPosition();
				D3DXVECTOR3 diff = targetPos - localPos;
				float distance = D3DXVec3Length(&diff);

				if (distance > f_chaserGaugeDistance) continue;

				// ★★★ 修正: 視線が通っている場合のみ通常描画 ★★★
				D3DXVECTOR3 intersection;
				bool isBlocked = m_map.RayToWallIntersection(m_camera.m_vecEye, targetPos, &intersection);
				if (isBlocked) continue;

				runner->DrawMeltGauge(m_pEngine, &m_camera, &m_projection, distance);
			}
		}
	}

#if _DEBUG
	if (d_debugFlag & DRAW_BOXLINE)
	{
		m_map.DebugBoxLine(m_pEngine, &m_camera, &m_projection);
	}
#endif

	if (m_pLocalPlayer)
	{
		m_map.DrawMiniMap(m_pEngine, m_pLocalPlayer->GetPosition2D(), m_pLocalPlayer->GetArrowAngle());
	}

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
		m_pEngine->DrawPrintf(0, 50, FONT_GOTHIC40, Color::WHITE, "DEL : %f", m_deltaTime);
		m_pEngine->DrawPrintf(0, 100, FONT_GOTHIC40, Color::WHITE, "FPS : %f", (float)m_pEngine->GetFPS());
		m_pEngine->DrawPrintf(0, 900, FONT_GOTHIC40, Color::CYAN, "Players: %d (Drawn: %d) Lights: %d",
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

			const char* frozenStr = "";
			if (m_localRole == ROLE_RUNNER)
			{
				Runner* runner = dynamic_cast<Runner*>(m_pLocalPlayer);
				if (runner && runner->IsFrozen())
				{
					frozenStr = " [凍結中]";
					color = Color::BLUE;
				}
			}

			m_pEngine->DrawPrintf(0, yOffset, FONT_GOTHIC40, color,
				"Local[%u]: %s [%s]%s Pos=(%.1f,%.1f,%.1f)",
				m_localClientId, m_pLocalPlayer->GetCharacterName().c_str(), roleStr, frozenStr,
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

			const char* frozenStr = "";
			if (m_playerRoles[kv.first] == ROLE_RUNNER)
			{
				Runner* runner = dynamic_cast<Runner*>(kv.second);
				if (runner && runner->IsFrozen())
				{
					frozenStr = " [凍結中]";
					color = Color::BLUE;
				}
			}

			m_pEngine->DrawPrintf(0, yOffset, FONT_GOTHIC40, color,
				"Remote[%u]: %s [%s]%s Pos=(%.1f,%.1f,%.1f)",
				kv.first, kv.second->GetCharacterName().c_str(), roleStr, frozenStr,
				pos.x, pos.y, pos.z);
			yOffset += 50;
		}
	}
#endif
	if (m_gameState == IN_GAME && !m_bGameEnded)
	{
		float remaining = f_gameDuration - m_gameTime;
		int minutes = (int)(remaining / 60.0f);
		int seconds = (int)remaining % 60;

		D3DCOLOR timeColor = Color::WHITE;
		if (remaining < 60.0f) timeColor = Color::YELLOW;
		if (remaining < 30.0f) timeColor = Color::RED;

		m_pEngine->DrawPrintfCenter(WINDOW_WIDTH / 2, 50, FONT_GOTHIC60, timeColor,
			"残り時間: %02d:%02d", minutes, seconds);
	}

	// ★★★ 修正: リザルト画像表示（RESULT_DISPLAY と FADE_OUT 両方で表示）★★★
	if ((m_gameState == RESULT_DISPLAY || m_gameState == FADE_OUT) && m_winnerTeam != -1)
	{
		RECT sour, dest;

		// ★★★ 背景暗転用の画像（リザルト画面の後ろ）★★★
		SetRect(&sour, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
		m_pEngine->Blt(&sour, TEXTURE_FADE, &sour, 150, 0.0f);

		// ★★★ ローカルプレイヤーの勝敗判定 ★★★
		bool isLocalWinner = false;
		if (m_localRole == ROLE_RUNNER)
		{
			isLocalWinner = (m_winnerTeam == 0);
		}
		else if (m_localRole == ROLE_CHASER)
		{
			isLocalWinner = (m_winnerTeam == 1);
		}

		// ★★★ リザルト画像の描画 ★★★
		SetRect(&sour, 0, 0, (int)f_resultSize.x, (int)f_resultSize.y);

		D3DXVECTOR2 center = { (float)WINDOW_WIDTH / 2.0f, (float)(WINDOW_HEIGHT / 2.0f) };
		SetRect(&dest,
			(int)(center.x - f_resultSize.x / 2),
			(int)(center.y - f_resultSize.y / 2),
			(int)(center.x + f_resultSize.x / 2),
			(int)(center.y + f_resultSize.y / 2));

		int displayAlpha = (int)min(255.0f, m_resultImageAlpha);

		if (isLocalWinner)
		{
			m_pEngine->Blt(&dest, TEXTURE_VICTORY, &sour, displayAlpha, 0.0f);
		}
		else
		{
			m_pEngine->Blt(&dest, TEXTURE_DEFEAT, &sour, displayAlpha, 0.0f);
		}
	}

	// ★★★ フェードは最後に描画（リザルト画像の上に重ねる）★★★
	// FADE_OUT状態の時のみフェード画像を描画
	if (m_gameState == FADE_OUT || m_gameState == FADE_IN)
	{
		m_fade.Draw(m_pEngine);
	}

	m_pEngine->SpriteEnd();
}
void SceneGame::PostEffect()
{
}

void SceneGame::Exit()
{
	NET_LOG("[SceneGame] Exit開始");

	// ★★★ 1. BGM停止 ★★★
	SoundManager::StopAll(SoundManager::ID_LISTENER);
	NET_LOG("[SceneGame] BGM停止完了");

	// ★★★ 2. 全リモートプレイヤーの音を停止 ★★★
	for (auto& kv : m_players)
	{
		if (kv.second)
		{
			// そのプレイヤーIDに紐づく全音を停止
			SoundManager::StopAll(kv.first);
			NET_LOG_F("[SceneGame] Player[%u]の全音停止完了", kv.first);
		}
	}

	// ★★★ 3. ローカルプレイヤーの音を停止 ★★★
	if (m_pLocalPlayer)
	{
		// ローカルプレイヤーIDに紐づく全音を停止
		SoundManager::StopAll(m_localClientId);
		NET_LOG_F("[SceneGame] LocalPlayer[%u]の全音停止完了", m_localClientId);
	}

	NET_LOG("[SceneGame] 全音停止完了");

	// ★★★ 4. プレイヤーの削除（既存処理） ★★★
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
	m_pEngine->ReleaseFont(FONT_GOTHIC60);
	m_pEngine->ReleaseTexture(TEXTURE_VICTORY);
	m_pEngine->ReleaseTexture(TEXTURE_DEFEAT);
	m_pEngine->ReleaseTexture(TEXTURE_FADE);
	m_pEngine->ReleaseModel(MODEL_CHARACTER);

	NET_LOG("[SceneGame] Exit完了");
}
void SceneGame::LoadGameParameter()
{
	std::ifstream file(JSON_GAME_PARAMETER);
	if (!file.is_open())
	{
		throw DxSystemException(DxSystemException::OM_FILE_OPEN_ERROR);
	}

	nlohmann::json config;
	file >> config;
	file.close();

	f_maxGaugeDistance = config.value("maxGaugeDistance", 10.0f);
	f_chaserGaugeDistance = config.value("chaserGaugeDistance", 5.0f);
	f_wallAlphaFar = config.value("wallAlphaFar", 0.5f);
	f_wallAlphaMid = config.value("wallAlphaMid", 0.4f);
	f_wallAlphaNear = config.value("wallAlphaNear", 0.7f);
	f_wallDistanceMid = config.value("wallDistanceMid", 15.0f);
	f_wallDistanceFar = config.value("wallDistanceFar", 30.0f);
	f_networkSendInterval = config.value("networkSendInterval", 16);
	f_worldBroadcastInterval = config.value("worldBroadcastInterval", 8);

	// ★★★ ゲーム時間設定（デフォルト5分）★★★
	f_gameDuration = config.value("gameDuration", 300.0f);
	f_resultDisplayDuration = config.value("resultDisplayDuration", 5000);
	f_resultFadeSpeed = config.value("resultFadeSpeed", 200.0f);

	for (int i = 0; i < 2; i++)
	{
		f_resultSize[i] = config["resultSize"][i];
	}
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