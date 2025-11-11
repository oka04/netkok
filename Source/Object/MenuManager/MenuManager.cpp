#define _USING_V110_SDK71_ 1

#include "MenuManager.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;

using namespace std;

void MenuManager::Initialize(Engine * pEngine)
{
	pEngine->AddTexture(TEXTURE_PAUSE);
	pEngine->AddTexture(TEXTURE_BUTTON);
	pEngine->AddTexture(TEXTURE_OPERATION);
	pEngine->AddFont(FONT_GOTHIC60);
	pEngine->AddFont(FONT_GOTHIC160);

	m_fade.Initialize(pEngine);

	while (ShowCursor(TRUE) < 0);

	LoadParameter();
	m_selectNumber = 0;
	m_buttonAlpha = f_minAlpha;
	m_bChangeScene = false;
	m_bQuickChangeScene = false;
	m_bShowOperation = false;
	m_firstButtonPosition.x = WINDOW_WIDTH / 2 - f_buttonSize.x / 2;
	m_firstButtonPosition.y = (WINDOW_HEIGHT - f_buttonOffsetY) - (f_buttonSize.y * (int)m_buttons.size() + f_buttonSpaceY * ((int)m_buttons.size() - 1)) / 2;

	m_keepCursorPoint = pEngine->GetMousePosition();
}

void MenuManager::Release(Engine * pEngine)
{
	pEngine->ReleaseTexture(TEXTURE_PAUSE);
	pEngine->ReleaseTexture(TEXTURE_BUTTON);
	pEngine->ReleaseTexture(TEXTURE_OPERATION);
	pEngine->ReleaseFont(FONT_GOTHIC60);
	pEngine->ReleaseFont(FONT_GOTHIC160);

	if (m_nowSceneNumber != Common::SCENE_PAUSE) m_fade.Release(pEngine);
}

bool MenuManager::Update(Engine * pEngine, Common::CommonData& gameData, const float deltaTime)
{
	if (m_bChangeScene)
	{
		if (!m_bQuickChangeScene) {
			if (m_fade.Update(deltaTime)) return true;
		}
		else {
			return true;
		}
		return false;
	}

	UpdateAlpha();
	UpdateKeyFlag(pEngine);

	if (!m_bShowOperation)
	{
		UpdateMenuKey(pEngine, gameData);
		UpdateMenuCursor(pEngine, gameData);
	}
	else
	{
		UpdateOperation(pEngine);
	}
	return false;
}

void MenuManager::Draw(Engine * pEngine, Common::CommonData & gameData)
{
	RECT sour, dest;

	if (m_nowSceneNumber == Common::SCENE_PAUSE)
	{
		SetRect(&sour, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
		pEngine->Blt(&sour, TEXTURE_PAUSE, &sour, f_pauseTextureAlpha, 0.0f);
	}

	SetRect(&sour, 0, 0, f_buttonSize.x, f_buttonSize.y);

	dest.left = m_firstButtonPosition.x;
	dest.right = m_firstButtonPosition.x + f_buttonSize.x;

	for (int i = 0; i < (int)m_buttons.size(); i++)
	{
		dest.top = m_firstButtonPosition.y + i * (f_buttonSize.y + f_buttonSpaceY);
		dest.bottom = dest.top + f_buttonSize.y;

		int alpha;
		if (m_selectNumber == i) alpha = (int)m_buttonAlpha;
		else alpha = (int)f_maxAlpha;

		pEngine->Blt(&dest, TEXTURE_BUTTON, &sour, alpha, 0.0f);

		int index = m_buttons[i];
		pEngine->DrawPrintf((int)dest.left, (int)dest.top + f_textOffsetY, FONT_GOTHIC60, Color::BLACK, f_buttonTexts[index].c_str());
	}

	switch (m_nowSceneNumber)
	{
	case Common::SCENE_TITLE:
		pEngine->DrawPrintf(f_titlePosition.x, f_titlePosition.y, FONT_GOTHIC160, Color::WHITE, f_titleText.c_str());
		break;

	case Common::SCENE_PAUSE:
		pEngine->DrawPrintf(f_pausePosition.x, f_pausePosition.y, FONT_GOTHIC160, Color::WHITE, f_pauseText.c_str());
		break;

	case Common::SCENE_CLEAR:
		pEngine->DrawPrintf(f_clearPosition.x, f_clearPosition.y, FONT_GOTHIC160, Color::WHITE, f_clearText.c_str());
		break;

	case Common::SCENE_GAMEOVER:
		pEngine->DrawPrintf(f_gameOverPosition.x, f_gameOverPosition.y, FONT_GOTHIC160, Color::WHITE, f_gameOverText.c_str());
		break;
	}

	if (m_nowSceneNumber == Common::SCENE_CLEAR || m_nowSceneNumber == Common::SCENE_GAMEOVER)
	{
		pEngine->DrawPrintf(f_timeTextPosition.x, f_timeTextPosition.y, FONT_GOTHIC60, Color::WHITE, "  ゲーム時間");
		if (gameData.m_gameTime >= 60.0f)
		{
			pEngine->DrawPrintf(f_timeValuePosition.x, f_timeValuePosition.y, FONT_GOTHIC60, Color::WHITE, "  %4d分 %2d秒", (int)(gameData.m_gameTime / 60.0f), (int)gameData.m_gameTime % 60);
		}
		else
		{
			pEngine->DrawPrintf(f_timeValuePosition.x, f_timeValuePosition.y, FONT_GOTHIC60, Color::WHITE, "     %2d秒", (int)gameData.m_gameTime);
		}

		pEngine->DrawPrintf(f_alertTextPosition.x, f_alertTextPosition.y, FONT_GOTHIC60, Color::WHITE, "見つかった回数");
		pEngine->DrawPrintf(f_alertValuePosition.x, f_alertValuePosition.y, FONT_GOTHIC60, Color::WHITE, "        %3d回", gameData.m_alertCount);
	}

	if (m_bShowOperation)
	{
		SetRect(&sour, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
		pEngine->Blt(&sour, TEXTURE_OPERATION, &sour);

		SetRect(&sour, 0, 0, f_buttonSize.x, f_buttonSize.y);
		SetRect(&dest, f_operationButtonPosition.x, f_operationButtonPosition.y, f_operationButtonPosition.x + f_buttonSize.x, f_operationButtonPosition.y + f_buttonSize.y);

		pEngine->Blt(&dest, TEXTURE_BUTTON, &sour, (int)m_buttonAlpha, 0.0f);
		pEngine->DrawPrintf((int)dest.left, (int)dest.top + f_textOffsetY, FONT_GOTHIC60, Color::BLACK, f_buttonTexts[BACK_BUTTON].c_str());
	}

	if (m_bChangeScene)
	{
		m_fade.Draw(pEngine);
	}
}

void MenuManager::LoadParameter()
{
	ifstream file(JSON_MENU_PARAMETER);
	if (!file.is_open())
	{
		throw DxSystemException(DxSystemException::OM_FILE_OPEN_ERROR);
	}

	nlohmann::json config;
	file >> config;
	file.close();

	f_buttonSpaceY = config["buttonSpaceY"];
	f_buttonOffsetY = config["buttonOffsetY"];
	f_pauseTextureAlpha = config["pauseTextureAlpha"];
	f_maxAlpha = config["maxAlpha"];
	f_minAlpha = config["minAlpha"];
	f_changeAlphaValue = config["changeAlphaValue"];
	f_textOffsetY = config["textOffsetY"];
	f_titleText = config["titleText"];
	f_clearText = config["clearText"];
	f_gameOverText = config["gameOverText"];
	f_pauseText = config["pauseText"];

	for (int i = 0; i < 2; i++)
	{
		f_timeTextPosition[i] = config["timeTextPosition"][i];
		f_timeValuePosition[i] = config["timeValuePosition"][i];
		f_alertTextPosition[i] = config["alertTextPosition"][i];
		f_alertValuePosition[i] = config["alertValuePosition"][i];
		f_titlePosition[i] = config["titlePosition"][i];
		f_clearPosition[i] = config["clearPosition"][i];
		f_gameOverPosition[i] = config["gameOverPosition"][i];
		f_pausePosition[i] = config["pausePosition"][i];
		f_operationButtonPosition[i] = config["operationButtonPosition"][i];
		f_buttonSize[i] = config["buttonSize"][i];
	};

	for (int i = 0; i < MAX_BUTTON; i++)
	{
		std::string utf8Text = config["buttonTexts"][i];  
		std::string sjisText = ConvertUTF8ToSJIS(utf8Text); 
		f_buttonTexts.push_back(sjisText);
	}
}

//UTF-8 → Shift-JIS　に変換して日本語対応できるようにする
std::string MenuManager::ConvertUTF8ToSJIS(const std::string& utf8)
{
	int wsize = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
	std::wstring wstr(wsize, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], wsize);

	int size = WideCharToMultiByte(932, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string sjis(size, 0);
	WideCharToMultiByte(932, 0, wstr.c_str(), -1, &sjis[0], size, nullptr, nullptr);

	return sjis;
}

void MenuManager::UpdateKeyFlag(Engine * pEngine)
{
	if (pEngine->GetKeyStateSync(DIK_UP))
	{
		m_keyFlag |= UP_KEY;
	}
	else
	{
		m_keyFlag &= ~UP_KEY;
	}

	if (pEngine->GetKeyStateSync(DIK_DOWN))
	{
		m_keyFlag |= DOWN_KEY;
	}
	else
	{
		m_keyFlag &= ~DOWN_KEY;
	}

	if (pEngine->GetKeyStateSync(DIK_RETURN))
	{
		m_keyFlag |= DECIDE_KEY;
	}
	else
	{
		m_keyFlag &= ~DECIDE_KEY;
	}

	if (pEngine->GetKeyStateSync(DIK_ESCAPE))
	{
		m_keyFlag |= BACK_KEY;
	}
	else
	{
		m_keyFlag &= ~BACK_KEY;
	}

	if (pEngine->GetMouseButtonSync(DIK_LBUTTON))
	{
		m_keyFlag |= MOUSE_CLICK;
	}
	else
	{
		m_keyFlag &= ~MOUSE_CLICK;
	}
}

void MenuManager::UpdateMenuKey(Engine * pEngine, Common::CommonData& gameData)
{
	if (m_keyFlag & UP_KEY && m_selectNumber > 0)
	{
		m_selectNumber--;
		m_buttonAlpha = f_minAlpha;
		SoundManager::Play(AK::EVENTS::PLAY_SE_SELECT, ID_UI);
	}

	if (m_keyFlag & DOWN_KEY && m_selectNumber < (int)m_buttons.size() - 1)
	{
		m_selectNumber++;
		m_buttonAlpha = f_minAlpha;
		SoundManager::Play(AK::EVENTS::PLAY_SE_SELECT, ID_UI);
	}

	if (m_keyFlag & DECIDE_KEY)
	{
		Pressed(gameData);
		SoundManager::Play(AK::EVENTS::PLAY_SE_DECIDE, ID_UI);
	}

	if (m_keyFlag & BACK_KEY && m_nowSceneNumber == Common::SCENE_PAUSE)
	{
		m_selectNumber = 0;
		Pressed(gameData);
		SoundManager::Play(AK::EVENTS::PLAY_SE_BACK, ID_UI);
	}
}

void MenuManager::UpdateMenuCursor(Engine * pEngine, Common::CommonData& gameData)
{
	POINT point = pEngine->GetMousePosition();

	//マウスが動いていなかったりボタンが押されていない場合は判定しない
	if (point.x == m_keepCursorPoint.x && point.y == m_keepCursorPoint.y && !(m_keyFlag & MOUSE_CLICK)) return;
	m_keepCursorPoint = point;

	for (int i = 0; i < (int)m_buttons.size(); i++)
	{
		int buttonY = m_firstButtonPosition.y + i * (f_buttonSize.y + f_buttonSpaceY);

		if (point.x >= m_firstButtonPosition.x && point.x <= m_firstButtonPosition.x + f_buttonSize.x
			&& point.y >= buttonY && point.y <= buttonY + f_buttonSize.y)
		{
			//触ったボタンが同じ場合はアルファ値の変更をしない
			if (m_selectNumber != i)
			{
				m_selectNumber = i;
				m_buttonAlpha = f_minAlpha;
				SoundManager::Play(AK::EVENTS::PLAY_SE_SELECT, ID_UI);
			}

			if (m_keyFlag & MOUSE_CLICK)
			{
				Pressed(gameData);
				SoundManager::Play(AK::EVENTS::PLAY_SE_DECIDE, ID_UI);
			}
			break;//ボタンの上にあった場合はそれ以上見ない
		}
	}
}

void MenuManager::UpdateOperation(Engine * pEngine)
{
	//操作説明のボタンの場所などが特殊なため別個で処理する
	if (m_keyFlag & DECIDE_KEY || m_keyFlag & BACK_KEY)
	{
		m_bShowOperation = false;
		m_selectNumber = 0;
		m_buttonAlpha = f_minAlpha;
		SoundManager::Play(AK::EVENTS::PLAY_SE_BACK, ID_UI);
	}

	POINT point = pEngine->GetMousePosition();

	if (point.x == m_keepCursorPoint.x && point.y == m_keepCursorPoint.y && !(m_keyFlag & MOUSE_CLICK)) return;
	m_keepCursorPoint = point;

	if (point.x >= f_operationButtonPosition.x && point.x <= f_operationButtonPosition.x + f_buttonSize.x
		&& point.y >= f_operationButtonPosition.y && point.y <= f_operationButtonPosition.y + f_buttonSize.y)
	{
		if (m_keyFlag & MOUSE_CLICK)
		{
			m_bShowOperation = false;
			m_selectNumber = 0;
			m_buttonAlpha = f_minAlpha;
			SoundManager::Play(AK::EVENTS::PLAY_SE_BACK, ID_UI);
		}
	}
}

void MenuManager::UpdateAlpha()
{
	if (m_buttonAlpha >= f_maxAlpha)
	{
		m_changeAlpha = -f_changeAlphaValue;
	}
	else if (m_buttonAlpha <= f_minAlpha)
	{
		m_changeAlpha = f_changeAlphaValue;
	}
	m_buttonAlpha += m_changeAlpha;

}
// ボタン押下処理（HOST/FIND 追加対応）
void MenuManager::Pressed(Common::CommonData& gameData)
{
	switch (m_buttons[m_selectNumber])
	{
	case HOST_BUTTON:
		// タイトル → ロビーへ遷移。Scene 側で実際に Server/Client を起動する想定
		m_bQuickChangeScene = true;
		gameData.m_nextSceneNumber = Common::SCENE_LOBBY;
		break;
	case FIND_BUTTON:
		// Find（サーバーを探す）→ ロビー（ブラウザ）へ遷移
		m_bQuickChangeScene = true;
		gameData.m_nextSceneNumber = Common::SCENE_LOBBY;
		break;
	case BACK_BUTTON:
		m_bQuickChangeScene = true;
	case START_BUTTON:
		gameData.m_nextSceneNumber = Common::SCENE_GAME;
		break;
	case TITLE_BUTTON:
		gameData.m_nextSceneNumber = Common::SCENE_TITLE;
		break;
	case RESTART_BUTTON:
		gameData.m_nextSceneNumber = Common::RESTART;
		break;
	case EXIT_BUTTON:
		m_bQuickChangeScene = true;
		gameData.m_nextSceneNumber = Common::SCENE_EXIT;
		break;
	case OPERATION_BUTTON:
		m_bShowOperation = true;
		m_buttonAlpha = f_minAlpha;
		return;
	}
	m_fade.SetFadeOut();
	m_bChangeScene = true;

	m_buttonAlpha = f_minAlpha;
}