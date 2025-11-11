#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include "ServerManager.h"
#include "..\\ClientManager\\ClientManager.h"
#include "..\\Discovery\\Discovery.h"
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

// message ids
enum {
	MSG_JOIN = 1,
	MSG_JOIN_ACK = 2,
	MSG_LOBBY_UPDATE = 3,
	MSG_START_GAME = 4,
};

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

bool ServerManager::StartServer(int port, int maxm_clients)
{
	ENetAddress address;
	address.host = ENET_HOST_ANY;
	address.port = (enet_uint16)port;

	m_pServerHost = enet_host_create(&address, maxm_clients, 1, 0, 0);
	if (m_pServerHost == nullptr)
	{
		MessageBoxA(NULL, "サーバーホスト作成失敗", "エラー", MB_OK);
		return false;
	}

	// Start discovery advertisement on a discovery port (separate from ENet port)
	const uint16_t discoveryPort = 12346;
	m_advertiser = std::make_unique<Discovery>();
	m_advertiser->StartAdvertise(discoveryPort, (uint16_t)port, m_serverName, (uint8_t)maxm_clients);
	m_advertiser->SetAdvertisePlayerCount(0);
	m_advertiser->SetAdvertiseState(0);
	m_clientCount = 0;
	BroadcastLobbyUpdate();

	std::cout << "[Server] 起動: ポート " << port << std::endl;
	return true;
}

void ServerManager::StopServer()
{
	if (m_pServerHost)
	{
		enet_host_destroy(m_pServerHost);
		m_pServerHost = nullptr;
		m_clientCount = 0;
		for (auto &kv : m_clients) {
			delete kv.second;
		}
		m_clients.clear();
		std::cout << "[Server] 停止" << std::endl;
	}

	if (m_advertiser) {
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

	// format: [MSG_LOBBY_UPDATE][count][nameLen,name...]* (reliable)
	std::vector<uint8_t> payload;
	payload.push_back((uint8_t)MSG_LOBBY_UPDATE);
	uint8_t count = (uint8_t)m_clients.size();
	payload.push_back(count);
	for (auto &kv : m_clients) {
		const std::string &name = kv.second->name; 
		uint8_t nl = (uint8_t)(name.size() > 255 ? 255 : name.size());
		payload.push_back(nl);
		payload.insert(payload.end(), name.begin(), name.begin() + nl);
	}

	ENetPacket* packet = enet_packet_create(payload.data(), payload.size(), ENET_PACKET_FLAG_RELIABLE);
	enet_host_broadcast(m_pServerHost, 0, packet);
	enet_host_flush(m_pServerHost);

	if (m_advertiser)
	{
		m_advertiser->SetAdvertisePlayerCount(count);
		m_advertiser->SetAdvertiseState(0);
	}
}

void ServerManager::StartGame()
{
	if (!m_pServerHost) return;
	if (m_clients.size() < 2) {
		std::cout << "[Server] プレイヤーが足りません。開始できません。\n";
		return;
	}

	// set advertise state to in-game so discovery filters it out
	if (m_advertiser) m_advertiser->SetAdvertiseState(1);

	std::vector<uint8_t> payload;
	payload.push_back((uint8_t)MSG_START_GAME);
	ENetPacket* packet = enet_packet_create(payload.data(), payload.size(), ENET_PACKET_FLAG_RELIABLE);
	enet_host_broadcast(m_pServerHost, 0, packet);
	enet_host_flush(m_pServerHost);

	std::cout << "[Server] ゲーム開始通知を送信しました\n";
}

void ServerManager::SetServerName(std::string & name)
{
	m_serverName = name;
}

const std::string & ServerManager::GetServerName() const
{
	return m_serverName;
}

void ServerManager::SetHostName(const std::string& name)
{
	m_hostName = name;
}

const std::string& ServerManager::GetHostName() const
{
	return m_hostName;
}

std::vector<std::string> ServerManager::GetLobbyPlayerNames() const
{
	std::vector<std::string> names;
	for (auto& kv : m_clients) {
		names.push_back(kv.second->name);
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
	if (m_clientCount == 0 && !m_hostName.empty())
	{
		ci->name = m_hostName; 
	}
	else
	{
		ci->name = "Player";  
	}
	peer->data = ci;

	m_clients[peer] = ci;
	m_clientCount++;

	std::cout << "[Server] クライアント接続 (" << m_clientCount << "人)" << std::endl;
	BroadcastLobbyUpdate();
}

void ServerManager::OnClientReceive(const ENetEvent& event)
{
	const uint8_t* data = (const uint8_t*)event.packet->data;
	size_t len = event.packet->dataLength;
	if (len < 1) {
		enet_packet_destroy(event.packet);
		return;
	}

	uint8_t id = data[0];
	if (id == MSG_JOIN)
		ProcessJoin(event.peer, data, len);

	enet_packet_destroy(event.packet);
}

void ServerManager::OnClientDisconnect(ENetPeer* peer)
{
	auto it = m_clients.find(peer);
	if (it != m_clients.end()) {
		delete it->second;
		m_clients.erase(it);
		m_clientCount--;
		std::cout << "[Server] クライアント切断 (" << m_clientCount << "人)" << std::endl;
		BroadcastLobbyUpdate();
	}
}

void ServerManager::ProcessJoin(ENetPeer* peer, const uint8_t* data, size_t len)
{
	size_t idx = 1;
	if (idx >= len) return;

	uint8_t nl = data[idx++];
	if (idx + nl > len) return;

	std::string name(reinterpret_cast<const char*>(data + idx), nl);
	idx += nl;

	ClientInfo* ci = static_cast<ClientInfo*>(peer->data);
	if (ci) ci->name = name;

	BroadcastLobbyUpdate();
}