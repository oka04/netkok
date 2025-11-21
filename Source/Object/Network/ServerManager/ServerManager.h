#pragma once
#include <enet/enet.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include "..\\NetworkSync.h"

class Discovery;

struct ClientInfo
{
	ENetPeer* peer;
	uint32_t id;
	std::string name;
	NetPlayerState lastState;
	bool stateReceived;
};

class ServerManager
{
public:
	ServerManager();
	~ServerManager();

	static ServerManager* GetInstance();

	bool StartServer(int port, int maxClients);
	void StopServer();
	void Reset();
	void Update();
	int GetClientCount() const;
	void BroadcastLobbyUpdate();
	void StartGame();
	void SetServerName(std::string& name);
	const std::string& GetServerName() const;
	void SetHostName(const std::string& name);
	const std::string& GetHostName() const;
	std::vector<std::string> GetLobbyPlayerNames() const;

	void UpdatePlayerState(uint32_t clientId, const NetPlayerState& state);
	void BroadcastWorldState();
	void BroadcastPlayerSpawn(const NetPlayerSpawn& spawn);
	void BroadcastPlayerDespawn(uint32_t clientId);
	std::vector<NetPlayerState> GetAllPlayerStates() const;
	uint32_t GetNextClientId() const { return m_nextClientId; }

	void SetHostState(const NetPlayerState& state);

private:
	void OnClientConnect(ENetPeer* peer);
	void OnClientReceive(const ENetEvent& event);
	void OnClientDisconnect(ENetPeer* peer);
	void ProcessJoin(ENetPeer* peer, const uint8_t* data, size_t len);
	void ProcessPlayerState(ENetPeer* peer, const uint8_t* data, size_t len);
	void SendToClient(ENetPeer* peer, const std::vector<uint8_t>& data);
	void SendJoinAck(ENetPeer* peer, uint32_t clientId);

	ENetHost* m_pServerHost;
	int m_clientCount;

	std::unordered_map<ENetPeer*, ClientInfo*> m_clients;
	uint32_t m_nextClientId;

	std::unique_ptr<Discovery> m_advertiser;

	unsigned int m_lastAdvertiseTime;
	std::string m_serverName;
	std::string m_hostName;

	NetPlayerState m_hostState;
	bool m_hostStateSet;
	mutable std::mutex m_stateMutex;

	static ServerManager* s_instance;
};