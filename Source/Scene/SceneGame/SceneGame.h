#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include "..\\..\\GameBase.h"
#include "..\\Scene\\Scene.h"
#include "..\\..\\Object\\Fade\\Fade.h"
#include "..\\..\\Object\\Map\\Map.h"
#include "..\\..\\Object\\Player\\Player.h"
#include "..\\..\\Object\\Network\\ClientManager\\ClientManager.h"
#include "..\\..\\Object\\Network\\ServerManager\\ServerManager.h"
#include <map>

class SceneGame : public Scene
{
public:
	SceneGame(Engine *pEngine);
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
	void SyncPlayers();

	enum DEBUG_FLAG
	{
		DRAW_PLAYER_STATE = 1 << 0,
		DRAW_BOXLINE = 1 << 1,
		RELOAD_FILE = 1 << 2,
		PATROLLER_VIEW_LINE = 1 << 3,
		DISPLAY_DEBUG_STRING = 1 << 4,
		STOP_GAME = 1 << 5,
		DEBUG_MODE = 1 << 6,
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
	DWORD m_lastNetworkUpdate;

	uint32_t m_localClientId;
	bool m_bIsHost;
	Player* m_pLocalPlayer;
	std::map<uint32_t, Player*> m_remotePlayers;

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
};