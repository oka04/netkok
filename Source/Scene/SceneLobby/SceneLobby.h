#pragma once

#include "..\\..\\Object\\Network\\ClientManager\\ClientManager.h"
#include "..\\..\\Object\\Network\\ServerManager\\ServerManager.h"

#include "..\\..\\GameBase.h"
#include "..\\Scene\\Scene.h"
#include <vector>
#include <string>

enum REQUEST_MODE {
	NONE = 0,
	HOST = 1,
	FIND = 2
};

class SceneLobby : public Scene
{
public:
	SceneLobby(Engine* pEngine);
	~SceneLobby();

	void Start() override;
	void Update() override;
	void Draw() override;
	void PostEffect() override;
	void Exit() override;

	static void SetRequestedMode(REQUEST_MODE mode);
	static REQUEST_MODE GetRequestMode();

private:
	ClientManager* m_client;
	ServerManager* m_server;
	DWORD m_lastTime;

	IntVector2 f_buttonSize;
	IntVector2 f_backButtonPosition;
	IntVector2 f_startButtonPosition;
	IntVector2 f_serverNameLabelPosition;
	IntVector2 f_serverNamePosition;
	IntVector2 f_memberLabelPosition;
	IntVector2 f_memberNamePosition;
	std::string f_backButtonText;
	std::string f_startButtonText;
	std::string f_serverNameLabelText;
	std::string f_memberLabelText;
	int f_memberOffsetY;
	
	std::string m_serverName;
	int f_textOffsetY;
	int f_clientStartButtonAlpha;
	int m_selectedServerIndex;
	bool m_pressedMouseLast;

	std::string m_hostServerName;
	bool m_hostNameInputActive;

	bool PointInRect(IntVector2 pos, IntVector2 size);
	static REQUEST_MODE s_requestMode;
};