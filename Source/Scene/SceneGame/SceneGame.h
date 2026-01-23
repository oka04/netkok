// SceneGame.h
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
#include <map>
#include <fstream>
#include "..\\..\\Object\\json.hpp"

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
	void SpawnPlayerWithRole(uint32_t clientId, const std::string& name, const D3DXVECTOR3& pos, PlayerRole role);
	void DespawnPlayer(uint32_t clientId);

	void RenderShadowMaps();

	void CheckBreathHitPlayers();
	void LoadGameParameter();

	void ProcessPlayerMelting();

	// ★★★ 勝敗判定関連 ★★★
	void UpdateGameTimer();
	void CheckGameEnd();
	bool AreAllRunnersFrozen();
	void BroadcastGameResult(int winnerTeam);  // 0=逃げる側, 1=鬼側
	void ProcessGameResult(int winnerTeam);

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
		GAME_END,
		RESULT_DISPLAY,
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
	D3DXVECTOR2 f_resultSize;
	std::vector<LPDIRECT3DTEXTURE9> m_shadowMaps;
	std::vector<LPDIRECT3DSURFACE9> m_shadowSurfaces;
	std::vector<LPDIRECT3DSURFACE9> m_shadowDepthSurfaces;
	std::vector<D3DXMATRIX> m_lightViewProjMatrices;
	static const int SHADOW_MAP_SIZE = 1024;
	static const int MAX_SPOT_LIGHTS = 4;

	void UpdateChaserLights();
	void ReceiveWorldState();
	float f_maxGaugeDistance;
	float f_chaserGaugeDistance;
	float f_wallAlphaFar;
	float f_wallAlphaMid;
	float f_wallAlphaNear;
	float f_wallDistanceMid;
	float f_wallDistanceFar;
	DWORD f_networkSendInterval;
	DWORD f_worldBroadcastInterval;

	bool m_bEnablePrediction;
	bool m_bEnableJitterReduction;

	float m_gameTime;
	float f_gameDuration;
	bool m_bGameEnded;
	int m_winnerTeam;
	DWORD m_resultDisplayStart;
	DWORD f_resultDisplayDuration;
	float m_resultImageAlpha;  // ★ リザルト画像のアルファ値
	float f_resultFadeSpeed;   // ★ リザルト画像のフェード速度
	float f_hostSpeedMultiplier = 0.8f;
};