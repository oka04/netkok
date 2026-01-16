#include "SceneLobby.h"
#include "..\\..\\Object\\Network\\NetworkLogger.h"

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
	f_connectionCheckInterval = 500;
	m_lastConnectionCheck = 0;
	m_bWasConnected = false;
}

SceneLobby::~SceneLobby()
{
	Exit();
}

void SceneLobby::Start()
{
	NET_LOG("========================================");
	NET_LOG("[SceneLobby] Start開始");
	NET_LOG("========================================");

	while (ShowCursor(TRUE) < 0);
	m_pEngine->AddTexture(TEXTURE_BUTTON);
	m_pEngine->AddFont(FONT_GOTHIC60);
	m_pEngine->AddTexture(TEXTURE_FADE);

	m_client = ClientManager::GetInstance();
	m_server = ServerManager::GetInstance();
	
	// ★★★ 最重要: ゲーム終了後のロビー復帰時、ゲーム開始フラグをリセット ★★★
	if (m_client)
	{
		m_client->ResetForLobbyReturn();
		NET_LOG_F("[SceneLobby] ResetForLobbyReturn実行完了: IsGameStarted=%s",
			m_client->IsGameStarted() ? "true" : "false");
	}

	m_gameState = IN_LOBBY;
	m_lastTime = timeGetTime();

	m_serverName = "接続中...";
	m_lastConnectionCheck = timeGetTime();
	m_bWasConnected = false;

	// ★★★ ホスト判定を正しく行う ★★★
	bool isHost = m_client->IsHost();

	if (isHost && m_server)
	{
		// ホストの場合：ServerManagerから取得
		m_serverName = m_server->GetServerName();
		NET_LOG_F("[SceneLobby] ホストとしてロビー開始: サーバー名='%s'", m_serverName.c_str());

		// ★★★ サーバー状態を待機中に設定 ★★★
		m_server->SetGameState(0);
		NET_LOG("[SceneLobby] サーバー状態を待機中(0)に設定");

		// ★★★ ロビー更新をブロードキャスト ★★★
		m_server->BroadcastLobbyUpdate();
		NET_LOG("[SceneLobby] 初回ロビー更新をブロードキャスト");
	}
	else if (m_client)
	{
		// クライアントの場合：ClientManagerから取得
		m_serverName = m_client->GetServerName();

		if (m_serverName.empty() || m_serverName == "Unknown Server")
		{
			m_serverName = "接続中...";
			NET_LOG("[SceneLobby] クライアントとしてロビー開始 - サーバー情報待機中");
		}
		else
		{
			NET_LOG_F("[SceneLobby] クライアントとしてロビー開始: サーバー名='%s'", m_serverName.c_str());
		}
	}

	m_pressedMouseLast = false;

	NET_LOG_F("[SceneLobby] Start完了: GameStarted=%s",
		m_client ? (m_client->IsGameStarted() ? "true" : "false") : "N/A");
}

void SceneLobby::Update()
{
	DWORD now = timeGetTime();
	float deltaTime = (now - m_lastTime) / 1000.0f;
	m_lastTime = now;

	if (m_server) m_server->Update();
	if (m_client) m_client->Update();

	// ★★★ デバッグ: ゲーム開始フラグの状態を定期的にログ出力 ★★★
	static DWORD lastDebugLog = 0;
	if (now - lastDebugLog > 2000 && m_client)
	{
		NET_LOG_F("[SceneLobby::Update] GameStarted=%s GameState=%d",
			m_client->IsGameStarted() ? "true" : "false",
			(int)m_gameState);
		lastDebugLog = now;
	}

	// ★★★ 接続チェック ★★★
	if (now - m_lastConnectionCheck > f_connectionCheckInterval)
	{
		m_lastConnectionCheck = now;

		if (m_client && !m_client->IsHost())
		{
			bool isConnected = m_client->IsConnected();

			if (isConnected && !m_bWasConnected)
			{
				m_bWasConnected = true;
				NET_LOG("[SceneLobby] サーバーに接続しました");
			}

			if (!isConnected && m_bWasConnected)
			{
				NET_LOG("[SceneLobby] サーバーから切断されました - タイトルに戻ります");
				m_client->Disconnect();
				m_nowSceneData.Set(Common::SCENE_TITLE, false, nullptr);
				return;
			}
		}
	}

	// ★★★ マウス入力処理 ★★★
	POINT mp = m_pEngine->GetMousePosition();
	bool mouseDown = (m_pEngine->GetMouseButtonSync(DIK_LBUTTON) != 0);
	bool clicked = mouseDown && !m_pressedMouseLast;
	m_pressedMouseLast = mouseDown;

	// 戻るボタン
	if (clicked && PointInRect(f_backButtonPosition, f_buttonSize))
	{
		NET_LOG("[SceneLobby] 戻るボタン押下");
		if (m_client) m_client->Disconnect();
		if (m_server) m_server->StopServer();
		m_nowSceneData.Set(Common::SCENE_TITLE, false, nullptr);
		return;
	}

	// ゲーム開始ボタン（ホストのみ）
	if (m_server)
	{
		if (clicked && PointInRect(f_startButtonPosition, f_buttonSize))
		{
			NET_LOG("[SceneLobby] ゲーム開始ボタン押下");
			m_server->StartGame();
		}
	}

	// ★★★ ゲーム開始チェック ★★★
	if (m_client && m_client->IsGameStarted())
	{
		NET_LOG("[SceneLobby] ゲーム開始通知受信 - ゲームシーンへ遷移");
		m_nowSceneData.Set(Common::SCENE_GAME, false, nullptr);
	}
}

void SceneLobby::Draw()
{
	m_pEngine->SpriteBegin();

	RECT src, dst;
	SetRect(&src, 0, 0, f_buttonSize.x, f_buttonSize.y);

	// 戻るボタン
	SetRect(&dst, f_backButtonPosition.x, f_backButtonPosition.y,
		f_backButtonPosition.x + f_buttonSize.x, f_backButtonPosition.y + f_buttonSize.y);
	m_pEngine->Blt(&dst, TEXTURE_BUTTON, &src);
	m_pEngine->DrawPrintf(f_backButtonPosition.x, f_backButtonPosition.y + f_textOffsetY,
		FONT_GOTHIC60, Color::BLACK, f_backButtonText);

	// サーバー名の表示
	std::string displayServerName = "接続中...";

	if (m_client && m_client->IsHost() && m_server)
	{
		// ホストの場合：ServerManagerから取得
		displayServerName = m_server->GetServerName();
	}
	else if (m_client)
	{
		// クライアントの場合：ClientManagerから取得
		std::string clientServerName = m_client->GetServerName();
		if (!clientServerName.empty() &&
			clientServerName != "Unknown Server" &&
			clientServerName != "接続中...")
		{
			displayServerName = clientServerName;
		}
	}

	m_pEngine->DrawPrintf(f_serverNameLabelPosition.x, f_serverNameLabelPosition.y,
		FONT_GOTHIC60, Color::WHITE, f_serverNameLabelText);
	m_pEngine->DrawPrintfCenter(f_serverNamePosition.x, f_serverNamePosition.y,
		FONT_GOTHIC60, Color::WHITE, displayServerName.c_str());

	// メンバー一覧
	std::vector<std::string> members;
	if (m_client)
		members = m_client->GetLobbyPlayerNames();
	else if (m_server)
		members = m_server->GetLobbyPlayerNames();

	m_pEngine->DrawPrintf(f_memberLabelPosition.x, f_memberLabelPosition.y + f_textOffsetY,
		FONT_GOTHIC60, Color::WHITE, f_memberLabelText);

	if (members.empty())
	{
		m_pEngine->DrawPrintf(f_memberNamePosition.x, f_memberNamePosition.y,
			FONT_GOTHIC60, Color::GRAY, "読み込み中...");
	}
	else
	{
		for (size_t i = 0; i < members.size(); i++)
		{
			m_pEngine->DrawPrintf(f_memberNamePosition.x,
				(int)i * f_memberOffsetY + f_memberNamePosition.y,
				FONT_GOTHIC60, Color::WHITE, members[i].c_str());
		}
	}

	if (m_client->IsHost()) {
		// ゲーム開始ボタン
		SetRect(&dst, f_startButtonPosition.x, f_startButtonPosition.y,
			f_startButtonPosition.x + f_buttonSize.x, f_startButtonPosition.y + f_buttonSize.y);
		m_pEngine->Blt(&dst, TEXTURE_BUTTON, &src);

		if (m_server)
			m_pEngine->DrawPrintf(f_startButtonPosition.x, f_startButtonPosition.y + f_textOffsetY,
				FONT_GOTHIC60, Color::BLACK, f_startButtonText);
		else if (m_client)
			m_pEngine->Blt(&dst, TEXTURE_BUTTON, &src, f_clientStartButtonAlpha, 0);

	}
	
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
	NET_LOG("[SceneLobby] Exit開始");

	m_pEngine->ReleaseTexture(TEXTURE_BUTTON);
	m_pEngine->ReleaseTexture(TEXTURE_FADE);
	m_pEngine->ReleaseFont(FONT_GOTHIC60);

	NET_LOG("[SceneLobby] Exit完了");
}

void SceneLobby::SetRequestedMode(REQUEST_MODE mode)
{
	s_requestMode = mode;
}

REQUEST_MODE SceneLobby::GetRequestMode()
{
	return s_requestMode;
}