#pragma once
#include <enet/enet.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Discovery;

struct ClientInfo
{
	ENetPeer* peer;
	uint32_t id;
	std::string name;
};

class ServerManager
{
public:
	ServerManager();
	~ServerManager();

	static ServerManager* GetInstance();

	bool StartServer(int port, int maxClients);
	void StopServer();
	void Update();  
	int GetClientCount() const;
	void BroadcastLobbyUpdate();
	void StartGame();
	void SetServerName(std::string& name);
	const std::string& GetServerName() const;
	void SetHostName(const std::string& name);
	const std::string& GetHostName() const;
	std::vector<std::string> GetLobbyPlayerNames() const;
private:
	void OnClientConnect(ENetPeer* peer);
	void OnClientReceive(const ENetEvent& event);
	void OnClientDisconnect(ENetPeer* peer);
	void ProcessJoin(ENetPeer* peer, const uint8_t* data, size_t len);

	ENetHost* m_pServerHost;
	int m_clientCount;

	std::unordered_map<ENetPeer*, ClientInfo*> m_clients;
	uint32_t m_nextClientId;

	std::unique_ptr<Discovery> m_advertiser;

	unsigned int m_lastAdvertiseTime;
	std::string m_serverName;
	std::string m_hostName;


	static ServerManager* s_instance;
};