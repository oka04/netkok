#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include "ServerManager.h"
#include "..\\ClientManager\\ClientManager.h"
#include "..\\Discovery\\Discovery.h"
#include "..\\NetworkLogger.h"
#include <windows.h>
#include <iostream>
#include <cstring>

#pragma warning(disable:4996)
#pragma warning(disable:26812)
#pragma warning(disable:26495)
#pragma warning(disable:6387)

#pragma comment(lib, "enetlib.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")

ServerManager* ServerManager::s_instance = nullptr;

ServerManager* ServerManager::GetInstance()
{
	if (!s_instance) s_instance = new ServerManager();
	return s_instance;
}

ServerManager::ServerManager()
	: m_pServerHost(nullptr)
	, m_clientCount(0)
	, m_advertiser(nullptr)
	, m_lastAdvertiseTime(0)
	, m_nextClientId(1)
	, m_serverName("Silent Host")
	, m_hostStateSet(false)
{
	if (enet_initialize() != 0)
	{
		MessageBoxA(NULL, "ENetの初期化に失敗しました。", "エラー", MB_OK);
	}
}

ServerManager::~ServerManager()
{
	StopServer();
	enet_deinitialize();

	if (s_instance == this) s_instance = nullptr;
}

bool ServerManager::StartServer(int port, int maxClients)
{
	ENetAddress address;
	address.host = ENET_HOST_ANY;
	address.port = (enet_uint16)port;

	m_pServerHost = enet_host_create(&address, maxClients, 2, 0, 0);
	if (m_pServerHost == nullptr)
	{
		NET_LOG("[ServerManager] サーバーホスト作成失敗");
		return false;
	}

	const uint16_t discoveryPort = 12346;
	m_advertiser = std::make_unique<Discovery>();
	m_advertiser->StartAdvertise(discoveryPort, (enet_uint16)port, m_serverName, (uint8_t)maxClients);
	m_advertiser->SetAdvertisePlayerCount(1);
	m_advertiser->SetAdvertiseState(0);
	m_clientCount = 0;
	m_nextClientId = 1;
	m_hostStateSet = false;

	NET_LOG_F("[ServerManager] サーバー起動: ポート=%d", port);
	std::cout << "[Server] 起動: ポート " << port << std::endl;
	return true;
}

void ServerManager::Reset()
{
	NET_LOG("[ServerManager] Reset開始");

	StopServer();

	m_clientCount = 0;
	m_nextClientId = 1;
	m_serverName = "Silent Host";
	m_hostName = "";
	m_lastAdvertiseTime = 0;
	m_hostStateSet = false;

	NET_LOG("[ServerManager] Reset完了");
}

void ServerManager::StopServer()
{
	if (m_pServerHost)
	{
		NET_LOG("[ServerManager] サーバー停止処理開始");

		for (auto& kv : m_clients)
		{
			if (kv.first && kv.first->state == ENET_PEER_STATE_CONNECTED)
			{
				NET_LOG_F("[ServerManager] クライアント %s に切断通知", kv.second->name.c_str());
				enet_peer_disconnect_now(kv.first, 0);
			}
		}

		for (auto& kv : m_clients)
		{
			delete kv.second;
		}
		m_clients.clear();

		enet_host_destroy(m_pServerHost);
		m_pServerHost = nullptr;
		m_clientCount = 0;

		NET_LOG("[ServerManager] サーバー停止完了");
		std::cout << "[Server] 停止" << std::endl;
	}

	if (m_advertiser)
	{
		m_advertiser->StopAdvertise();
		m_advertiser.reset();
	}
}

int ServerManager::GetClientCount() const
{
	return m_clientCount;
}

void ServerManager::BroadcastLobbyUpdate()
{
	if (!m_pServerHost) return;

	std::vector<std::string> allPlayers;

	if (!m_hostName.empty())
	{
		allPlayers.push_back(m_hostName);
	}

	for (auto& kv : m_clients)
	{
		if (kv.second->name != m_hostName)
		{
			allPlayers.push_back(kv.second->name);
		}
	}

	NET_LOG_F("[ServerManager] BroadcastLobbyUpdate: %d 人", (int)allPlayers.size());

	std::vector<uint8_t> payload;
	payload.push_back((uint8_t)MSG_LOBBY_UPDATE);
	uint8_t count = (uint8_t)allPlayers.size();
	payload.push_back(count);

	int playerIndex = 0;
	for (const auto& name : allPlayers)
	{
		size_t nameSize = name.size();
		if (nameSize > 255) nameSize = 255;
		uint8_t nl = (uint8_t)nameSize;
		payload.push_back(nl);
		payload.insert(payload.end(), name.begin(), name.begin() + nl);

		NET_LOG_F("[ServerManager] プレイヤー%d: '%s' (長さ:%d)", ++playerIndex, name.c_str(), (int)nl);
	}

	NET_LOG_F("[ServerManager] 送信データサイズ: %d bytes", (int)payload.size());

	ENetPacket* packet = enet_packet_create(payload.data(), payload.size(), ENET_PACKET_FLAG_RELIABLE);
	enet_host_broadcast(m_pServerHost, 0, packet);
	enet_host_flush(m_pServerHost);

	if (m_advertiser)
	{
		m_advertiser->SetAdvertisePlayerCount((uint8_t)allPlayers.size());
		m_advertiser->SetAdvertiseState(0);
	}
}


void ServerManager::SetServerName(std::string& name)
{
	m_serverName = name;
}

const std::string& ServerManager::GetServerName() const
{
	return m_serverName;
}

void ServerManager::SetHostName(const std::string& name)
{
	m_hostName = name;
	NET_LOG_F("[ServerManager] ホスト名設定: %s", name.c_str());
}

const std::string& ServerManager::GetHostName() const
{
	return m_hostName;
}

std::vector<std::string> ServerManager::GetLobbyPlayerNames() const
{
	std::vector<std::string> names;

	if (!m_hostName.empty())
	{
		names.push_back(m_hostName);
	}

	for (auto& kv : m_clients)
	{
		if (kv.second->name != m_hostName)
		{
			names.push_back(kv.second->name);
		}
	}

	return names;
}

void ServerManager::Update()
{
	if (!m_pServerHost) return;

	ENetEvent event;
	while (enet_host_service(m_pServerHost, &event, 0) > 0)
	{
		switch (event.type)
		{
		case ENET_EVENT_TYPE_CONNECT:
			OnClientConnect(event.peer);
			break;

		case ENET_EVENT_TYPE_RECEIVE:
			OnClientReceive(event);
			break;

		case ENET_EVENT_TYPE_DISCONNECT:
			OnClientDisconnect(event.peer);
			break;
		}
	}
}

void ServerManager::OnClientConnect(ENetPeer* peer)
{
	auto ci = new ClientInfo();
	ci->peer = peer;
	ci->id = m_nextClientId++;
	ci->name = "";
	ci->stateReceived = false;

	peer->data = ci;
	m_clients[peer] = ci;
	m_clientCount++;

	NET_LOG_F("[ServerManager] クライアント接続: ID=%d (現在%d人)", ci->id, m_clientCount);
}

void ServerManager::OnClientReceive(const ENetEvent& event)
{
	const uint8_t* data = (const uint8_t*)event.packet->data;
	size_t len = event.packet->dataLength;
	if (len < 1)
	{
		enet_packet_destroy(event.packet);
		return;
	}

	uint8_t id = data[0];
	switch (id)
	{
	case MSG_JOIN:
		ProcessJoin(event.peer, data + 1, len - 1);
		break;
	case MSG_PLAYER_STATE:
		ProcessPlayerState(event.peer, data + 1, len - 1);
		break;
	}

	enet_packet_destroy(event.packet);
}

void ServerManager::OnClientDisconnect(ENetPeer* peer)
{
	auto it = m_clients.find(peer);
	if (it != m_clients.end())
	{
		uint32_t clientId = it->second->id;
		std::string clientName = it->second->name;

		NET_LOG_F("[ServerManager] クライアント切断: %s (ID=%u)", clientName.c_str(), clientId);

		BroadcastPlayerDespawn(clientId);

		delete it->second;
		m_clients.erase(it);
		m_clientCount--;

		std::cout << "[Server] クライアント切断 (" << m_clientCount << "人)" << std::endl;

		BroadcastLobbyUpdate();
	}
}

void ServerManager::ProcessJoin(ENetPeer* peer, const uint8_t* data, size_t len)
{
	NET_LOG_F("[ServerManager] ProcessJoin: データ長=%d", (int)len);

	size_t idx = 0;
	if (idx >= len)
	{
		NET_LOG("[ServerManager] エラー: データ長不足");
		return;
	}

	uint8_t nl = data[idx++];
	NET_LOG_F("[ServerManager] 名前長さ: %d", (int)nl);

	if (idx + nl > len)
	{
		NET_LOG("[ServerManager] エラー: 名前データ不足");
		return;
	}

	std::string name(reinterpret_cast<const char*>(data + idx), nl);
	NET_LOG_F("[ServerManager] 受信した名前: '%s'", name.c_str());

	ClientInfo* ci = static_cast<ClientInfo*>(peer->data);
	if (ci)
	{
		if (ci->name.empty())
		{
			if (m_clientCount == 1 && !m_hostName.empty())
			{
				ci->name = m_hostName;
				NET_LOG_F("[ServerManager] ホストのJOIN受信: %s (ID=%d)", ci->name.c_str(), ci->id);
			}
			else
			{
				ci->name = name;
				NET_LOG_F("[ServerManager] クライアントのJOIN受信: %s (ID=%d)", ci->name.c_str(), ci->id);
			}

			// クライアントIDを送信
			SendJoinAck(peer, ci->id);

			//既存の全プレイヤー情報を新規参加者に送信
			//ホストの情報を送信
			if (!m_hostName.empty())
			{
				NetPlayerSpawn hostSpawn;
				hostSpawn.clientId = 1; 
				//ホストの現在位置を取得
				if (m_hostStateSet)
				{
					hostSpawn.startX = m_hostState.posX;
					hostSpawn.startY = m_hostState.posY;
					hostSpawn.startZ = m_hostState.posZ;
				}
				else
				{
					hostSpawn.startX = 0.0f;
					hostSpawn.startY = 0.0f;
					hostSpawn.startZ = 0.0f;
				}

				strncpy_s(hostSpawn.name, m_hostName.c_str(), sizeof(hostSpawn.name) - 1);
				hostSpawn.name[sizeof(hostSpawn.name) - 1] = '\0';

				auto hostData = NetworkSerializer::SerializePlayerSpawn(hostSpawn);
				ENetPacket* hostPacket = enet_packet_create(hostData.data(), hostData.size(), ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, hostPacket);
				NET_LOG_F("[ServerManager] 新規参加者にホスト情報を送信: %s Pos=(%.1f,%.1f,%.1f)",
					m_hostName.c_str(), hostSpawn.startX, hostSpawn.startY, hostSpawn.startZ);
			}

			//既存の他のクライアント情報を送信
			for (const auto& kv : m_clients)
			{
				if (kv.first != peer && kv.second->name != m_hostName && !kv.second->name.empty())
				{
					NetPlayerSpawn existingSpawn;
					existingSpawn.clientId = kv.second->id;

					//既存クライアントの現在位置を取得
					if (kv.second->stateReceived)
					{
						existingSpawn.startX = kv.second->lastState.posX;
						existingSpawn.startY = kv.second->lastState.posY;
						existingSpawn.startZ = kv.second->lastState.posZ;
					}
					else
					{
						existingSpawn.startX = 0.0f;
						existingSpawn.startY = 0.0f;
						existingSpawn.startZ = 0.0f;
					}

					strncpy_s(existingSpawn.name, kv.second->name.c_str(), sizeof(existingSpawn.name) - 1);
					existingSpawn.name[sizeof(existingSpawn.name) - 1] = '\0';

					auto existingData = NetworkSerializer::SerializePlayerSpawn(existingSpawn);
					ENetPacket* existingPacket = enet_packet_create(existingData.data(), existingData.size(), ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(peer, 0, existingPacket);
					NET_LOG_F("[ServerManager] 新規参加者に既存プレイヤー情報を送信: %s (ID=%u) Pos=(%.1f,%.1f,%.1f)",
						kv.second->name.c_str(), kv.second->id,
						existingSpawn.startX, existingSpawn.startY, existingSpawn.startZ);
				}
			}

			NetPlayerSpawn newSpawn;
			newSpawn.clientId = ci->id;
			newSpawn.startX = 0.0f; 
			newSpawn.startY = 0.0f;
			newSpawn.startZ = 0.0f;
			strncpy_s(newSpawn.name, ci->name.c_str(), sizeof(newSpawn.name) - 1);
			newSpawn.name[sizeof(newSpawn.name) - 1] = '\0';

			BroadcastPlayerSpawn(newSpawn);
			NET_LOG_F("[ServerManager] 新規参加者情報をブロードキャスト: %s (ID=%u)",
				ci->name.c_str(), ci->id);

			//フラッシュして確実に送信
			enet_host_flush(m_pServerHost);

			//ロビー更新をブロードキャスト
			NET_LOG("[ServerManager] ロビー更新をブロードキャスト");
			BroadcastLobbyUpdate();
		}
		else
		{
			NET_LOG_F("[ServerManager] 重複JOIN無視: %s は既に登録済み", ci->name.c_str());
		}
	}
}

void ServerManager::ProcessPlayerState(ENetPeer* peer, const uint8_t* data, size_t len)
{
	if (len < sizeof(NetPlayerState)) return;

	NetPlayerState state;
	if (!NetworkSerializer::DeserializePlayerState(data, len, state))
		return;

	std::lock_guard<std::mutex> lk(m_stateMutex);

	auto it = m_clients.find(peer);
	if (it != m_clients.end())
	{
		it->second->lastState = state;
		it->second->lastState.clientId = it->second->id;
		it->second->stateReceived = true;

		// ★★★ デバッグ: meltTargetIdの受信確認 ★★★
		static std::map<uint32_t, uint32_t> lastTargets;
		if (state.meltTargetId != lastTargets[it->second->id])
		{
			NET_LOG_F("[ServerManager::ProcessPlayerState] クライアント[%u] meltTarget更新: %u -> %u",
				it->second->id, lastTargets[it->second->id], state.meltTargetId);
			lastTargets[it->second->id] = state.meltTargetId;
		}
	}
}

void ServerManager::SetHostState(const NetPlayerState& state)
{
	std::lock_guard<std::mutex> lk(m_stateMutex);
	m_hostState = state;
	m_hostState.clientId = 1;
	m_hostStateSet = true;

	// ★★★ デバッグ: ホストのmeltTargetId確認 ★★★
	static uint32_t lastHostTarget = 0;
	if (state.meltTargetId != lastHostTarget)
	{
		NET_LOG_F("[ServerManager::SetHostState] ホスト meltTarget更新: %u -> %u",
			lastHostTarget, state.meltTargetId);
		lastHostTarget = state.meltTargetId;
	}
}

void ServerManager::ProcessMeltingOnServer()
{
	std::lock_guard<std::mutex> lk(m_stateMutex);

	// ★★★ 解凍対象とヘルパーのマップを作成 ★★★
	std::map<uint32_t, std::vector<uint32_t>> targetToHelpers;

	// ホストの情報を収集
	if (m_hostStateSet && m_hostState.meltTargetId != 0)
	{
		targetToHelpers[m_hostState.meltTargetId].push_back(1); // ホストのID=1
		NET_LOG_F("[ServerManager::ProcessMelting] ホスト[1] -> ターゲット[%u]", m_hostState.meltTargetId);
	}

	// 各クライアントの情報を収集
	for (auto& kv : m_clients)
	{
		if (kv.second->stateReceived && kv.second->lastState.meltTargetId != 0)
		{
			uint32_t helperId = kv.second->id;
			uint32_t targetId = kv.second->lastState.meltTargetId;
			targetToHelpers[targetId].push_back(helperId);
			NET_LOG_F("[ServerManager::ProcessMelting] クライアント[%u] -> ターゲット[%u]", helperId, targetId);
		}
	}

	// ★★★ 各ターゲットの解凍処理 ★★★
	bool stateChanged = false;
	const float deltaTime = 0.016f; // 約60FPS想定
	const float meltSpeed = 0.2f;   // Runner.cppのf_meltSpeedと同じ値

	for (auto& pair : targetToHelpers)
	{
		uint32_t targetId = pair.first;
		const auto& helpers = pair.second;

		NetPlayerState* targetState = nullptr;

		// ターゲットの状態を取得
		if (targetId == 1 && m_hostStateSet)
		{
			targetState = &m_hostState;
		}
		else
		{
			for (auto& kv : m_clients)
			{
				if (kv.second->id == targetId && kv.second->stateReceived)
				{
					targetState = &kv.second->lastState;
					break;
				}
			}
		}

		// ターゲットが存在しない、または凍結していない場合はスキップ
		if (!targetState || targetState->frozen == 0)
		{
			NET_LOG_F("[ServerManager::ProcessMelting] ターゲット[%u] は凍結していない", targetId);
			continue;
		}

		// ★★★ 解凍速度を計算（ヘルパーの数 × meltSpeed） ★★★
		float totalSpeed = (float)helpers.size() * meltSpeed;
		float oldAmount = targetState->frozenAmount;
		float newAmount = oldAmount + totalSpeed * deltaTime;

		NET_LOG_F("[ServerManager::ProcessMelting] ターゲット[%u] 解凍: %.3f -> %.3f (ヘルパー%d人)",
			targetId, oldAmount, newAmount, (int)helpers.size());

		if (newAmount > oldAmount)
		{
			if (newAmount >= 1.0f)
			{
				// 完全解凍
				targetState->frozenAmount = 1.0f;
				targetState->frozen = 0;
				NET_LOG_F("[ServerManager::ProcessMelting] ターゲット[%u] 完全解凍！", targetId);
			}
			else
			{
				// 解凍進行
				targetState->frozenAmount = newAmount;
			}

			stateChanged = true;
		}
	}

	if (stateChanged)
	{
		NET_LOG("[ServerManager::ProcessMelting] 解凍処理により状態が変化");
	}
}

std::vector<NetPlayerState> ServerManager::GetAllPlayerStates() const
{
	std::lock_guard<std::mutex> lk(m_stateMutex);
	std::vector<NetPlayerState> states;

	if (m_hostStateSet)
	{
		states.push_back(m_hostState);
	}

	for (const auto& kv : m_clients)
	{
		if (kv.second->stateReceived)
		{
			states.push_back(kv.second->lastState);
		}
	}

	return states;
}

void ServerManager::BroadcastWorldState()
{
	if (!m_pServerHost) return;

	// ★★★ 修正: 解凍処理をサーバー側で実行 ★★★
	ProcessMeltingOnServer();

	NetWorldState world;
	auto states = GetAllPlayerStates();

	world.playerCount = (uint8_t)min(16, (int)states.size());
	for (int i = 0; i < world.playerCount; ++i)
	{
		world.players[i] = states[i];
	}

	auto data = NetworkSerializer::SerializeWorldState(world);

	ENetPacket* packet = enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_UNSEQUENCED);
	enet_host_broadcast(m_pServerHost, 1, packet);
	enet_host_flush(m_pServerHost);
}

void ServerManager::BroadcastPlayerSpawn(const NetPlayerSpawn& spawn)
{
	if (!m_pServerHost) return;

	auto data = NetworkSerializer::SerializePlayerSpawn(spawn);
	ENetPacket* packet = enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE);
	enet_host_broadcast(m_pServerHost, 0, packet);
	enet_host_flush(m_pServerHost);
}

void ServerManager::BroadcastPlayerDespawn(uint32_t clientId)
{
	if (!m_pServerHost) return;

	auto data = NetworkSerializer::SerializePlayerDespawn(clientId);
	ENetPacket* packet = enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE);
	enet_host_broadcast(m_pServerHost, 0, packet);
	enet_host_flush(m_pServerHost);
}

void ServerManager::UpdatePlayerState(uint32_t clientId, const NetPlayerState& state)
{
	std::lock_guard<std::mutex> lk(m_stateMutex);

	for (auto& kv : m_clients)
	{
		if (kv.second->id == clientId)
		{
			kv.second->lastState = state;
			kv.second->stateReceived = true;
			return;
		}
	}
}

void ServerManager::SendToClient(ENetPeer* peer, const std::vector<uint8_t>& data)
{
	ENetPacket* packet = enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(peer, 0, packet);
}

void ServerManager::SendJoinAck(ENetPeer* peer, uint32_t clientId)
{
	std::vector<uint8_t> payload;
	payload.push_back((uint8_t)MSG_JOIN_ACK);
	const uint8_t* p = reinterpret_cast<const uint8_t*>(&clientId);
	payload.insert(payload.end(), p, p + sizeof(uint32_t));

	ENetPacket* packet = enet_packet_create(payload.data(), payload.size(), ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(peer, 0, packet);
	enet_host_flush(m_pServerHost);

	NET_LOG_F("[ServerManager] JoinAck送信: ClientID=%u", clientId);
}

void ServerManager::AssignRoles()
{
	NET_LOG("[ServerManager] 役割割り当て開始");

	// ★★★ 修正: 全プレイヤーIDを正しく収集 ★★★
	std::vector<uint32_t> allClientIds;

	// ホストがいる場合（m_hostNameが空でない = ホストが存在）
	if (!m_hostName.empty())
	{
		allClientIds.push_back(1); // ホストのIDは常に1
		NET_LOG("[ServerManager] ホストをリストに追加: ID=1");
	}

	// 他のクライアントを追加（ホスト以外）
	for (auto& kv : m_clients)
	{
		// ホストは既に追加済みなので、名前が異なるクライアントのみ追加
		if (kv.second->name != m_hostName)
		{
			allClientIds.push_back(kv.second->id);
			NET_LOG_F("[ServerManager] クライアントをリストに追加: ID=%u Name=%s",
				kv.second->id, kv.second->name.c_str());
		}
	}

	int totalPlayers = (int)allClientIds.size();
	NET_LOG_F("[ServerManager] 総プレイヤー数: %d", totalPlayers);

	if (totalPlayers == 0)
	{
		NET_LOG("[ServerManager] プレイヤーが0人のため割り当て中止");
		return;
	}

	// 鬼の数を計算（最低1人、最大でも総人数-1）
	int chaserCount = max(1, totalPlayers / 3);
	int runnerCount = totalPlayers - chaserCount;

	NET_LOG_F("[ServerManager] 総人数=%d 鬼=%d 逃げる側=%d",
		totalPlayers, chaserCount, runnerCount);

	// ランダムにシャッフル
	std::random_device rd;
	std::mt19937 gen(rd());
	std::shuffle(allClientIds.begin(), allClientIds.end(), gen);

	NET_LOG("[ServerManager] シャッフル後のID順:");
	for (size_t i = 0; i < allClientIds.size(); ++i)
	{
		NET_LOG_F("  位置%d: ID=%u", (int)i, allClientIds[i]);
	}

	// 最初のchaserCount人を鬼に、残りを逃げる側に
	for (size_t i = 0; i < allClientIds.size(); ++i)
	{
		uint32_t clientId = allClientIds[i];
		PlayerRole role = (i < (size_t)chaserCount) ? ROLE_CHASER : ROLE_RUNNER;

		// ホストの役割を設定
		if (clientId == 1 && !m_hostName.empty())
		{
			m_hostRole = role;
			NET_LOG_F("[ServerManager] ホスト(ID=1 Name=%s)の役割: %s",
				m_hostName.c_str(),
				(role == ROLE_CHASER) ? "鬼" : "逃げる側");
		}

		// クライアントの役割を設定（ホストも含む）
		for (auto& kv : m_clients)
		{
			if (kv.second->id == clientId)
			{
				kv.second->role = role;
				NET_LOG_F("[ServerManager] クライアント %s (ID=%u) の役割: %s",
					kv.second->name.c_str(), clientId,
					(role == ROLE_CHASER) ? "鬼" : "逃げる側");
				break;
			}
		}
	}

	NET_LOG("[ServerManager] 役割割り当て完了");

	// ★★★ デバッグ: 割り当て結果を確認 ★★★
	int actualChasers = 0;
	int actualRunners = 0;

	if (!m_hostName.empty())
	{
		if (m_hostRole == ROLE_CHASER) actualChasers++;
		else actualRunners++;
	}

	for (auto& kv : m_clients)
	{
		if (kv.second->name == m_hostName) continue; // ホストは既にカウント済み
		if (kv.second->role == ROLE_CHASER) actualChasers++;
		else actualRunners++;
	}

	NET_LOG_F("[ServerManager] 割り当て結果確認: 鬼=%d 逃げる側=%d",
		actualChasers, actualRunners);

	// ★★★ 鬼が0人の場合は警告 ★★★
	if (actualChasers == 0)
	{
		NET_LOG("[ServerManager] 警告: 鬼が0人です！");
	}
}

// ★ 役割をクライアントにブロードキャスト
void ServerManager::BroadcastRoleAssignments()
{
	if (!m_pServerHost) return;

	NET_LOG("[ServerManager] 役割割り当てをブロードキャスト");

	// ホストの役割を送信
	if (!m_hostName.empty())
	{
		NetRoleAssignment assignment;
		assignment.clientId = 1;
		assignment.role = m_hostRole;

		auto data = NetworkSerializer::SerializeRoleAssignment(assignment);
		ENetPacket* packet = enet_packet_create(data.data(), data.size(),
			ENET_PACKET_FLAG_RELIABLE);
		enet_host_broadcast(m_pServerHost, 0, packet);

		NET_LOG_F("[ServerManager] ホスト(ID=1)の役割送信: %s",
			(m_hostRole == ROLE_CHASER) ? "鬼" : "逃げる側");
	}

	// 各クライアントの役割を送信
	for (auto& kv : m_clients)
	{
		NetRoleAssignment assignment;
		assignment.clientId = kv.second->id;
		assignment.role = kv.second->role;

		auto data = NetworkSerializer::SerializeRoleAssignment(assignment);
		ENetPacket* packet = enet_packet_create(data.data(), data.size(),
			ENET_PACKET_FLAG_RELIABLE);
		enet_host_broadcast(m_pServerHost, 0, packet);

		NET_LOG_F("[ServerManager] クライアント %s (ID=%u)の役割送信: %s",
			kv.second->name.c_str(), kv.second->id,
			(kv.second->role == ROLE_CHASER) ? "鬼" : "逃げる側");
	}

	enet_host_flush(m_pServerHost);
	NET_LOG("[ServerManager] 役割割り当てブロードキャスト完了");
}

void ServerManager::StartGame()
{
	if (!m_pServerHost) return;

	int totalPlayers = m_clientCount;
	if (!m_hostName.empty()) totalPlayers++;

	if (totalPlayers < 2)
	{
		std::cout << "[Server] プレイヤーが足りません。開始できません。\n";
		return;
	}

	// ★ 役割を割り当て
	AssignRoles();

	// ★★★ 修正: 役割を最優先で送信（複数回送信で確実性向上） ★★★
	for (int retry = 0; retry < 3; retry++)  // 3回送信
	{
		BroadcastRoleAssignments();

		if (retry < 2)
		{
			Sleep(50);  // 50msの間隔を空けて再送
		}
	}

	NET_LOG("[ServerManager] 役割割り当て送信完了（3回送信）");

	// ★★★ 100ms待機してから状態を更新 ★★★
	Sleep(100);

	if (m_advertiser) m_advertiser->SetAdvertiseState(1);

	// ゲーム開始通知
	std::vector<uint8_t> payload;
	payload.push_back((uint8_t)MSG_START_GAME);
	ENetPacket* packet = enet_packet_create(payload.data(), payload.size(),
		ENET_PACKET_FLAG_RELIABLE);
	enet_host_broadcast(m_pServerHost, 0, packet);
	enet_host_flush(m_pServerHost);

	std::cout << "[Server] ゲーム開始通知を送信しました\n";
	NET_LOG("[ServerManager] ゲーム開始 - 役割割り当て済み");
}