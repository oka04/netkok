#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <atomic>
#include <map>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cstddef>
#include "Discovery.h"
#include "..\\NetworkLogger.h"

#pragma comment(lib, "ws2_32.lib")

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

	uint16_t advEnetPort;
	uint8_t advPlayerCount;
	uint8_t advMaxPlayers;
	uint8_t advState;
	std::string advName;

	std::mutex mtx;
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
	if (impl->listenerRunning) {
		NET_LOG("[Discovery/Listener] 既に起動済み");
		return true;
	}

	impl->discoveryPort = discoveryPort;
	impl->listenerRunning = true;

	impl->listenerThread = std::thread([this]() {
		NET_LOG("[Discovery/Listener] スレッド開始");

		WSADATA wsa;
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
			NET_LOG_F("[Discovery/Listener] WSAStartup失敗: %d", WSAGetLastError());
			impl->listenerRunning = false;
			return;
		}
		NET_LOG("[Discovery/Listener] WSAStartup成功");

		SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (sock == INVALID_SOCKET) {
			NET_LOG_F("[Discovery/Listener] socket作成失敗: %d", WSAGetLastError());
			impl->listenerRunning = false;
			WSACleanup();
			return;
		}
		NET_LOG("[Discovery/Listener] ソケット作成成功");

		BOOL reuse = TRUE;
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) == SOCKET_ERROR) {
			NET_LOG("[Discovery/Listener] SO_REUSEADDR設定失敗（続行）");
		}

		sockaddr_in local;
		local.sin_family = AF_INET;
		local.sin_addr.s_addr = INADDR_ANY;
		local.sin_port = htons((u_short)impl->discoveryPort);

		if (bind(sock, (sockaddr*)&local, sizeof(local)) == SOCKET_ERROR) {
			NET_LOG_F("[Discovery/Listener] bind失敗 ポート:%d エラー:%d", impl->discoveryPort, WSAGetLastError());
			closesocket(sock);
			impl->listenerRunning = false;
			WSACleanup();
			return;
		}
		NET_LOG_F("[Discovery/Listener] bind成功 ポート:%d", impl->discoveryPort);

		u_long mode = 1;
		ioctlsocket(sock, FIONBIO, &mode);

		char buf[512];
		sockaddr_in from;
		int fromlen = sizeof(from);

		const char magic[] = "SILENT_DISC";
		const int magicLen = 10;

		NET_LOG("[Discovery/Listener] 受信ループ開始");
		int loopCount = 0;

		while (impl->listenerRunning) {
			loopCount++;
			if (loopCount % 50 == 0) {
				NET_LOG_F("[Discovery/Listener] 生存確認 ループ:%d", loopCount);
			}

			int recvLen = recvfrom(sock, buf, sizeof(buf), 0, (sockaddr*)&from, &fromlen);

			if (recvLen > 0) {
				NET_LOG_F("[Discovery/Listener] パケット受信! サイズ:%d bytes", recvLen);

				if (recvLen < 18) {
					NET_LOG("[Discovery/Listener] パケットサイズ不足 (最小18bytes必要)");
					continue;
				}

				if (memcmp(buf, magic, magicLen) != 0) {
					NET_LOG("[Discovery/Listener] マジック不一致");
					continue;
				}
				NET_LOG("[Discovery/Listener] マジック一致確認");

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

				if (idx + (int)nameLen > recvLen) {
					NET_LOG("[Discovery/Listener] 名前長さ不正");
					continue;
				}
				std::string name(buf + idx, nameLen);

				char ipstr[INET_ADDRSTRLEN] = { 0 };
				inet_ntop(AF_INET, &from.sin_addr, ipstr, sizeof(ipstr));
				std::string key = std::string(ipstr) + ":" + std::to_string(enetPort);

				NET_LOG_F("[Discovery/Listener] ★サーバー検出★: %s @ %s:%d (%d/%d) state=%d",
					name.c_str(), ipstr, enetPort, (int)playerCount, (int)maxPlayers, (int)state);

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
					NET_LOG_F("[Discovery/Listener] サーバーリストに追加 (合計:%d)", (int)impl->servers.size());
				}
			}
			else if (recvLen == SOCKET_ERROR) {
				int error = WSAGetLastError();
				if (error != WSAEWOULDBLOCK && loopCount % 100 == 0) {
					NET_LOG_F("[Discovery/Listener] recvfrom エラー:%d", error);
				}
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(100));

			{
				std::lock_guard<std::mutex> lk(impl->mtx);
				auto now = std::chrono::steady_clock::now();
				std::vector<std::string> eraseKeys;
				for (auto &kv : impl->servers) {
					auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - kv.second.lastSeen).count();
					if (diff > impl->expireSeconds) {
						eraseKeys.push_back(kv.first);
					}
				}
				for (auto &k : eraseKeys) {
					NET_LOG_F("[Discovery/Listener] タイムアウト削除: %s", k.c_str());
					impl->servers.erase(k);
				}
			}
		}

		NET_LOG("[Discovery/Listener] 受信ループ終了");
		closesocket(sock);
		WSACleanup();
	});

	return true;
}

void Discovery::StopListener()
{
	if (!impl->listenerRunning) return;
	NET_LOG("[Discovery/Listener] 停止中...");
	impl->listenerRunning = false;
	if (impl->listenerThread.joinable()) impl->listenerThread.join();
	NET_LOG("[Discovery/Listener] 停止完了");
}

bool Discovery::StartAdvertise(uint16_t discoveryPort, uint16_t enetPort, const std::string& serverName, uint8_t maxPlayers)
{
	if (impl->advertiserRunning) {
		NET_LOG("[Discovery/Advertiser] 既に起動済み");
		return true;
	}

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
		NET_LOG("[Discovery/Advertiser] スレッド開始");

		WSADATA wsa;
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
			NET_LOG_F("[Discovery/Advertiser] WSAStartup失敗: %d", WSAGetLastError());
			impl->advertiserRunning = false;
			return;
		}
		NET_LOG("[Discovery/Advertiser] WSAStartup成功");

		SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (sock == INVALID_SOCKET) {
			NET_LOG_F("[Discovery/Advertiser] socket作成失敗: %d", WSAGetLastError());
			impl->advertiserRunning = false;
			WSACleanup();
			return;
		}
		NET_LOG("[Discovery/Advertiser] ソケット作成成功");

		BOOL opt = TRUE;
		if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
			NET_LOG_F("[Discovery/Advertiser] SO_BROADCAST設定失敗: %d", WSAGetLastError());
		}
		else {
			NET_LOG("[Discovery/Advertiser] SO_BROADCAST設定成功");
		}

		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons((u_short)impl->discoveryPort);
		addr.sin_addr.s_addr = inet_addr("255.255.255.255");

		const char magic[] = "SILENT_DISC";
		int advertiseCount = 0;

		NET_LOG("[Discovery/Advertiser] ブロードキャストループ開始");

		while (impl->advertiserRunning) {
			advertiseCount++;

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

			int sent = sendto(sock, payload.data(), (int)payload.size(), 0, (sockaddr*)&addr, sizeof(addr));

			if (sent == SOCKET_ERROR) {
				NET_LOG_F("[Discovery/Advertiser] sendto失敗: %d", WSAGetLastError());
			}
			else {
				NET_LOG_F("[Discovery/Advertiser] ブロードキャスト送信 #%d: %s (%d/%d) サイズ:%d bytes ポート:%d",
					advertiseCount, name.c_str(), (int)playerCount, (int)maxPlayers, sent, impl->discoveryPort);
			}

			for (int i = 0; i < 50 && impl->advertiserRunning; i++)
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		NET_LOG("[Discovery/Advertiser] ブロードキャストループ終了");
		closesocket(sock);
		WSACleanup();
	});

	return true;
}

void Discovery::StopAdvertise()
{
	if (!impl->advertiserRunning) return;
	NET_LOG("[Discovery/Advertiser] 停止中...");
	impl->advertiserRunning = false;
	if (impl->advertiserThread.joinable()) impl->advertiserThread.join();
	NET_LOG("[Discovery/Advertiser] 停止完了");
}

std::vector<ServerInfo> Discovery::GetServers()
{
	std::vector<ServerInfo> out;
	std::lock_guard<std::mutex> lk(impl->mtx);
	out.reserve(impl->servers.size());
	for (auto &kv : impl->servers) out.push_back(kv.second);

	NET_LOG_F("[Discovery] GetServers呼び出し: %d サーバー返却", (int)out.size());
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