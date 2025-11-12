#include "SceneLobby.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;
REQUEST_MODE SceneLobby::s_requestMode = REQUEST_MODE::NONE;

SceneLobby::SceneLobby(Engine* pEngine)
	: Scene(pEngine), m_client(nullptr), m_server(nullptr), m_pressedMouseLast(false)
{
	f_buttonSize = { 300, 100 };
	f_backButtonPosition = { 50, 50 };
	f_startButtonPosition = { WINDOW_WIDTH / 2 - f_buttonSize.x / 2 , 900 };
	f_serverNameLabelPosition = { 800, 50 };
	f_serverNamePosition = { WINDOW_WIDTH / 2, 130 };
	f_memberLabelPosition = { 750, 200 };
	f_memberNamePosition = { 750, 300 };
	f_backButtonText = "　 戻る";
	f_startButtonText = "ゲーム開始";
	f_serverNameLabelText = "サーバー名";
	f_memberLabelText = "メンバー 一覧";
	f_textOffsetY = 20;
	f_memberOffsetY = 80;
	f_clientStartButtonAlpha = 150;
}

SceneLobby::~SceneLobby()
{
	Exit();
}

void SceneLobby::Start()
{
	m_pEngine->AddTexture(TEXTURE_BUTTON);
	m_pEngine->AddFont(FONT_GOTHIC60);

	m_client = ClientManager::GetInstance();
	m_server = ServerManager::GetInstance();

	m_serverName = "Unknown Server";

	if (m_server)
		m_serverName = m_server->GetServerName();
	else if (m_client)
		m_serverName = m_client->GetServerName();
	m_pressedMouseLast = false;
}

void SceneLobby::Update()
{
	// 更新処理
	if (m_server) m_server->Update();
	if (m_client) m_client->Update();

	// クライアントのみの場合（ホストではない）、サーバーから切断されたらタイトルに戻る
	// ただし、接続確立前（まだプレイヤーリストが空）は切断チェックをスキップ
	if (m_client && !m_client->IsHost())
	{
		// プレイヤーリストが1人以上いる = 接続が確立している
		auto playerNames = m_client->GetLobbyPlayerNames();
		if (!playerNames.empty() && !m_client->IsConnected())
		{
			NET_LOG("[SceneLobby] サーバーから切断されました - タイトルに戻ります");
			m_client->Disconnect();
			m_nowSceneData.Set(Common::SCENE_TITLE, false, nullptr);
			return;
		}
	}

	POINT mp = m_pEngine->GetMousePosition();
	bool mouseDown = (m_pEngine->GetMouseButtonSync(DIK_LBUTTON) != 0);
	bool clicked = mouseDown && !m_pressedMouseLast;
	m_pressedMouseLast = mouseDown;

	// タイトルへ戻るボタン
	if (clicked && PointInRect(f_backButtonPosition, f_buttonSize))
	{
		if (m_client) m_client->Disconnect();
		if (m_server) m_server->StopServer();
		m_nowSceneData.Set(Common::SCENE_TITLE, false, nullptr);
		return;
	}

	// ゲーム開始ボタン（ホストのみ有効）
	if (m_server)
	{
		if (clicked && PointInRect(f_startButtonPosition, f_buttonSize))
		{
			m_server->StartGame();
		}
	}

	// クライアントがゲーム開始を受信したら遷移
	if (m_client && m_client->IsGameStarted())
	{
		m_nowSceneData.Set(Common::SCENE_GAME, false, nullptr);
	}
}

void SceneLobby::Draw()
{
	m_pEngine->SpriteBegin();

	RECT src, dst;
	SetRect(&src, 0, 0, f_buttonSize.x, f_buttonSize.y);

	SetRect(&dst, f_backButtonPosition.x, f_backButtonPosition.y, f_backButtonPosition.x + f_buttonSize.x, f_backButtonPosition.y + f_buttonSize.y);
	m_pEngine->Blt(&dst, TEXTURE_BUTTON, &src);
	m_pEngine->DrawPrintf(f_backButtonPosition.x, f_backButtonPosition.y + f_textOffsetY, FONT_GOTHIC60, Color::BLACK, f_backButtonText);

	m_pEngine->DrawPrintf(f_serverNameLabelPosition.x, f_serverNameLabelPosition.y, FONT_GOTHIC60, Color::WHITE, f_serverNameLabelText);
	m_pEngine->DrawPrintfCenter(f_serverNamePosition.x, f_serverNamePosition.y, FONT_GOTHIC60, Color::WHITE, m_serverName);

	std::vector<std::string> members;
	if (m_client)
		members = m_client->GetLobbyPlayerNames();
	else if (m_server)
		members = m_server->GetLobbyPlayerNames();

	m_pEngine->DrawPrintf(f_memberLabelPosition.x, f_memberLabelPosition.y + f_textOffsetY, FONT_GOTHIC60, Color::WHITE, f_memberLabelText);
	for (size_t i = 0; i < members.size(); i++)
	{
		m_pEngine->DrawPrintf(f_memberNamePosition.x, (int)i * f_memberOffsetY + f_memberNamePosition.y, FONT_GOTHIC60, Color::WHITE, members[i].c_str());
	}

	SetRect(&dst, f_startButtonPosition.x, f_startButtonPosition.y, f_startButtonPosition.x + f_buttonSize.x, f_startButtonPosition.y + f_buttonSize.y);
	m_pEngine->Blt(&dst, TEXTURE_BUTTON, &src);

	if (m_server)
		m_pEngine->DrawPrintf(f_startButtonPosition.x, f_startButtonPosition.y + f_textOffsetY, FONT_GOTHIC60, Color::BLACK, f_startButtonText);

	if (m_client) m_pEngine->Blt(&dst, TEXTURE_BUTTON, &src, f_clientStartButtonAlpha, 0);

	m_pEngine->SpriteEnd();
}

bool SceneLobby::PointInRect(IntVector2 pos, IntVector2 size)
{
	POINT point = m_pEngine->GetMousePosition();
	return (point.x >= pos.x && point.x <= pos.x + f_buttonSize.x &&
		point.y >= pos.y && point.y <= pos.y + f_buttonSize.y);
}

void SceneLobby::PostEffect()
{

}

void SceneLobby::Exit()
{
	if (m_client) m_client->Disconnect();
	m_pEngine->ReleaseTexture(TEXTURE_BUTTON);
	m_pEngine->ReleaseFont(FONT_GOTHIC60);
}

void SceneLobby::SetRequestedMode(REQUEST_MODE mode)
{
	s_requestMode = mode;
}

REQUEST_MODE SceneLobby::GetRequestMode()
{
	return s_requestMode;
}