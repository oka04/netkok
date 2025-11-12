#pragma once

#include "..\\..\\Object\\Network\\ClientManager\\ClientManager.h"
#include "..\\..\\Object\\Network\\ServerManager\\ServerManager.h"
#include "..\\Object\\Network\\NetworkLogger.h"
#include "..\\SceneLobby\\SceneLobby.h"
#include "..\\..\\GameBase.h"
#include "..\\Scene\\Scene.h"

class SceneSelectClient : public Scene
{
public:
	SceneSelectClient(Engine *pEngine);
	~SceneSelectClient();
	void Start();
	void Update();
	void Draw();
	void PostEffect();
	void Exit();

private:
	std::vector<ServerInfoNet> m_waitingServers;  // 待機中のサーバー
	std::vector<ServerInfoNet> m_ingameServers;   // ゲーム中のサーバー

	int selectedIndex;
	IntVector2 f_serverNamePosition;

	std::string m_playerName;
	bool m_nameInputActive;
	int m_cursorPosition;
	float m_timer;
	bool m_cursorVisible;
	float m_deltaTime;
	DWORD m_lastTime;

	DWORD m_lastRefreshTime;
	DWORD m_refreshInterval;
	bool m_bMouseDownLast;

	int m_scrollOffset;        // スクロール位置
	int m_maxVisibleServers;   // 一度に表示できる最大サーバー数

	bool PointInRect(int x, int y, int w, int h, POINT pt);
	void RefreshServerList();  // サーバーリストを待機中とゲーム中に分ける
};