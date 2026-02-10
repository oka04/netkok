#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include "..\\GameBase.h"
#include "..\\Discovery\\Discovery.h"
#include "..\\NetworkSync.h"
#include <enet/enet.h>
#include <vector>
#include <memory>
#include <string>
#include <mutex>
#include <queue>
#include <map>

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
	void ResetForLobbyReturn();

	void SendPlayerState(const NetPlayerState& state);
	bool GetWorldState(NetWorldState& out);
	bool PopPlayerSpawn(NetPlayerSpawn& out);
	bool PopPlayerDespawn(uint32_t& out);
	uint32_t GetAssignedClientId() const { return m_assignedClientId; }
	void SetAssignedClientId(uint32_t id) { m_assignedClientId = id; }

	bool PopRoleAssignment(NetRoleAssignment& out);
	PlayerRole GetMyRole() const { return m_myRole; }
	const std::map<uint32_t, PlayerRole>& GetAllRoles() const { return m_roleMap; }

	bool PopGameResult(NetGameResult& out);

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
	void ProcessServerInfo(const uint8_t* data, size_t len);
	void ProcessWorldState(const uint8_t* data, size_t len);
	void ProcessPlayerSpawn(const uint8_t* data, size_t len);
	void ProcessPlayerDespawn(const uint8_t* data, size_t len);
	void ProcessJoinAck(const uint8_t* data, size_t len);
	void ProcessRoleAssignment(const uint8_t* data, size_t len);
	void ProcessGameResult(const uint8_t* data, size_t len);

	std::vector<std::string> m_lobbyPlayerNames;
	bool m_bGameStarted;
	bool m_bHost;
	bool m_bConnected;
	std::mutex m_lobbyMutex;

	int m_previousLobbyCount;
	std::string m_playerName;
	std::string m_serverName;
	DWORD m_lastHeartbeatTime;

	NetWorldState m_worldState;
	std::mutex m_worldMutex;
	bool m_worldStateReceived;
	std::queue<NetPlayerSpawn> m_spawnQueue;
	std::queue<uint32_t> m_despawnQueue;
	std::queue<NetRoleAssignment> m_roleQueue;
	std::queue<NetGameResult> m_resultQueue;
	uint32_t m_assignedClientId;

	PlayerRole m_myRole;
	std::map<uint32_t, PlayerRole> m_roleMap;

	static ClientManager* s_instance;
};