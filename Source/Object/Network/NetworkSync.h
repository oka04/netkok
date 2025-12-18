// NetworkSync.h - ライト情報を追加
#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <cstdint>
#include <cstring>

// プレイヤーの役割
enum PlayerRole : uint8_t
{
	ROLE_NONE = 0,
	ROLE_RUNNER = 1,  // 逃げる側
	ROLE_CHASER = 2   // 鬼
};

// ★★★ ネットワークメッセージタイプ ★★★
enum NetMessageType : uint8_t
{
	NET_MSG_PLAYER_STATE = 1,
	NET_MSG_WORLD_STATE = 2,
	NET_MSG_PLAYER_SPAWN = 3,
	NET_MSG_PLAYER_DESPAWN = 4,
	NET_MSG_ROLE_ASSIGNMENT = 5,
	NET_MSG_CLIENT_ID_ASSIGNMENT = 6,
};

enum NetworkMessageType : uint8_t
{
	MSG_JOIN = 1,
	MSG_JOIN_ACK = 2,
	MSG_LOBBY_UPDATE = 3,
	MSG_START_GAME = 4,
	MSG_SERVER_INFO = 5,
	MSG_PLAYER_STATE = 10,
	MSG_WORLD_STATE = 11,
	MSG_PLAYER_SPAWN = 12,
	MSG_PLAYER_DESPAWN = 13,
	MSG_ROLE_ASSIGNMENT = 14,
};

// ★★★ プレイヤー状態（120バイト）★★★
#pragma pack(push, 1)
struct NetPlayerState
{
	uint32_t clientId;
	float posX, posY, posZ;
	float hAngle, vAngle;
	float depthX, depthY, depthZ;
	uint8_t keyFlag;
	uint8_t flags;

	// ライト情報（Chaser用）
	float lightPosX, lightPosY, lightPosZ;
	float lightDirX, lightDirY, lightDirZ;
	float lightRange;

	// ブレス情報
	uint8_t breathActive;
	float breathPosX, breathPosY, breathPosZ;
	float breathDirX, breathDirY, breathDirZ;

	uint8_t frozen;         
	float frozenAmount;     
	uint32_t meltTargetId;
	void SetFirstPerson(bool fp)
	{
		if (fp) flags |= 0x01;
		else flags &= ~0x01;
	}

	bool IsFirstPerson() const
	{
		return (flags & 0x01) != 0;
	}
};
#pragma pack(pop)

#pragma pack(push, 1)
struct NetWorldState
{
	int playerCount;
	NetPlayerState players[16];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct NetPlayerSpawn
{
	uint32_t clientId;
	char name[64];
	float startX, startY, startZ;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct NetRoleAssignment
{
	uint32_t clientId;
	PlayerRole role;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct NetClientIdAssignment
{
	uint32_t assignedClientId;
};
#pragma pack(pop)
	
class NetworkSerializer
{
public:
	static std::vector<uint8_t> SerializePlayerState(const NetPlayerState& state)
	{
		std::vector<uint8_t> buf;
		buf.push_back(MSG_PLAYER_STATE);
		const uint8_t* p = reinterpret_cast<const uint8_t*>(&state);
		buf.insert(buf.end(), p, p + sizeof(NetPlayerState));
		return buf;
	}

	static std::vector<uint8_t> SerializeWorldState(const NetWorldState& world)
	{
		std::vector<uint8_t> buf;
		buf.push_back(MSG_WORLD_STATE);
		buf.push_back(world.playerCount);
		for (int i = 0; i < world.playerCount; ++i)
		{
			const uint8_t* p = reinterpret_cast<const uint8_t*>(&world.players[i]);
			buf.insert(buf.end(), p, p + sizeof(NetPlayerState));
		}
		return buf;
	}

	static std::vector<uint8_t> SerializePlayerSpawn(const NetPlayerSpawn& spawn)
	{
		std::vector<uint8_t> buf;
		buf.push_back(MSG_PLAYER_SPAWN);
		const uint8_t* p = reinterpret_cast<const uint8_t*>(&spawn);
		buf.insert(buf.end(), p, p + sizeof(NetPlayerSpawn));
		return buf;
	}

	static std::vector<uint8_t> SerializePlayerDespawn(uint32_t clientId)
	{
		std::vector<uint8_t> buf;
		buf.push_back(MSG_PLAYER_DESPAWN);
		const uint8_t* p = reinterpret_cast<const uint8_t*>(&clientId);
		buf.insert(buf.end(), p, p + sizeof(uint32_t));
		return buf;
	}

	static std::vector<uint8_t> SerializeRoleAssignment(const NetRoleAssignment& assignment)
	{
		std::vector<uint8_t> buf;
		buf.push_back(MSG_ROLE_ASSIGNMENT);
		const uint8_t* p = reinterpret_cast<const uint8_t*>(&assignment);
		buf.insert(buf.end(), p, p + sizeof(NetRoleAssignment));
		return buf;
	}

	static bool DeserializePlayerState(const uint8_t* data, size_t len, NetPlayerState& out)
	{
		if (len < sizeof(NetPlayerState)) return false;
		std::memcpy(&out, data, sizeof(NetPlayerState));
		return true;
	}

	static bool DeserializeWorldState(const uint8_t* data, size_t len, NetWorldState& out)
	{
		if (len < 1) return false;
		out.playerCount = data[0];
		if (out.playerCount > 8) out.playerCount = 8;
		size_t needed = 1 + sizeof(NetPlayerState) * out.playerCount;
		if (len < needed) return false;
		for (int i = 0; i < out.playerCount; ++i)
		{
			std::memcpy(&out.players[i], data + 1 + i * sizeof(NetPlayerState), sizeof(NetPlayerState));
		}
		return true;
	}

	static bool DeserializeRoleAssignment(const uint8_t* data, size_t len, NetRoleAssignment& out)
	{
		if (len < sizeof(NetRoleAssignment)) return false;
		std::memcpy(&out, data, sizeof(NetRoleAssignment));
		return true;
	}
};