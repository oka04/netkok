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

	static ClientManager* GetInstance();

	bool ConnectToServer(const std::string& ip, int port);
	void Disconnect();
	void SendMessage(const char* msg);
	void Update();

	void SendJoin(const std::string& name);
	std::vector<std::string> GetLobbyPlayerNames();
	const std::vector<ServerInfoNet>& GetCachedServers() const;
	const std::vector<ServerInfoNet>& GetAllServers() const;
	const std::string& GetServerName() const;
	bool IsGameStarted() const;
	bool IsHost() const;
	bool IsConnected() const;
	void SetServerName(const std::string& name);
	void SetPlayerName(const std::string& name);
	const std::string& GetPlayerName() const;

	void RefreshAvailableServers();
	void Reset();

private:
	ENetHost* m_pClientHost;
	ENetPeer* m_pServerPeer;
	std::unique_ptr<Discovery> m_pDiscovery;
	std::vector<ServerInfoNet> m_availableServers;
	std::vector<ServerInfoNet> m_cachedServers;
	std::vector<ServerInfoNet> m_allServers;

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

	static ClientManager* s_instance;
};