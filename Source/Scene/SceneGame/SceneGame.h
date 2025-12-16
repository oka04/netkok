#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include "..\\..\\GameBase.h"
#include "..\\Scene\\Scene.h"
#include "..\\..\\Object\\Fade\\Fade.h"
#include "..\\..\\Object\\Map\\Map.h"
#include "..\\..\\Object\\Chaser\\Chaser.h"
#include "..\\..\\Object\\Runner\\Runner.h"
#include "..\\..\\Object\\Network\\ClientManager\\ClientManager.h"
#include "..\\..\\Object\\Network\\ServerManager\\ServerManager.h"
#include "..\\..\\Object\\Network\\NetworkSync.h"
#include "..\\..\\Object\\IceBlock\\IceBlock.h"
#include <map>

class SceneGame : public Scene
{
public:
	SceneGame(Engine* pEngine);
	~SceneGame();

	void Start();
	void Update();
	void Draw();
	void PostEffect();
	void Exit();

#ifdef USE_IMGUI
	void ImGuiFrameProcess();
#endif

private:
	void Initialize();
	void UpdateDebugFlag();
	void UpdateNetwork();
	void UpdateLocalPlayer();
	void UpdateRemotePlayers();
	void SyncToServer();
	void ReceiveFromServer();
	void SceneGame::SpawnPlayerWithRole(uint32_t clientId, const std::string& name, const D3DXVECTOR3& pos, PlayerRole role);
	void DespawnPlayer(uint32_t clientId);

	// ★★★ シャドウマップ関連のメソッドを追加 ★★★
	void CreateShadowMaps();
	void RenderShadowMaps();
	void ReleaseShadowMaps();

	enum DEBUG_FLAG
	{
		DRAW_PLAYER_STATE = 1 << 0,
		DRAW_BOXLINE = 1 << 1,
		RELOAD_FILE = 1 << 2,
		PATROLLER_VIEW_LINE = 1 << 3,
		DISPLAY_DEBUG_STRING = 1 << 4,
		STOP_GAME = 1 << 5,
		DEBUG_MODE = 1 << 6,
		SHOW_ICE_BLOCK = 1 << 7,      
		RESET_ICE_BLOCK = 1 << 8,
	};

	enum VIEW_KIND
	{
		VIEW_GAME,
		VIEW_FIRST,
		VIEW_THIRD,
		VIEW_MAX
	};

	enum GAME_STATE
	{
		FADE_IN,
		IN_GAME,
		CHANGE_SCENE,
		FADE_OUT,
	};

	unsigned char d_debugFlag;
	int d_fpsCount;
	int d_viewPointCount;
	int m_gameState;
	float m_deltaTime;
	float f_miniMapSourHalfSize;
	DWORD m_lastTime;
	DWORD m_lastNetworkSend;
	DWORD m_lastWorldBroadcast;

	uint32_t m_localClientId;
	bool m_bIsHost;
	bool m_bInitialSyncDone;
	bool m_bFirstPerson;
	CharacterBase* m_pLocalPlayer;
	std::map<uint32_t, CharacterBase*> m_players;
	PlayerRole m_localRole;
	std::map<uint32_t, PlayerRole> m_playerRoles;

	Camera m_camera;
	Projection m_projection;
	Viewport m_viewport;
	AmbientLight m_ambient;
	DirectionalLight m_light;
	Map m_map;
	Fade m_fade;

	D3DXVECTOR3 m_outPatrollerPosition;

	ClientManager* m_pClient;
	ServerManager* m_pServer;
	std::vector<SpotLight*> m_chaserLights;

	// ★★★ シャドウマップ関連のメンバー変数を追加 ★★★
	std::vector<LPDIRECT3DTEXTURE9> m_shadowMaps;
	std::vector<LPDIRECT3DSURFACE9> m_shadowSurfaces;
	std::vector<LPDIRECT3DSURFACE9> m_shadowDepthSurfaces;
	std::vector<D3DXMATRIX> m_lightViewProjMatrices;
	static const int SHADOW_MAP_SIZE = 1024;
	static const int MAX_SPOT_LIGHTS = 4;

	void UpdateChaserLights();
	void ReceiveWorldState();
	static const DWORD NETWORK_SEND_INTERVAL = 16;
	static const DWORD WORLD_BROADCAST_INTERVAL = 8;
	bool m_bEnablePrediction;
	bool m_bEnableJitterReduction;
	IceBlock* m_pDebugIceBlock;
};