#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include "ClientManager.h"
#include <windows.h>
#include <iostream>
#include <algorithm>

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
	if (enet_initialize() != 0)
	{
		MessageBoxA(NULL, "ENetの初期化に失敗しました。", "エラー", MB_OK);
	}

	m_pDiscovery = std::make_unique<Discovery>();
	m_pDiscovery->StartListener(12346);
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

bool ClientManager::ConnectToServer(const char* ip, int port)
{
	m_pClientHost = enet_host_create(nullptr, 1, 1, 0, 0);
	if (!m_pClientHost)
	{
		MessageBoxA(NULL, "クライアント作成失敗", "エラー", MB_OK);
		return false;
	}

	ENetAddress address;
	enet_address_set_host(&address, ip);
	address.port = (enet_uint16)port;

	m_pServerPeer = enet_host_connect(m_pClientHost, &address, 1, 0);
	if (!m_pServerPeer)
	{
		MessageBoxA(NULL, "サーバー接続要求失敗", "エラー", MB_OK);
		return false;
	}

	// mark host flag if connecting to localhost
	m_bHost = (strcmp(ip, "127.0.0.1") == 0 || strcmp(ip, "localhost") == 0);

	std::cout << "[Client] 接続要求送信中..." << std::endl;
	return true;
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

bool ClientManager::ConnectToHost(const std::string& ip, int port)
{
	if (m_pClientHost)
	{
		enet_host_destroy(m_pClientHost);
		m_pClientHost = nullptr;
	}

	m_pClientHost = enet_host_create(nullptr, 1, 1, 0, 0);
	if (!m_pClientHost)
	{
		MessageBoxA(NULL, "クライアントの作成に失敗しました。", "エラー", MB_OK | MB_ICONERROR);
		return false;
	}

	ENetAddress address;
	enet_address_set_host(&address, ip.c_str());
	address.port = (enet_uint16)port;

	m_pServerPeer = enet_host_connect(m_pClientHost, &address, 1, 0);
	if (!m_pServerPeer)
	{
		MessageBoxA(NULL, "サーバーへの接続要求に失敗しました。", "エラー", MB_OK | MB_ICONERROR);
		return false;
	}

	// 自分がホストか判定（127.0.0.1 または localhost の場合）
	m_bHost = (ip == "127.0.0.1" || ip == "localhost");

	std::cout << "[Client] サーバー (" << ip << ":" << port << ") に接続要求を送信しました。" << std::endl;
	return true;
}

void ClientManager::RefreshAvailableServers()
{
	m_availableServers.clear();
	if (!m_pDiscovery) return;
	auto servers = m_pDiscovery->GetServers();
	for (auto &s : servers) {
		if (s.state != 0) continue; // only show lobby servers
		ServerInfoNet n;
		n.ip = s.ip;
		n.port = s.port;
		n.playerCount = s.playerCount;
		n.maxPlayers = s.maxPlayers;
		n.state = s.state;
		n.name = s.name;
		m_availableServers.push_back(n);
	}
	// swap into cached
	m_cachedServers = m_availableServers;
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
	std::cout << "[Client] サーバー接続成功" << std::endl;
	SendJoin(m_playerName.empty() ? "Player" : m_playerName);
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
	std::cout << "[Client] サーバーから切断" << std::endl;
}

void ClientManager::ProcessLobbyUpdate(const uint8_t* data, size_t len)
{
	size_t idx = 1;
	if (idx >= len) return;

	uint8_t count = data[idx++];
	std::vector<std::string> newNames;
	newNames.reserve(count);

	for (int i = 0; i < count && idx < len; ++i)
	{
		uint8_t nl = data[idx++];
		if (idx + nl > len) break;
		newNames.emplace_back(reinterpret_cast<const char*>(data + idx), nl);
		idx += nl;
	}

	std::lock_guard<std::mutex> lk(m_lobbyMutex);
	int prev = (int)m_lobbyPlayerNames.size();
	m_lobbyPlayerNames = std::move(newNames);
	int now = (int)m_lobbyPlayerNames.size();

	if (now > prev) {
		// サウンドを鳴らす
	}
	m_previousLobbyCount = now;
}