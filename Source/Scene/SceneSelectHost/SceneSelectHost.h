#pragma once

#include "..\\..\\Object\\Network\\ClientManager\\ClientManager.h"
#include "..\\..\\Object\\Network\\ServerManager\\ServerManager.h"
#include "..\\SceneLobby\\SceneLobby.h"
#include "..\\..\\GameBase.h"
#include "..\\Scene\\Scene.h"

class SceneSelectHost : public Scene
{
public:
	SceneSelectHost(Engine *pEngine);
	~SceneSelectHost();
	void Start();
	void Update();
	void Draw();
	void PostEffect();
	void Exit();
private:
	bool IsMouseOverButton(IntVector2 buttonPos);
	void UpdateAlpha();
	void UpdateButton();
	bool CreateServer();
	IntVector2 f_buttonSize;
	IntVector2 f_backButtonPosition;
	IntVector2 f_enterButtonPosition;
	IntVector2 f_serverNameLabelPosition;
	IntVector2 f_serverNameInputPosition;
	IntVector2 f_errorNameTextPosition;
	std::string f_errorNamesText;
	std::string f_serverNameLabelText;
	std::string f_backButtonText;
	std::string f_enterButtonText;
	std::string f_emptyNameText;

	std::string m_serverName;
	std::string m_playerName;

	DWORD f_errorNameViewTime;
	DWORD m_errorNameStartTime;
	bool m_bErrorName;

	int f_textOffsetY;
	int f_maxNameCcount;
	int f_clickMargin;

	float m_changeAlpha;
	float m_buttonAlpha;

	float m_alpha;
	float f_maxAlpha;
	float f_changeAlphaValue;
	float f_minAlpha;
	bool m_bMouseOverBackButton;
	bool m_bMouseOverEnterButton;

	float m_deltaTime;
	DWORD m_lastTime;
	int m_cursorPosition;
	float m_timer;
	bool m_cursorVisible;
	D3DXVECTOR2 m_inputNamePosition;
};