#define _USING_V110_SDK71_ 1

#include "SceneSelectClient.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;

SceneSelectClient::SceneSelectClient(Engine* pEngine)
	: Scene(pEngine)
{
	selectedIndex = -1;
	m_lastRefreshTime = 0;
	m_refreshInterval = 1000;
	m_bMouseDownLast = false;
}

SceneSelectClient::~SceneSelectClient()
{
	Exit();
}

void SceneSelectClient::Start()
{
	NET_LOG("========================================");
	NET_LOG("SceneSelectClient 開始");
	NET_LOG("========================================");

	m_pEngine->AddFont(FONT_GOTHIC60);

	NET_LOG("[SceneSelectClient] 初回サーバー検索開始");
	ClientManager::GetInstance()->RefreshAvailableServers();
	m_serverList = ClientManager::GetInstance()->GetCachedServers();
	m_lastRefreshTime = timeGetTime();

	NET_LOG_F("[SceneSelectClient] 初期サーバー数: %d", (int)m_serverList.size());
}

void SceneSelectClient::Update()
{
	DWORD now = timeGetTime();
	if (now - m_lastRefreshTime > m_refreshInterval)
	{
		NET_LOG_F("[SceneSelectClient] サーバーリスト更新 (経過時間: %d ms)", now - m_lastRefreshTime);

		ClientManager::GetInstance()->RefreshAvailableServers();
		auto newList = ClientManager::GetInstance()->GetCachedServers();

		if (newList.size() != m_serverList.size())
		{
			NET_LOG_F("[SceneSelectClient] サーバー数変化: %d → %d",
				(int)m_serverList.size(), (int)newList.size());
		}

		m_serverList = newList;
		m_lastRefreshTime = now;
	}

	POINT mp = m_pEngine->GetMousePosition();
	bool mouseDown = (m_pEngine->GetMouseButtonSync(DIK_LBUTTON) != 0);

	bool clicked = mouseDown && !m_bMouseDownLast;
	m_bMouseDownLast = mouseDown;

	int startY = 200;
	for (int i = 0; i < (int)m_serverList.size(); ++i)
	{
		if (clicked && PointInRect(200, startY + i * 60, 400, 50, mp))
		{
			selectedIndex = i;
			auto& info = m_serverList[i];

			char ipStr[64];
			ENetAddress addr;
			addr.host = info.ip;
			addr.port = info.port;
			enet_address_get_host_ip(&addr, ipStr, sizeof(ipStr));

			NET_LOG_F("[SceneSelectClient] サーバー選択: %s @ %s:%d",
				info.name.c_str(), ipStr, info.port);

			ClientManager::GetInstance()->ConnectToServer(ipStr, info.port);
			SceneLobby::SetRequestedMode(REQUEST_MODE::FIND);
			m_nowSceneData.Set(Common::SCENE_LOBBY, false, nullptr);
			return;
		}
	}

	if (m_pEngine->GetKeyStateSync(DIK_ESCAPE))
	{
		NET_LOG("[SceneSelectClient] ESCでタイトルに戻る");
		m_nowSceneData.Set(Common::SCENE_TITLE, false, nullptr);
	}
}

void SceneSelectClient::Draw()
{
	m_pEngine->SpriteBegin();

	m_pEngine->DrawPrintf(100, 100, FONT_GOTHIC60, Color::WHITE, "参加するサーバーを選択:");

	if (m_serverList.empty())
	{
		m_pEngine->DrawPrintf(200, 200, FONT_GOTHIC60, Color::GRAY,
			"サーバーが見つかりません...");
		m_pEngine->DrawPrintf(200, 280, FONT_GOTHIC60, Color::GRAY,
			"検索中... (ESCで戻る)");
	}
	else
	{
		int startY = 200;
		for (int i = 0; i < (int)m_serverList.size(); ++i)
		{
			D3DCOLOR color = (selectedIndex == i) ? Color::YELLOW : Color::CYAN;

			m_pEngine->DrawPrintf(200, startY + i * 60, FONT_GOTHIC60, color,
				"%s (%d/%d)",
				m_serverList[i].name.c_str(),
				m_serverList[i].playerCount,
				m_serverList[i].maxPlayers);
		}
	}

	m_pEngine->DrawPrintf(100, WINDOW_HEIGHT - 100, FONT_GOTHIC60, Color::WHITE,
		"見つかったサーバー: %d", (int)m_serverList.size());

	m_pEngine->DrawPrintf(100, WINDOW_HEIGHT - 50, FONT_GOTHIC60, Color::GRAY,
		"※ログは network_debug.txt に出力されています");

	m_pEngine->SpriteEnd();
}

void SceneSelectClient::PostEffect()
{
}

void SceneSelectClient::Exit()
{
	NET_LOG("[SceneSelectClient] Exit");
	m_pEngine->ReleaseFont(FONT_GOTHIC60);
}

bool SceneSelectClient::PointInRect(int x, int y, int w, int h, POINT pt)
{
	return (pt.x >= x && pt.x <= x + w && pt.y >= y && pt.y <= y + h);
}