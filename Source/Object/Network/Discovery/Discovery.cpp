#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <atomic>
#include <map>
#include <vector>
#include <cstring>
#include <algorithm> // for std::min
#include <cstddef>
#include "Discovery.h"

#pragma comment(lib, "ws2_32.lib")

			// Payload format (binary):
			// [0..9] magic "SILENT_DISC" (10 bytes)
			// [10..11] protocolVersion (uint16_t, network byte order)
			// [12..13] enetPort (uint16_t, network byte order)
			// [14] playerCount (uint8_t)
			// [15] maxPlayers (uint8_t)
			// [16] state (uint8_t) // 0=lobby,1=in-game
			// [17] nameLen (uint8_t)
			// [18..] name (nameLen bytes, UTF-8)

struct Discovery::Impl
{
	Impl()
		: listenerThread(), advertiserThread(),
		listenerRunning(false), advertiserRunning(false),
		discoveryPort(12346), expireSeconds(10),
		advEnetPort(12345), advPlayerCount(0), advMaxPlayers(4), advState(0), advName("Silent Host")
	{}

	std::thread listenerThread;
	std::thread advertiserThread;
	std::atomic<bool> listenerRunning;
	std::atomic<bool> advertiserRunning;
	uint16_t discoveryPort;
	int expireSeconds;

	// advertise info (protected by mtx)
	uint16_t advEnetPort;
	uint8_t advPlayerCount;
	uint8_t advMaxPlayers;
	uint8_t advState; // 0 lobby, 1 in-game
	std::string advName;

	std::mutex mtx;
	// key = ip:port string
	std::map<std::string, ServerInfo> servers;
};

Discovery::Discovery()
	: impl(new Impl())
{
}

Discovery::~Discovery()
{
	StopAdvertise();
	StopListener();
	delete impl;
}

bool Discovery::StartListener(uint16_t discoveryPort)
{
	if (impl->listenerRunning) return true;
	impl->discoveryPort = discoveryPort;
	impl->listenerRunning = true;

	impl->listenerThread = std::thread([this]() {
		WSADATA wsa;
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
			impl->listenerRunning = false;
			return;
		}

		SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (sock == INVALID_SOCKET) {
			impl->listenerRunning = false;
			WSACleanup();
			return;
		}

		sockaddr_in local;
		local.sin_family = AF_INET;
		local.sin_addr.s_addr = INADDR_ANY;
		local.sin_port = htons((u_short)impl->discoveryPort);

		if (bind(sock, (sockaddr*)&local, sizeof(local)) == SOCKET_ERROR) {
			closesocket(sock);
			impl->listenerRunning = false;
			WSACleanup();
			return;
		}

		// non-blocking
		u_long mode = 1;
		ioctlsocket(sock, FIONBIO, &mode);

		char buf[512];
		sockaddr_in from;
		int fromlen = sizeof(from);

		const char magic[] = "SILENT_DISC";
		const int magicLen = 10;

		while (impl->listenerRunning) {
			int recvLen = recvfrom(sock, buf, sizeof(buf), 0, (sockaddr*)&from, &fromlen);
			if (recvLen > 0) {
				if (recvLen < 18) continue; // minimal size
				if (memcmp(buf, magic, magicLen) != 0) continue;

				int idx = magicLen;
				if (idx + 2 > recvLen) continue;
				uint16_t protoVer = ntohs(*(uint16_t*)(buf + idx)); idx += 2;
				if (idx + 2 > recvLen) continue;
				uint16_t enetPort = ntohs(*(uint16_t*)(buf + idx)); idx += 2;
				if (idx + 1 > recvLen) continue;
				uint8_t playerCount = *(uint8_t*)(buf + idx); idx += 1;
				if (idx + 1 > recvLen) continue;
				uint8_t maxPlayers = *(uint8_t*)(buf + idx); idx += 1;
				if (idx + 1 > recvLen) continue;
				uint8_t state = *(uint8_t*)(buf + idx); idx += 1;
				if (idx + 1 > recvLen) continue;
				uint8_t nameLen = *(uint8_t*)(buf + idx); idx += 1;
				if (idx + (int)nameLen > recvLen) continue;
				std::string name(buf + idx, nameLen);

				char ipstr[INET_ADDRSTRLEN] = { 0 };
				inet_ntop(AF_INET, &from.sin_addr, ipstr, sizeof(ipstr));
				std::string key = std::string(ipstr) + ":" + std::to_string(enetPort);

				ServerInfo si;
				si.ip = from.sin_addr.s_addr;
				si.port = enetPort;
				si.playerCount = playerCount;
				si.maxPlayers = maxPlayers;
				si.state = state;
				si.name = name;
				si.lastSeen = std::chrono::steady_clock::now();

				{
					std::lock_guard<std::mutex> lk(impl->mtx);
					impl->servers[key] = si;
				}
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}

			// expire old entries
			{
				std::lock_guard<std::mutex> lk(impl->mtx);
				auto now = std::chrono::steady_clock::now();
				std::vector<std::string> eraseKeys;
				for (auto &kv : impl->servers) {
					auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - kv.second.lastSeen).count();
					if (diff > impl->expireSeconds) eraseKeys.push_back(kv.first);
				}
				for (auto &k : eraseKeys) impl->servers.erase(k);
			}
		}

		closesocket(sock);
		WSACleanup();
	});

	return true;
}

void Discovery::StopListener()
{
	if (!impl->listenerRunning) return;
	impl->listenerRunning = false;
	if (impl->listenerThread.joinable()) impl->listenerThread.join();
}

bool Discovery::StartAdvertise(uint16_t discoveryPort, uint16_t enetPort, const std::string& serverName, uint8_t maxPlayers)
{
	if (impl->advertiserRunning) return true;
	impl->discoveryPort = discoveryPort;
	{
		std::lock_guard<std::mutex> lk(impl->mtx);
		impl->advEnetPort = enetPort;
		impl->advMaxPlayers = maxPlayers;
		impl->advName = serverName;
		impl->advPlayerCount = 0;
		impl->advState = 0;
	}
	impl->advertiserRunning = true;

	impl->advertiserThread = std::thread([this]() {
		WSADATA wsa;
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
			impl->advertiserRunning = false;
			return;
		}

		SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (sock == INVALID_SOCKET) {
			impl->advertiserRunning = false;
			WSACleanup();
			return;
		}

		BOOL opt = TRUE;
		setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char*)&opt, sizeof(opt));

		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons((u_short)impl->discoveryPort);
		addr.sin_addr.s_addr = inet_addr("255.255.255.255");

		const char magic[] = "SILENT_DISC";

		while (impl->advertiserRunning) {
			// read advertise info under lock
			uint16_t enetPort;
			uint8_t playerCount;
			uint8_t maxPlayers;
			uint8_t state;
			std::string name;
			{
				std::lock_guard<std::mutex> lk(impl->mtx);
				enetPort = impl->advEnetPort;
				playerCount = impl->advPlayerCount;
				maxPlayers = impl->advMaxPlayers;
				state = impl->advState;
				name = impl->advName;
			}

			std::vector<char> payload;
			payload.insert(payload.end(), magic, magic + sizeof(magic) - 1);

			uint16_t protoVerNet = htons(1);
			payload.push_back(static_cast<char>((protoVerNet >> 8) & 0xFF));
			payload.push_back(static_cast<char>(protoVerNet & 0xFF));

			uint16_t portNet = htons(enetPort);
			payload.push_back(static_cast<char>((portNet >> 8) & 0xFF));
			payload.push_back(static_cast<char>(portNet & 0xFF));

			payload.push_back(static_cast<char>(playerCount));
			payload.push_back(static_cast<char>(maxPlayers));
			payload.push_back(static_cast<char>(state));

			size_t nameLen = std::min<size_t>(255, name.size());
			payload.push_back(static_cast<char>(nameLen));
			payload.insert(payload.end(), name.begin(), name.begin() + nameLen);

			sendto(sock, payload.data(), (int)payload.size(), 0, (sockaddr*)&addr, sizeof(addr));

			// advertise every 5 seconds (check flag more frequently)
			for (int i = 0; i<50 && impl->advertiserRunning; i++) std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		closesocket(sock);
		WSACleanup();
	});

	return true;
}

void Discovery::StopAdvertise()
{
	if (!impl->advertiserRunning) return;
	impl->advertiserRunning = false;
	if (impl->advertiserThread.joinable()) impl->advertiserThread.join();
}

std::vector<ServerInfo> Discovery::GetServers()
{
	std::vector<ServerInfo> out;
	std::lock_guard<std::mutex> lk(impl->mtx);
	out.reserve(impl->servers.size());
	for (auto &kv : impl->servers) out.push_back(kv.second);
	return out;
}

void Discovery::SetExpireSeconds(int sec)
{
	impl->expireSeconds = sec;
}

void Discovery::SetAdvertisePlayerCount(uint8_t count)
{
	std::lock_guard<std::mutex> lk(impl->mtx);
	impl->advPlayerCount = count;
}

void Discovery::SetAdvertiseState(uint8_t state)
{
	std::lock_guard<std::mutex> lk(impl->mtx);
	impl->advState = state;
}

void Discovery::SetAdvertiseName(const std::string& name)
{
	std::lock_guard<std::mutex> lk(impl->mtx);
	impl->advName = name;
}

void Discovery::SetAdvertiseMaxPlayers(uint8_t maxPlayers)
{
	std::lock_guard<std::mutex> lk(impl->mtx);
	impl->advMaxPlayers = maxPlayers;
}