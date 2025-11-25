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

	if (m_advertiser) m_advertiser->SetAdvertiseState(1);

	std::vector<uint8_t> payload;
	payload.push_back((uint8_t)MSG_START_GAME);
	ENetPacket* packet = enet_packet_create(payload.data(), payload.size(), ENET_PACKET_FLAG_RELIABLE);
	enet_host_broadcast(m_pServerHost, 0, packet);
	enet_host_flush(m_pServerHost);

	std::cout << "[Server] ゲーム開始通知を送信しました\n";
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
		NET_LOG_F("[ServerManager] クライアント切断: %s (ID=%u)", it->second->name.c_str(), clientId);

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

			// ★★★ 既存の全プレイヤー情報を新規参加者に送信 ★★★
			// 1. ホストの情報を送信
			if (!m_hostName.empty())
			{
				NetPlayerSpawn hostSpawn;
				hostSpawn.clientId = 1;  // ホストのIDは常に1

										 // ★★★ ホストの現在位置を取得（ワールド状態から）★★★
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

			// 2. 既存の他のクライアント情報を送信
			for (const auto& kv : m_clients)
			{
				if (kv.first != peer && kv.second->name != m_hostName && !kv.second->name.empty())
				{
					NetPlayerSpawn existingSpawn;
					existingSpawn.clientId = kv.second->id;

					// ★★★ 既存クライアントの現在位置を取得 ★★★
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

			// ★★★ 新規参加者の情報を全員にブロードキャスト ★★★
			NetPlayerSpawn newSpawn;
			newSpawn.clientId = ci->id;
			newSpawn.startX = 0.0f;  // 新規参加者は初期位置
			newSpawn.startY = 0.0f;
			newSpawn.startZ = 0.0f;
			strncpy_s(newSpawn.name, ci->name.c_str(), sizeof(newSpawn.name) - 1);
			newSpawn.name[sizeof(newSpawn.name) - 1] = '\0';

			BroadcastPlayerSpawn(newSpawn);
			NET_LOG_F("[ServerManager] 新規参加者情報をブロードキャスト: %s (ID=%u)",
				ci->name.c_str(), ci->id);

			// フラッシュして確実に送信
			enet_host_flush(m_pServerHost);

			// ロビー更新をブロードキャスト
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
	if (len < sizeof(NetPlayerState))
	{
		NET_LOG_F("[ServerManager] プレイヤー状態データ不足: %d bytes", (int)len);
		return;
	}

	NetPlayerState state;
	if (!NetworkSerializer::DeserializePlayerState(data, len, state))
	{
		NET_LOG("[ServerManager] プレイヤー状態のデシリアライズ失敗");
		return;
	}

	std::lock_guard<std::mutex> lk(m_stateMutex);

	auto it = m_clients.find(peer);
	if (it != m_clients.end())
	{
		// ★★★ 受信した状態を保存 ★★★
		it->second->lastState = state;
		it->second->lastState.clientId = it->second->id; // IDを確実に設定
		it->second->stateReceived = true;

		// ★★★ デバッグ: 受信した状態をログ出力 ★★★
		static DWORD lastLogTime = 0;
		DWORD now = timeGetTime();
		if (now - lastLogTime > 1000) // 1秒ごとにログ
		{
			NET_LOG_F("[ServerManager] クライアント状態受信: ID=%u Pos=(%.1f,%.1f,%.1f)",
				it->second->id, state.posX, state.posY, state.posZ);
			lastLogTime = now;
		}
	}
	else
	{
		NET_LOG("[ServerManager] 不明なクライアントから状態を受信");
	}
}

void ServerManager::SetHostState(const NetPlayerState& state)
{
	std::lock_guard<std::mutex> lk(m_stateMutex);
	m_hostState = state;
	m_hostState.clientId = 1;
	m_hostStateSet = true;
}

std::vector<NetPlayerState> ServerManager::GetAllPlayerStates() const
{
	std::lock_guard<std::mutex> lk(m_stateMutex);
	std::vector<NetPlayerState> states;

	if (m_hostStateSet)
	{
		states.push_back(m_hostState);
		NET_LOG_F("[ServerManager] ホスト状態取得: ID=%u Pos=(%.1f,%.1f,%.1f)",
			m_hostState.clientId, m_hostState.posX, m_hostState.posY, m_hostState.posZ);
	}

	for (const auto& kv : m_clients)
	{
		if (kv.second->stateReceived)
		{
			states.push_back(kv.second->lastState);
			NET_LOG_F("[ServerManager] クライアント状態取得: ID=%u Pos=(%.1f,%.1f,%.1f)",
				kv.second->lastState.clientId,
				kv.second->lastState.posX, kv.second->lastState.posY, kv.second->lastState.posZ);
		}
	}

	return states;
}
void ServerManager::BroadcastWorldState()
{
	if (!m_pServerHost) return;

	NetWorldState world;
	auto states = GetAllPlayerStates();

	world.playerCount = (uint8_t)min(8, (int)states.size());
	for (int i = 0; i < world.playerCount; ++i)
	{
		world.players[i] = states[i];
	}

	// ★★★ デバッグ: ブロードキャストする内容をログ出力 ★★★
	static DWORD lastLogTime = 0;
	DWORD now = timeGetTime();
	if (now - lastLogTime > 1000) // 1秒ごとにログ
	{
		NET_LOG_F("[ServerManager] ワールド状態ブロードキャスト: プレイヤー数=%d", (int)world.playerCount);
		for (int i = 0; i < world.playerCount; ++i)
		{
			NET_LOG_F("  [%d] ID=%u Pos=(%.1f,%.1f,%.1f)",
				i, world.players[i].clientId,
				world.players[i].posX, world.players[i].posY, world.players[i].posZ);
		}
		lastLogTime = now;
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