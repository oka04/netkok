#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include "ClientManager.h"
#include <windows.h>
#include <iostream>
#include <algorithm>
#include "..\\NetworkLogger.h"

#pragma warning(disable:4996)
#pragma warning(disable:26812)
#pragma warning(disable:26495)
#pragma warning(disable:6387)

#pragma comment(lib, "enetlib.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")

ClientManager* ClientManager::s_instance = nullptr;

ClientManager* ClientManager::GetInstance()
{
	if (!s_instance) s_instance = new ClientManager();
	return s_instance;
}

ClientManager::ClientManager()
	: m_pClientHost(nullptr)
	, m_pServerPeer(nullptr)
	, m_pDiscovery(nullptr)
	, m_bGameStarted(false)
	, m_bHost(false)
	, m_previousLobbyCount(0)
	, m_bConnected(false)
	, m_lastHeartbeatTime(0)
	, m_worldStateReceived(false)
	, m_assignedClientId(0)
	, m_myRole(ROLE_NONE)
{
	NetworkLogger::GetInstance().Initialize("network_debug.txt");
	NET_LOG("========================================");
	NET_LOG("ClientManager 初期化開始");
	NET_LOG("========================================");

	if (enet_initialize() != 0)
	{
		NET_LOG("[ClientManager] ENetの初期化に失敗しました");
		MessageBoxA(NULL, "ENetの初期化に失敗しました。", "エラー", MB_OK);
	}
	else
	{
		NET_LOG("[ClientManager] ENet初期化成功");
	}

	m_pDiscovery = std::make_unique<Discovery>();
	NET_LOG("[ClientManager] Discovery作成");

	m_pDiscovery->StartListener(12346);
	NET_LOG("[ClientManager] Discoveryリスナー起動 (ポート:12346)");
}

ClientManager::~ClientManager()
{
	if (m_pDiscovery)
	{
		m_pDiscovery->StopListener();
		m_pDiscovery.reset();
	}

	Disconnect();
	enet_deinitialize();

	if (s_instance == this) s_instance = nullptr;
}

void ClientManager::Disconnect()
{
	if (m_pServerPeer)
	{
		enet_peer_disconnect(m_pServerPeer, 0);
		m_pServerPeer = nullptr;
	}

	if (m_pClientHost)
	{
		enet_host_destroy(m_pClientHost);
		m_pClientHost = nullptr;
	}

	m_bConnected = false;
	NET_LOG("[Client] 切断");
}

void ClientManager::Reset()
{
	NET_LOG("[ClientManager] Reset開始");

	if (m_pServerPeer)
	{
		enet_peer_disconnect(m_pServerPeer, 0);

		if (m_pClientHost)
		{
			ENetEvent event;
			int timeout = 30;
			while (timeout > 0 && enet_host_service(m_pClientHost, &event, 100) > 0)
			{
				if (event.type == ENET_EVENT_TYPE_DISCONNECT)
				{
					NET_LOG("[ClientManager] 切断完了");
					break;
				}
				timeout--;
			}
		}

		m_pServerPeer = nullptr;
	}

	if (m_pClientHost)
	{
		enet_host_destroy(m_pClientHost);
		m_pClientHost = nullptr;
	}

	{
		std::lock_guard<std::mutex> lk(m_lobbyMutex);
		m_lobbyPlayerNames.clear();
	}

	m_bGameStarted = false;
	m_bHost = false;
	m_bConnected = false;
	m_serverName = "";
	m_availableServers.clear();
	m_cachedServers.clear();
	m_allServers.clear();
	m_previousLobbyCount = 0;
	m_lastHeartbeatTime = 0;

	m_assignedClientId = 0;
	m_myRole = ROLE_NONE;
	m_roleMap.clear();
	{
		std::lock_guard<std::mutex> lk(m_worldMutex);
		m_worldStateReceived = false;
		while (!m_spawnQueue.empty()) m_spawnQueue.pop();
		while (!m_despawnQueue.empty()) m_despawnQueue.pop();
		while (!m_roleQueue.empty()) m_roleQueue.pop();
		while (!m_resultQueue.empty()) m_resultQueue.pop();
	}
	m_assignedClientId = 0;

	NET_LOG("[ClientManager] Reset完了");
}
void ClientManager::ResetForLobbyReturn()
{
	NET_LOG("[ClientManager] ResetForLobbyReturn開始");

	m_bGameStarted = false;

	m_myRole = ROLE_NONE;
	m_roleMap.clear();

	{
		std::lock_guard<std::mutex> lk(m_worldMutex);
		m_worldStateReceived = false;
		while (!m_spawnQueue.empty()) m_spawnQueue.pop();
		while (!m_despawnQueue.empty()) m_despawnQueue.pop();
		while (!m_roleQueue.empty()) m_roleQueue.pop();
		while (!m_resultQueue.empty()) m_resultQueue.pop();
	}

	NET_LOG_F("[ClientManager] ResetForLobbyReturn完了: m_bGameStarted=%s ClientID=%u",
		m_bGameStarted ? "true" : "false", m_assignedClientId);
}
void ClientManager::SendMessage(const char* msg)
{
	if (!m_pServerPeer || !m_bConnected) return;
	ENetPacket* packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(m_pServerPeer, 0, packet);
	enet_host_flush(m_pClientHost);
}

void ClientManager::SendJoin(const std::string& name)
{
	if (!m_pServerPeer || !m_pClientHost) return;
	std::vector<uint8_t> buf;
	buf.push_back((uint8_t)MSG_JOIN);
	uint8_t nl = (uint8_t)std::min<size_t>(255, name.size());
	buf.push_back(nl);
	buf.insert(buf.end(), name.begin(), name.begin() + nl);
	ENetPacket* packet = enet_packet_create(buf.data(), (size_t)buf.size(), ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(m_pServerPeer, 0, packet);
	enet_host_flush(m_pClientHost);
	NET_LOG_F("[ClientManager] JOIN送信: %s", name.c_str());
}

std::vector<std::string> ClientManager::GetLobbyPlayerNames()
{
	std::lock_guard<std::mutex> lk(m_lobbyMutex);
	return m_lobbyPlayerNames;
}

const std::vector<ServerInfoNet>& ClientManager::GetCachedServers() const
{
	return m_cachedServers;
}

const std::vector<ServerInfoNet>& ClientManager::GetAllServers() const
{
	return m_allServers;
}

const std::string& ClientManager::GetServerName() const
{
	return m_serverName;
}

bool ClientManager::IsGameStarted() const
{
	return m_bGameStarted;
}

bool ClientManager::IsHost() const
{
	return m_bHost;
}

bool ClientManager::IsConnected() const
{
	return m_bConnected && m_pServerPeer != nullptr &&
		m_pServerPeer->state == ENET_PEER_STATE_CONNECTED;
}

void ClientManager::SetPlayerName(const std::string& name)
{
	m_playerName = name;
}

const std::string& ClientManager::GetPlayerName() const
{
	return m_playerName;
}

void ClientManager::SetServerName(const std::string& name)
{
	m_serverName = name;
	NET_LOG_F("[ClientManager] サーバー名を設定: %s", name.c_str());
}

bool ClientManager::ConnectToServer(const std::string& ip, int port)
{
	NET_LOG_F("[ClientManager] ConnectToServer: %s:%d", ip.c_str(), port);

	if (m_pServerPeer)
	{
		enet_peer_reset(m_pServerPeer);
		m_pServerPeer = nullptr;
	}

	if (m_pClientHost)
	{
		enet_host_destroy(m_pClientHost);
		m_pClientHost = nullptr;
	}

	m_bGameStarted = false;
	m_bConnected = false;
	m_lastHeartbeatTime = 0;
	m_worldStateReceived = false;
	m_assignedClientId = 0;
	{
		std::lock_guard<std::mutex> lk(m_lobbyMutex);
		m_lobbyPlayerNames.clear();
	}

	m_pClientHost = enet_host_create(nullptr, 1, 2, 0, 0);
	if (!m_pClientHost)
	{
		NET_LOG("[ClientManager] クライアント作成失敗");
		return false;
	}
	NET_LOG("[ClientManager] クライアントホスト作成成功");

	ENetAddress address;
	enet_address_set_host(&address, ip.c_str());
	address.port = (enet_uint16)port;

	m_pServerPeer = enet_host_connect(m_pClientHost, &address, 2, 0);
	if (!m_pServerPeer)
	{
		NET_LOG("[ClientManager] サーバー接続要求失敗");
		enet_host_destroy(m_pClientHost);
		m_pClientHost = nullptr;
		return false;
	}

	m_bHost = (ip == "127.0.0.1" || ip == "localhost");

	if (m_bHost)
	{
		m_serverName = "接続中...";
		m_assignedClientId = 1;
		NET_LOG("[ClientManager] ホスト接続 - サーバー名は接続後に取得");
	}
	else
	{
		m_serverName = "Unknown Server";
		for (const auto& server : m_allServers)
		{
			char serverIp[64];
			ENetAddress addr;
			addr.host = server.ip;
			addr.port = server.port;
			enet_address_get_host_ip(&addr, serverIp, sizeof(serverIp));

			if (std::string(serverIp) == ip && server.port == port)
			{
				m_serverName = server.name;
				NET_LOG_F("[ClientManager] サーバー名をDiscoveryから取得: %s", m_serverName.c_str());
				break;
			}
		}
	}

	NET_LOG_F("[ClientManager] 接続要求送信 (ホスト判定: %s)", m_bHost ? "true" : "false");
	return true;
}

void ClientManager::RefreshAvailableServers()
{
	NET_LOG("[ClientManager] RefreshAvailableServers 開始");

	m_availableServers.clear();
	m_allServers.clear();

	if (!m_pDiscovery)
	{
		NET_LOG("[ClientManager] エラー: m_pDiscovery が nullptr");
		return;
	}

	auto servers = m_pDiscovery->GetServers();
	NET_LOG_F("[ClientManager] Discovery から %d サーバー取得", (int)servers.size());

	int addedCount = 0;
	int skippedCount = 0;

	for (auto& s : servers)
	{
		char ipStr[INET_ADDRSTRLEN];
		struct in_addr addr;
		addr.s_addr = s.ip;
		inet_ntop(AF_INET, &addr, ipStr, sizeof(ipStr));

		NET_LOG_F("[ClientManager] サーバー情報: %s @ %s:%d (%d/%d) state=%d",
			s.name.c_str(), ipStr, s.port, (int)s.playerCount, (int)s.maxPlayers, (int)s.state);

		ServerInfoNet n;
		n.ip = s.ip;
		n.port = s.port;
		n.playerCount = s.playerCount;
		n.maxPlayers = s.maxPlayers;
		n.state = s.state;
		n.name = s.name;

		m_allServers.push_back(n);

		if (s.state != 0)
		{
			NET_LOG_F("[ClientManager] スキップ: ゲーム中 (state=%d)", (int)s.state);
			skippedCount++;
			continue;
		}

		m_availableServers.push_back(n);
		addedCount++;

		NET_LOG_F("[ClientManager] 追加成功: %s", s.name.c_str());
	}

	m_cachedServers = m_availableServers;

	NET_LOG_F("[ClientManager] RefreshAvailableServers 完了: 追加=%d スキップ=%d 全体=%d",
		addedCount, skippedCount, (int)m_allServers.size());
}

void ClientManager::Update()
{
	if (!m_pClientHost) return;

	if (m_bConnected && m_pServerPeer)
	{
		DWORD now = timeGetTime();
		if (now - m_lastHeartbeatTime > 5000)
		{
			if (m_pServerPeer->state != ENET_PEER_STATE_CONNECTED)
			{
				NET_LOG("[ClientManager] サーバー接続が切断されました（タイムアウト）");
				m_bConnected = false;
			}
		}
	}

	ENetEvent event;
	while (enet_host_service(m_pClientHost, &event, 0) > 0)
	{
		switch (event.type)
		{
		case ENET_EVENT_TYPE_CONNECT:
			OnConnect();
			break;

		case ENET_EVENT_TYPE_RECEIVE:
			OnReceive(event);
			break;

		case ENET_EVENT_TYPE_DISCONNECT:
			OnDisconnect();
			break;
		}
	}
}

void ClientManager::OnConnect()
{
	NET_LOG("[ClientManager] サーバー接続成功（ENET_EVENT_TYPE_CONNECT受信）");
	m_bConnected = true;
	m_lastHeartbeatTime = timeGetTime();

	std::string nameToSend = m_playerName.empty() ? "Player" : m_playerName;
	SendJoin(nameToSend);

	NET_LOG_F("[ClientManager] JOIN送信完了: %s", nameToSend.c_str());
}

void ClientManager::OnReceive(const ENetEvent& event)
{
	m_lastHeartbeatTime = timeGetTime();

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
	case MSG_SERVER_INFO:
		ProcessServerInfo(data, len);
		break;

	case MSG_LOBBY_UPDATE:
		ProcessLobbyUpdate(data, len);
		break;

	case MSG_START_GAME:
		m_bGameStarted = true;
		NET_LOG("[ClientManager] ゲーム開始通知受信");
		break;

	case MSG_WORLD_STATE:
		ProcessWorldState(data + 1, len - 1);
		break;

	case MSG_PLAYER_SPAWN:
		ProcessPlayerSpawn(data + 1, len - 1);
		break;

	case MSG_PLAYER_DESPAWN:
		ProcessPlayerDespawn(data + 1, len - 1);
		break;

	case MSG_JOIN_ACK:
		ProcessJoinAck(data + 1, len - 1);
		break;

	case MSG_ROLE_ASSIGNMENT:
		ProcessRoleAssignment(data + 1, len - 1);
		break;

	case MSG_GAME_RESULT:  
		ProcessGameResult(data + 1, len - 1);
		break;
	}

	enet_packet_destroy(event.packet);
}

void ClientManager::OnDisconnect()
{
	NET_LOG("[ClientManager] サーバーから切断されました");
	m_pServerPeer = nullptr;
	m_bConnected = false;
}

void ClientManager::ProcessGameResult(const uint8_t* data, size_t len)
{
	NetGameResult result;
	if (!NetworkSerializer::DeserializeGameResult(data, len, result))
		return;

	std::lock_guard<std::mutex> lk(m_worldMutex);
	m_resultQueue.push(result);

	NET_LOG_F("[ClientManager] ゲーム結果受信: winner=%d (%s)",
		(int)result.winnerTeam,
		(result.winnerTeam == 0) ? "逃げる側" : "鬼側");
}

bool ClientManager::PopGameResult(NetGameResult& out)
{
	std::lock_guard<std::mutex> lk(m_worldMutex);
	if (m_resultQueue.empty()) return false;
	out = m_resultQueue.front();
	m_resultQueue.pop();
	return true;
}

void ClientManager::ProcessServerInfo(const uint8_t* data, size_t len)
{
	NET_LOG_F("[ClientManager] ProcessServerInfo: データ長=%d", (int)len);

	size_t idx = 1;
	if (idx >= len)
	{
		NET_LOG("[ClientManager] エラー: データ長不足");
		return;
	}

	uint8_t nameLen = data[idx++];
	NET_LOG_F("[ClientManager] サーバー名長さ: %d", (int)nameLen);

	if (idx + nameLen > len)
	{
		NET_LOG("[ClientManager] エラー: サーバー名データ不足");
		return;
	}

	std::string serverName(reinterpret_cast<const char*>(data + idx), nameLen);
	m_serverName = serverName;

	m_bGameStarted = false;

	NET_LOG_F("[ClientManager] ★サーバー情報受信★: サーバー名='%s' GameStarted=%s",
		m_serverName.c_str(), m_bGameStarted ? "true" : "false");
}

void ClientManager::ProcessLobbyUpdate(const uint8_t* data, size_t len)
{
	NET_LOG_F("[ClientManager] ProcessLobbyUpdate: データ長=%d", (int)len);

	size_t idx = 1;
	if (idx >= len)
	{
		NET_LOG("[ClientManager] エラー: データ長不足");
		return;
	}

	uint8_t count = data[idx++];
	NET_LOG_F("[ClientManager] プレイヤー数: %d", (int)count);

	std::vector<std::string> newNames;
	newNames.reserve(count);

	for (int i = 0; i < count && idx < len; ++i)
	{
		if (idx >= len)
		{
			NET_LOG_F("[ClientManager] エラー: プレイヤー%d の名前長さ読み取り不可", i + 1);
			break;
		}

		uint8_t nl = data[idx++];
		NET_LOG_F("[ClientManager] プレイヤー%d 名前長さ: %d", i + 1, (int)nl);

		if (idx + nl > len)
		{
			NET_LOG_F("[ClientManager] エラー: プレイヤー%d の名前データ不足", i + 1);
			break;
		}

		std::string playerName(reinterpret_cast<const char*>(data + idx), nl);
		newNames.emplace_back(playerName);
		NET_LOG_F("[ClientManager] プレイヤー%d: '%s'", i + 1, playerName.c_str());
		idx += nl;
	}

	std::lock_guard<std::mutex> lk(m_lobbyMutex);
	m_lobbyPlayerNames = std::move(newNames);

	NET_LOG_F("[ClientManager] ロビー更新完了: %d 人", (int)m_lobbyPlayerNames.size());
}

void ClientManager::SendPlayerState(const NetPlayerState& state)
{
	if (!m_pServerPeer || !m_bConnected)
	{
		NET_LOG("[ClientManager] サーバーに未接続 - 状態送信スキップ");
		return;
	}

	auto data = NetworkSerializer::SerializePlayerState(state);

	static DWORD lastLogTime = 0;
	DWORD now = timeGetTime();
	if (now - lastLogTime > 1000) 
	{
		NET_LOG_F("[ClientManager] 状態送信: ID=%u Pos=(%.1f,%.1f,%.1f) Size=%d bytes",
			state.clientId, state.posX, state.posY, state.posZ, (int)data.size());
		lastLogTime = now;
	}

	ENetPacket* packet = enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_UNSEQUENCED);
	enet_peer_send(m_pServerPeer, 1, packet);

	enet_host_flush(m_pClientHost);
}

bool ClientManager::GetWorldState(NetWorldState& out)
{
	std::lock_guard<std::mutex> lk(m_worldMutex);
	if (!m_worldStateReceived) return false;
	out = m_worldState;
	m_worldStateReceived = false;
	return true;
}

bool ClientManager::PopPlayerSpawn(NetPlayerSpawn& out)
{
	std::lock_guard<std::mutex> lk(m_worldMutex);
	if (m_spawnQueue.empty()) return false;
	out = m_spawnQueue.front();
	m_spawnQueue.pop();
	return true;
}

bool ClientManager::PopPlayerDespawn(uint32_t& out)
{
	std::lock_guard<std::mutex> lk(m_worldMutex);
	if (m_despawnQueue.empty()) return false;
	out = m_despawnQueue.front();
	m_despawnQueue.pop();
	return true;
}

void ClientManager::ProcessWorldState(const uint8_t* data, size_t len)
{
	std::lock_guard<std::mutex> lk(m_worldMutex);

	if (NetworkSerializer::DeserializeWorldState(data, len, m_worldState))
	{
		m_worldStateReceived = true;
	}
}

void ClientManager::ProcessPlayerSpawn(const uint8_t* data, size_t len)
{
	if (len < sizeof(NetPlayerSpawn)) return;

	NetPlayerSpawn spawn;
	std::memcpy(&spawn, data, sizeof(NetPlayerSpawn));

	std::lock_guard<std::mutex> lk(m_worldMutex);
	m_spawnQueue.push(spawn);

	NET_LOG_F("[ClientManager] プレイヤー生成通知: ID=%u, Name=%s", spawn.clientId, spawn.name);
}

void ClientManager::ProcessPlayerDespawn(const uint8_t* data, size_t len)
{
	if (len < sizeof(uint32_t)) return;

	uint32_t clientId;
	std::memcpy(&clientId, data, sizeof(uint32_t));

	std::lock_guard<std::mutex> lk(m_worldMutex);
	m_despawnQueue.push(clientId);

	NET_LOG_F("[ClientManager] プレイヤー削除通知: ID=%u", clientId);
}

void ClientManager::ProcessJoinAck(const uint8_t* data, size_t len)
{
	if (len < sizeof(uint32_t)) return;

	std::memcpy(&m_assignedClientId, data, sizeof(uint32_t));
	NET_LOG_F("[ClientManager] クライアントID割り当て: %u", m_assignedClientId);
}

void ClientManager::ProcessRoleAssignment(const uint8_t* data, size_t len)
{
	if (len < sizeof(NetRoleAssignment)) return;

	NetRoleAssignment assignment;
	if (!NetworkSerializer::DeserializeRoleAssignment(data, len, assignment))
		return;

	std::lock_guard<std::mutex> lk(m_worldMutex);

	//キューに追加
	m_roleQueue.push(assignment);

	//役割マップに登録
	m_roleMap[assignment.clientId] = assignment.role;

	//自分の役割なら記録
	if (assignment.clientId == m_assignedClientId)
	{
		m_myRole = assignment.role;
		NET_LOG_F("[ClientManager] 自分の役割が決定: %s (ID=%u)",
			(m_myRole == ROLE_CHASER) ? "鬼" : "逃げる側", m_assignedClientId);
	}
	else
	{
		NET_LOG_F("[ClientManager] 他プレイヤーの役割: ID=%u %s",
			assignment.clientId,
			(assignment.role == ROLE_CHASER) ? "鬼" : "逃げる側");
	}
}

bool ClientManager::PopRoleAssignment(NetRoleAssignment& out)
{
	std::lock_guard<std::mutex> lk(m_worldMutex);
	if (m_roleQueue.empty()) return false;
	out = m_roleQueue.front();
	m_roleQueue.pop();
	return true;
}
