#pragma once

#include "..\\..\\GameBase.h"
#include "..\\..\\Scene\\Scene\\Scene.h"
#include "..\\..\\GameData.h"
#include "..\\Fade\\Fade.h"
#include <fstream>
#include "..\\json.hpp" 

class MenuManager
{
public:
	void Initialize(Engine* pEngine);
	void Release(Engine * pEngine);
	bool Update(Engine * pEngine, Common::CommonData& gameData, const float deltaTime);
	void Draw(Engine* pEngine, Common::CommonData & gameData);
protected:
	enum BUTTON_KIND
	{
		START_BUTTON,
		HOST_BUTTON,       
		FIND_BUTTON,       
		BACK_BUTTON,
		TITLE_BUTTON,
		RESTART_BUTTON,
		OPERATION_BUTTON,
		EXIT_BUTTON,
		MAX_BUTTON,
	};

	std::vector<int> m_buttons;
	int m_nowSceneNumber;
	int m_selectNumber;
private:
	std::string MenuManager::ConvertUTF8ToSJIS(const std::string& utf8);
	void LoadParameter();
	void UpdateKeyFlag(Engine * pEngine);
	void UpdateMenuKey(Engine * pEngine, Common::CommonData& gameData);
	void UpdateMenuCursor(Engine * pEngine, Common::CommonData& gameData);
	void UpdateOperation(Engine * pEngine);
	void UpdateAlpha();
	void Pressed(Common::CommonData& gameData);

	enum KEY_FLAG
	{
		UP_KEY = 1 << 0,
		DOWN_KEY = 1 << 1,
		DECIDE_KEY = 1 << 2,
		BACK_KEY = 1 << 3,
		MOUSE_CLICK = 1 << 4,
	};

	int f_buttonSpaceY;
	int f_buttonOffsetY;
	int f_textOffsetY;
	int f_pauseTextureAlpha;
	float f_maxAlpha;
	float f_minAlpha;
	float f_changeAlphaValue;
	std::string f_titleText;
	std::string f_clearText;
	std::string f_gameOverText;
	std::string f_pauseText;

	IntVector2 f_timeTextPosition;
	IntVector2 f_timeValuePosition;
	IntVector2 f_alertTextPosition;
	IntVector2 f_alertValuePosition;
	IntVector2 f_titlePosition;
	IntVector2 f_clearPosition;
	IntVector2 f_gameOverPosition;
	IntVector2 f_pausePosition;
	IntVector2 f_operationButtonPosition;
	IntVector2 f_buttonSize;
	std::vector<std::string> f_buttonTexts;

	float m_changeAlpha;
	float m_buttonAlpha;
	bool m_bChangeScene;
	bool m_bQuickChangeScene;
	bool m_bShowOperation;
	unsigned char m_keyFlag;
	IntVector2 m_firstButtonPosition;
	POINT m_keepCursorPoint;
	Fade m_fade;
};