#pragma once
#include <vector>
#include <string>
#include <mutex>
#include <chrono>
#include <cstdint>

struct ServerInfo
{
	uint32_t ip;    
	uint16_t port;  
	uint8_t playerCount;
	uint8_t maxPlayers;
	uint8_t state;    
	std::string name;
	std::chrono::steady_clock::time_point lastSeen;
};

class Discovery
{
public:
	Discovery();
	~Discovery();

	bool StartListener(uint16_t discoveryPort = 12346);
	void StopListener();

	bool StartAdvertise(uint16_t discoveryPort = 12346, uint16_t enetPort = 12345, const std::string& serverName = "Silent Host", uint8_t maxPlayers = 4);
	void StopAdvertise();

	void SetAdvertisePlayerCount(uint8_t count);
	void SetAdvertiseState(uint8_t state);
	void SetAdvertiseName(const std::string& name);
	void SetAdvertiseMaxPlayers(uint8_t maxPlayers);

	std::vector<ServerInfo> GetServers();

	void SetExpireSeconds(int sec);

private:
	struct Impl;
	Impl* impl;
};