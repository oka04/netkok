#pragma once
#include "..\\Discovery\\Discovery.h"
#include <enet/enet.h>
#include <vector>
#include <memory>
#include <string>
#include <mutex>

struct ServerInfoNet
{
	unsigned int ip;
	enet_uint16 port;
	uint8_t playerCount;
	uint8_t maxPlayers;
	uint8_t state;
	std::string name;
};

class ClientManager
{
public:
	ClientManager();
	~ClientManager();

	// singleton
	static ClientManager* GetInstance();

	// discovery cached list (only updated when RefreshAvailableServers is called)
	bool ConnectToServer(const std::string& ip, int port);
	void Disconnect();
	void SendMessage(const char* msg);
	void Update();

	// lobby state
	void SendJoin(const std::string& name);
	std::vector<std::string> GetLobbyPlayerNames();
	const std::vector<ServerInfoNet>& GetCachedServers() const;     // 待機中のみ
	const std::vector<ServerInfoNet>& GetAllServers() const;        // 全サーバー（待機中 + ゲーム中）
	const std::string& GetServerName() const;
	bool IsGameStarted() const;
	bool IsHost() const;
	void SetPlayerName(const std::string& name);
	const std::string& GetPlayerName() const;

	// manual refresh: copy discovery->GetServers() into cachedServers
	void RefreshAvailableServers();
	bool IsConnected() const;

	static void DestroyInstance();
	void Reset();

private:
	ENetHost* m_pClientHost;
	ENetPeer* m_pServerPeer;
	ENetHost* m_pFinderHost;
	std::unique_ptr<Discovery> m_pDiscovery;
	std::vector<ServerInfoNet> m_availableServers; // internal temp（待機中のみ）
	std::vector<ServerInfoNet> m_cachedServers;    // UI-visible snapshot（待機中のみ）
	std::vector<ServerInfoNet> m_allServers;       // 全サーバー（待機中 + ゲーム中）

	void OnConnect();
	void OnReceive(const ENetEvent& event);
	void OnDisconnect();
	void ProcessLobbyUpdate(const uint8_t* data, size_t len);

	std::vector<std::string> m_lobbyPlayerNames;
	bool m_bGameStarted;
	bool m_bHost;
	std::mutex m_lobbyMutex;

	int m_previousLobbyCount;
	std::string m_playerName;
	std::string m_serverName;

	// singleton storage
	static ClientManager* s_instance;
};