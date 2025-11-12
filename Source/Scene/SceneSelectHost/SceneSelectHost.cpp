#define _USING_V110_SDK71_ 1

#include "SceneSelectHost.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;

SceneSelectHost::SceneSelectHost(Engine* pEngine)
	: Scene(pEngine)
{
}

SceneSelectHost::~SceneSelectHost()
{
	Exit();
}

void SceneSelectHost::Start()
{
	m_pEngine->AddFont(FONT_GOTHIC60);
	m_pEngine->AddTexture(TEXTURE_BUTTON);

	f_maxNameCcount = 30;
	f_buttonSize = { 300, 100 };
	f_textOffsetY = 20;

	f_backButtonPosition = { 50, 50 };
	f_serverNameLabelPosition = { 700, 200 };

	f_serverNameInputPosition = { WINDOW_WIDTH / 2, 300 };

	f_enterButtonPosition = { WINDOW_WIDTH / 2 - f_buttonSize.x / 2, 900 };
	f_errorNameTextPosition = { 300, 700 };

	f_serverNameLabelText = "サーバー名を入力";

	f_emptyNameText = "(入力してください)";
	f_errorNamesText = "サーバー名を入力してください";

	f_backButtonText = "   戻る  ";
	f_enterButtonText = " 作成する ";
	m_lastTime = timeGetTime();
	f_maxAlpha = 200;
	f_errorNameViewTime = 5000;
	f_changeAlphaValue = 1.5;
	f_minAlpha = 100;
	f_clickMargin = 50;

	m_serverName = "";

	// タイトル画面から入力されたプレイヤー名を取得
	ClientManager* client = ClientManager::GetInstance();
	m_playerName = client->GetPlayerName();

	m_errorNameStartTime = 0;
	m_bErrorName = false;
	m_bMouseOverBackButton = false;
	m_bMouseOverEnterButton = false;
	m_buttonAlpha = f_maxAlpha;
	m_cursorPosition = 0;
	m_timer = 0.0f;
	m_cursorVisible = true;
}

void SceneSelectHost::Update()
{
	DWORD nowTime = timeGetTime();
	m_deltaTime = (nowTime - m_lastTime) / 1000.0f;
	m_lastTime = nowTime;

	const float BLINK_INTERVAL = 0.5f;
	m_timer += m_deltaTime;

	if (f_errorNameViewTime <= nowTime - m_errorNameStartTime) m_bErrorName = false;

	if (m_timer >= BLINK_INTERVAL)
	{
		m_cursorVisible = !m_cursorVisible;
		m_timer = 0.0f;
	}

	UpdateButton();

	if (GetAsyncKeyState(VK_LEFT) & 0x0001)
	{
		if (m_cursorPosition > 0)
		{
			m_cursorPosition--;
			m_cursorVisible = true;
			m_timer = 0.0f;
		}
	}

	if (GetAsyncKeyState(VK_RIGHT) & 0x0001)
	{
		if (m_cursorPosition < (int)m_serverName.size())
		{
			m_cursorPosition++;
			m_cursorVisible = true;
			m_timer = 0.0f;
		}
	}

	//文字入力（A～Z）
	if ((int)m_serverName.size() < f_maxNameCcount)
	{
		for (int key = 'A'; key <= 'Z'; key++)
		{
			if (GetAsyncKeyState(key) & 0x0001)
			{
				m_serverName.insert(m_cursorPosition, 1, static_cast<char>(key));
				m_cursorPosition++;
				m_cursorVisible = true;
				m_timer = 0.0f;

				if (f_maxNameCcount < (int)m_serverName.size())
				{
					m_serverName.pop_back();
					m_cursorPosition--;
				}
				break;
			}
		}
	}

	//削除系
	if (GetAsyncKeyState(VK_BACK) & 0x0001 && m_cursorPosition > 0)
	{
		m_serverName.erase(m_cursorPosition - 1, 1);
		m_cursorPosition--;
		m_cursorVisible = true;
		m_timer = 0.0f;
	}

	if (GetAsyncKeyState(VK_DELETE) & 0x0001 && m_cursorPosition < (int)m_serverName.size())
	{
		m_serverName.erase(m_cursorPosition, 1);
		m_cursorVisible = true;
		m_timer = 0.0f;
	}

	//マウスによるクリック入力欄切り替え
	if (GetAsyncKeyState(VK_LBUTTON) & 0x0001)
	{
		POINT point = m_pEngine->GetMousePosition();

		//サーバー名入力範囲
		int textWidth, textHeight;
		m_pEngine->GetTextSize(FONT_GOTHIC60, m_serverName.c_str(), &textWidth, &textHeight);
		int textStartX = (int)f_serverNameInputPosition.x - textWidth / 2;
		int textEndX = (int)f_serverNameInputPosition.x + textWidth / 2;
		int textStartY = (int)f_serverNameInputPosition.y;
		int textEndY = (int)f_serverNameInputPosition.y + textHeight;

		if (point.x >= textStartX - f_clickMargin && point.x <= textEndX + f_clickMargin &&
			point.y >= textStartY && point.y <= textEndY)
		{
			m_cursorPosition = (int)m_serverName.size();
			m_cursorVisible = true;
			m_timer = 0.0f;
		}

		if (m_bMouseOverBackButton)
		{
			m_buttonAlpha = f_minAlpha;

			ServerManager::GetInstance()->Reset();
			ClientManager::GetInstance()->Reset();

			m_nowSceneData.Set(Common::SCENE_TITLE, false, nullptr);
			return;
		}

		if (m_bMouseOverEnterButton)
		{
			m_buttonAlpha = f_minAlpha;
			if (CreateServer()) return;
		}
	}
}

void SceneSelectHost::Draw()
{
	m_pEngine->SpriteBegin();

	//サーバー名入力欄
	m_pEngine->DrawPrintf(f_serverNameLabelPosition.x, f_serverNameLabelPosition.y, FONT_GOTHIC60, Color::WHITE, f_serverNameLabelText);
	if (!m_serverName.empty())
		m_pEngine->DrawPrintfCenter((int)f_serverNameInputPosition.x, (int)f_serverNameInputPosition.y, FONT_GOTHIC60, Color::YELLOW, m_serverName.c_str());
	else
		m_pEngine->DrawPrintfCenter((int)f_serverNameInputPosition.x, (int)f_serverNameInputPosition.y, FONT_GOTHIC60, Color::GRAY, f_emptyNameText);

	//カーソル描画
	if (m_cursorVisible)
	{
		int textWidth, textHeight;
		m_pEngine->GetTextSize(FONT_GOTHIC60, m_serverName.c_str(), &textWidth, &textHeight);

		std::string prefix = m_serverName.substr(0, m_cursorPosition);
		int prefixWidth, prefixHeight;
		m_pEngine->GetTextSize(FONT_GOTHIC60, prefix.c_str(), &prefixWidth, &prefixHeight);

		int cursorX = (int)f_serverNameInputPosition.x - textWidth / 2 + prefixWidth;
		int cursorY = (int)f_serverNameInputPosition.y;
		m_pEngine->DrawLine(cursorX, cursorY, cursorX, cursorY + textHeight, Color::YELLOW);
	}

	//戻るボタン
	SetRect(&sour, 0, 0, f_buttonSize.x, f_buttonSize.y);
	SetRect(&dest, f_backButtonPosition.x, f_backButtonPosition.y, f_backButtonPosition.x + f_buttonSize.x, f_backButtonPosition.y + f_buttonSize.y);
	m_pEngine->Blt(&dest, TEXTURE_BUTTON, &sour, (int)((m_bMouseOverBackButton) ? m_buttonAlpha : f_maxAlpha), 0);
	m_pEngine->DrawPrintf(f_backButtonPosition.x, f_backButtonPosition.y + f_textOffsetY, FONT_GOTHIC60, Color::BLACK, f_backButtonText);

	//作成ボタン
	SetRect(&dest, f_enterButtonPosition.x, f_enterButtonPosition.y, f_enterButtonPosition.x + f_buttonSize.x, f_enterButtonPosition.y + f_buttonSize.y);
	m_pEngine->Blt(&dest, TEXTURE_BUTTON, &sour, (int)((m_bMouseOverEnterButton) ? m_buttonAlpha : f_maxAlpha), 0);
	m_pEngine->DrawPrintf(f_enterButtonPosition.x, f_enterButtonPosition.y + f_textOffsetY, FONT_GOTHIC60, Color::BLACK, f_enterButtonText);

	if (m_bErrorName)
		m_pEngine->DrawPrintf(f_errorNameTextPosition.x, f_errorNameTextPosition.y, FONT_GOTHIC60, Color::RED, f_errorNamesText);

	m_pEngine->SpriteEnd();
}

void SceneSelectHost::PostEffect()
{
}

void SceneSelectHost::Exit()
{
	m_pEngine->ReleaseFont(FONT_GOTHIC60);
	m_pEngine->ReleaseTexture(TEXTURE_BUTTON);
}

bool SceneSelectHost::IsMouseOverButton(IntVector2 buttonPos)
{
	POINT point = m_pEngine->GetMousePosition();
	return (point.x >= buttonPos.x && point.x <= buttonPos.x + f_buttonSize.x &&
		point.y >= buttonPos.y && point.y <= buttonPos.y + f_buttonSize.y);
}

void SceneSelectHost::UpdateAlpha()
{
	if (m_buttonAlpha >= f_maxAlpha)
		m_changeAlpha = -f_changeAlphaValue;
	else if (m_buttonAlpha <= f_minAlpha)
		m_changeAlpha = f_changeAlphaValue;
	m_buttonAlpha += m_changeAlpha;
}

void SceneSelectHost::UpdateButton()
{
	if (IsMouseOverButton(f_backButtonPosition))
	{
		if (!m_bMouseOverBackButton)
		{
			m_bMouseOverBackButton = true;
			m_alpha = f_minAlpha;
			m_changeAlpha = f_changeAlphaValue;
		}
		UpdateAlpha();
	}
	else m_bMouseOverBackButton = false;

	if (IsMouseOverButton(f_enterButtonPosition))
	{
		if (!m_bMouseOverEnterButton)
		{
			m_bMouseOverEnterButton = true;
			m_alpha = f_minAlpha;
			m_changeAlpha = f_changeAlphaValue;
		}
		UpdateAlpha();
	}
	else m_bMouseOverEnterButton = false;

}

bool SceneSelectHost::CreateServer()
{
	if (m_serverName.empty())
	{
		m_errorNameStartTime = timeGetTime();
		m_bErrorName = true;
		return false;
	}

	ServerManager* server = ServerManager::GetInstance();
	server->SetServerName(m_serverName);
	server->SetHostName(m_playerName); 

	const int MIN_PORT = 12000;
	const int MAX_PORT = 12999;
	const int MAX_RETRY = 50;

	bool started = false;
	int port = 0;

	for (int i = 0; i < MAX_RETRY; i++)
	{
		port = MIN_PORT + (rand() % (MAX_PORT - MIN_PORT + 1));
		if (server->StartServer(port, 8))
		{
			started = true;
			break;
		}
	}

	if (!started)
	{
		MessageBoxA(NULL, "サーバーの起動に失敗しました。\nポート確保に失敗。", "エラー", MB_OK | MB_ICONERROR);
		return false; 
	}

	SceneLobby::SetRequestedMode(REQUEST_MODE::HOST);

	ClientManager* client = ClientManager::GetInstance();
	// client->SetPlayerName(m_playerName);
	client->ConnectToServer("127.0.0.1", port);

	m_nowSceneData.Set(Common::SCENE_LOBBY, false, nullptr);
	return true;
}