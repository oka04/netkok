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

enum {
	MSG_JOIN = 1,
	MSG_JOIN_ACK = 2,
	MSG_LOBBY_UPDATE = 3,
	MSG_START_GAME = 4,
};

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
{
	// ネットワークログ初期化（ファイルを空にする）
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
	if (m_pDiscovery) {
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

	std::cout << "[Client] 切断" << std::endl;
}

void ClientManager::SendMessage(const char* msg)
{
	if (!m_pServerPeer) return;
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

const std::string & ClientManager::GetServerName() const
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
void ClientManager::SetPlayerName(const std::string& name)
{
	m_playerName = name;
}

const std::string & ClientManager::GetPlayerName() const
{
	return m_playerName;
}

bool ClientManager::ConnectToServer(const std::string& ip, int port)
{
	NET_LOG_F("[ClientManager] ConnectToHost: %s:%d", ip.c_str(), port);

	if (m_pClientHost)
	{
		enet_host_destroy(m_pClientHost);
		m_pClientHost = nullptr;
	}

	m_pClientHost = enet_host_create(nullptr, 1, 1, 0, 0);
	if (!m_pClientHost)
	{
		NET_LOG("[ClientManager] クライアント作成失敗");
		return false;
	}
	NET_LOG("[ClientManager] クライアントホスト作成成功");

	ENetAddress address;
	enet_address_set_host(&address, ip.c_str());
	address.port = (enet_uint16)port;

	m_pServerPeer = enet_host_connect(m_pClientHost, &address, 1, 0);
	if (!m_pServerPeer)
	{
		NET_LOG("[ClientManager] サーバー接続要求失敗");
		return false;
	}

	m_bHost = (ip == "127.0.0.1" || ip == "localhost");

	// ★★★ サーバー名を保存 ★★★
	// GetAllServers()から該当するサーバーを探す
	for (const auto& server : m_allServers)
	{
		char serverIp[64];
		struct in_addr addr;
		addr.s_addr = server.ip;
		inet_ntop(AF_INET, &addr, serverIp, sizeof(serverIp));

		if (std::string(serverIp) == ip && server.port == port)
		{
			m_serverName = server.name;
			NET_LOG_F("[ClientManager] サーバー名を設定: %s", m_serverName.c_str());
			break;
		}
	}

	NET_LOG_F("[ClientManager] 接続要求送信中... (ホスト判定: %s)", m_bHost ? "true" : "false");

	return true;
}

void ClientManager::RefreshAvailableServers()
{
	NET_LOG("[ClientManager] RefreshAvailableServers 開始");

	m_availableServers.clear();
	m_allServers.clear(); // 全サーバーリストもクリア

	if (!m_pDiscovery) {
		NET_LOG("[ClientManager] エラー: m_pDiscovery が nullptr");
		return;
	}

	auto servers = m_pDiscovery->GetServers();
	NET_LOG_F("[ClientManager] Discovery から %d サーバー取得", (int)servers.size());

	int addedCount = 0;
	int skippedCount = 0;

	for (auto &s : servers) {
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

		// 全サーバーリストには必ず追加
		m_allServers.push_back(n);

		// 待機中のサーバーのみ m_availableServers に追加
		if (s.state != 0) {
			NET_LOG_F("[ClientManager] スキップ: ゲーム中 (state=%d)", (int)s.state);
			skippedCount++;
			continue;
		}

		m_availableServers.push_back(n);
		addedCount++;

		NET_LOG_F("[ClientManager] 追加成功: %s", s.name.c_str());
	}

	m_cachedServers = m_availableServers;

	NET_LOG_F("[ClientManager] RefreshAvailableServers 完了: 追加=%d スキップ=%d 合計=%d 全体=%d",
		addedCount, skippedCount, (int)m_cachedServers.size(), (int)m_allServers.size());
}

bool ClientManager::IsConnected() const
{
	return (m_pServerPeer != nullptr && m_pServerPeer->state == ENET_PEER_STATE_CONNECTED);
}

// 新規メソッド：全サーバー（待機中 + ゲーム中）を取得
const std::vector<ServerInfoNet>& ClientManager::GetAllServers() const
{
	return m_allServers;
}

void ClientManager::Update()
{
	if (!m_pClientHost) return;

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
	NET_LOG("[ClientManager] サーバー接続成功");
	std::string nameToSend = m_playerName.empty() ? "Player" : m_playerName;
	SendJoin(nameToSend);
	NET_LOG_F("[ClientManager] JOIN送信: %s", nameToSend.c_str());
}

void ClientManager::OnReceive(const ENetEvent& event)
{
	const uint8_t* data = (const uint8_t*)event.packet->data;
	size_t len = event.packet->dataLength;
	if (len < 1) {
		enet_packet_destroy(event.packet);
		return;
	}

	uint8_t id = data[0];
	switch (id)
	{
	case MSG_LOBBY_UPDATE:
		ProcessLobbyUpdate(data, len);
		break;

	case MSG_START_GAME:
		m_bGameStarted = true;
		break;
	}

	enet_packet_destroy(event.packet);
}

void ClientManager::OnDisconnect()
{
	NET_LOG("[ClientManager] サーバーから切断されました");
	m_pServerPeer = nullptr;  // ★★★ ピアをnullにする ★★★
}

void ClientManager::ProcessLobbyUpdate(const uint8_t* data, size_t len)
{
	NET_LOG_F("[ClientManager] ProcessLobbyUpdate: データ長=%d", (int)len);

	// データの16進数ダンプ（デバッグ用）
	std::string hexDump;
	size_t dumpLen = (len < 32) ? len : 32;
	for (size_t i = 0; i < dumpLen; i++)
	{
		char buf[8];
		sprintf(buf, "%02X ", data[i]);
		hexDump += buf;
	}
	NET_LOG_F("[ClientManager] データダンプ: %s", hexDump.c_str());

	size_t idx = 1;
	if (idx >= len) {
		NET_LOG("[ClientManager] エラー: データ長不足（count読み取り不可）");
		return;
	}

	uint8_t count = data[idx++];
	NET_LOG_F("[ClientManager] プレイヤー数: %d", (int)count);

	std::vector<std::string> newNames;
	newNames.reserve(count);

	for (int i = 0; i < count && idx < len; ++i)
	{
		if (idx >= len) {
			NET_LOG_F("[ClientManager] エラー: プレイヤー%d の名前長さ読み取り不可", i + 1);
			break;
		}

		uint8_t nl = data[idx++];
		NET_LOG_F("[ClientManager] プレイヤー%d 名前長さ: %d", i + 1, (int)nl);

		if (idx + nl > len) {
			NET_LOG_F("[ClientManager] エラー: プレイヤー%d の名前データ不足 (必要:%d 残り:%d)",
				i + 1, (int)nl, (int)(len - idx));
			break;
		}

		std::string playerName(reinterpret_cast<const char*>(data + idx), nl);
		newNames.emplace_back(playerName);
		NET_LOG_F("[ClientManager] プレイヤー%d: '%s'", i + 1, playerName.c_str());
		idx += nl;
	}

	std::lock_guard<std::mutex> lk(m_lobbyMutex);
	int prev = (int)m_lobbyPlayerNames.size();
	m_lobbyPlayerNames = std::move(newNames);
	int now = (int)m_lobbyPlayerNames.size();

	NET_LOG_F("[ClientManager] ロビー更新: %d → %d 人", prev, now);

	if (now > prev) {
		// サウンドを鳴らす
	}
	m_previousLobbyCount = now;
}