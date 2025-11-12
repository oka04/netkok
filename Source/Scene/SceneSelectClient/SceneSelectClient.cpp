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
	m_scrollOffset = 0;
	m_maxVisibleServers = 10; // 一度に表示できる最大サーバー数
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
	m_pEngine->AddTexture(TEXTURE_BUTTON);

	NET_LOG("[SceneSelectClient] 初回サーバー検索開始");
	RefreshServerList();
	m_lastRefreshTime = timeGetTime();

	NET_LOG_F("[SceneSelectClient] 初期サーバー数: 待機中=%d ゲーム中=%d",
		(int)m_waitingServers.size(), (int)m_ingameServers.size());
}

void SceneSelectClient::Update()
{
	DWORD now = timeGetTime();
	if (now - m_lastRefreshTime > m_refreshInterval)
	{
		NET_LOG_F("[SceneSelectClient] サーバーリスト更新 (経過時間: %d ms)", now - m_lastRefreshTime);
		RefreshServerList();
		m_lastRefreshTime = now;
	}

	POINT mp = m_pEngine->GetMousePosition();
	bool mouseDown = (m_pEngine->GetMouseButtonSync(DIK_LBUTTON) != 0);
	bool clicked = mouseDown && !m_bMouseDownLast;
	m_bMouseDownLast = mouseDown;

	// スクロール処理（マウスホイール）
	int totalServers = (int)m_waitingServers.size() + (int)m_ingameServers.size();
	if (totalServers > m_maxVisibleServers)
	{
		// 上スクロール
		if (m_pEngine->GetKeyStateSync(DIK_UP) || m_pEngine->GetKeyStateSync(DIK_W))
		{
			if (m_scrollOffset > 0) m_scrollOffset--;
		}
		// 下スクロール
		if (m_pEngine->GetKeyStateSync(DIK_DOWN) || m_pEngine->GetKeyStateSync(DIK_S))
		{
			int maxScroll = totalServers - m_maxVisibleServers;
			if (m_scrollOffset < maxScroll) m_scrollOffset++;
		}
	}

	// サーバー選択処理
	int startY = 200;
	int serverHeight = 60;
	int displayIndex = 0;

	// 待機中のサーバーをチェック
	for (int i = 0; i < (int)m_waitingServers.size(); ++i)
	{
		int actualIndex = displayIndex - m_scrollOffset;
		if (actualIndex >= 0 && actualIndex < m_maxVisibleServers)
		{
			int y = startY + actualIndex * serverHeight;
			if (clicked && PointInRect(200, y, 800, 50, mp))
			{
				selectedIndex = displayIndex;
				auto& info = m_waitingServers[i];

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
		displayIndex++;
	}

	// ゲーム中のサーバーをチェック（クリックは無効）
	for (int i = 0; i < (int)m_ingameServers.size(); ++i)
	{
		int actualIndex = displayIndex - m_scrollOffset;
		if (actualIndex >= 0 && actualIndex < m_maxVisibleServers)
		{
			int y = startY + actualIndex * serverHeight;
			if (clicked && PointInRect(200, y, 800, 50, mp))
			{
				selectedIndex = displayIndex; // 選択はできるが接続はしない
				NET_LOG("[SceneSelectClient] ゲーム中のサーバーは選択できません");
			}
		}
		displayIndex++;
	}

	if (m_pEngine->GetKeyStateSync(DIK_ESCAPE))
	{
		NET_LOG("[SceneSelectClient] ESCでタイトルに戻る");

		// ★★★ クライアントをリセット ★★★
		ClientManager::GetInstance()->Reset();

		m_nowSceneData.Set(Common::SCENE_TITLE, false, nullptr);
	}
}

void SceneSelectClient::Draw()
{
	m_pEngine->SpriteBegin();

	m_pEngine->DrawPrintf(100, 100, FONT_GOTHIC60, Color::WHITE, "参加するサーバーを選択:");

	int totalServers = (int)m_waitingServers.size() + (int)m_ingameServers.size();

	if (totalServers == 0)
	{
		m_pEngine->DrawPrintf(200, 200, FONT_GOTHIC60, Color::GRAY,
			"サーバーが見つかりません...");
		m_pEngine->DrawPrintf(200, 280, FONT_GOTHIC60, Color::GRAY,
			"検索中... (ESCで戻る)");
	}
	else
	{
		int startY = 200;
		int serverHeight = 60;
		int displayIndex = 0;
		RECT src, dst;

		// 待機中のサーバーを描画
		for (int i = 0; i < (int)m_waitingServers.size(); ++i)
		{
			int actualIndex = displayIndex - m_scrollOffset;
			if (actualIndex >= 0 && actualIndex < m_maxVisibleServers)
			{
				int y = startY + actualIndex * serverHeight;

				// 選択中の背景
				if (selectedIndex == displayIndex)
				{
					SetRect(&src, 0, 0, 800, 50);
					SetRect(&dst, 200, y, 1000, y + 50);
					m_pEngine->Blt(&dst, TEXTURE_BUTTON, &src, 100, 0);
				}

				D3DCOLOR color = (selectedIndex == displayIndex) ? Color::YELLOW : Color::CYAN;

				m_pEngine->DrawPrintf(220, y, FONT_GOTHIC60, color,
					"%s (%d/%d)",
					m_waitingServers[i].name.c_str(),
					m_waitingServers[i].playerCount,
					m_waitingServers[i].maxPlayers);
			}
			displayIndex++;
		}

		// ゲーム中のサーバーを描画
		for (int i = 0; i < (int)m_ingameServers.size(); ++i)
		{
			int actualIndex = displayIndex - m_scrollOffset;
			if (actualIndex >= 0 && actualIndex < m_maxVisibleServers)
			{
				int y = startY + actualIndex * serverHeight;

				// 選択中の背景（薄く）
				if (selectedIndex == displayIndex)
				{
					SetRect(&src, 0, 0, 800, 50);
					SetRect(&dst, 200, y, 1000, y + 50);
					m_pEngine->Blt(&dst, TEXTURE_BUTTON, &src, 80, 0);
				}

				// サーバー名（灰色）
				m_pEngine->DrawPrintf(220, y, FONT_GOTHIC60, Color::GRAY,
					"%s (%d/%d)",
					m_ingameServers[i].name.c_str(),
					m_ingameServers[i].playerCount,
					m_ingameServers[i].maxPlayers);

				// 「ゲーム中」表示（赤色）
				m_pEngine->DrawPrintf(700, y, FONT_GOTHIC60, Color::RED, "ゲーム中");
			}
			displayIndex++;
		}

		// スクロールインジケーター
		if (totalServers > m_maxVisibleServers)
		{
			m_pEngine->DrawPrintf(1100, 200, FONT_GOTHIC60, Color::WHITE,
				"↑↓ スクロール");
			m_pEngine->DrawPrintf(1100, 260, FONT_GOTHIC60, Color::GRAY,
				"%d/%d", m_scrollOffset + 1, totalServers - m_maxVisibleServers + 1);
		}
	}

	m_pEngine->DrawPrintf(100, WINDOW_HEIGHT - 150, FONT_GOTHIC60, Color::WHITE,
		"見つかったサーバー: 待機中=%d ゲーム中=%d",
		(int)m_waitingServers.size(), (int)m_ingameServers.size());

	m_pEngine->DrawPrintf(100, WINDOW_HEIGHT - 100, FONT_GOTHIC60, Color::GRAY,
		"※灰色のサーバーはゲーム中のため参加できません");

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
	m_pEngine->ReleaseTexture(TEXTURE_BUTTON);
}

bool SceneSelectClient::PointInRect(int x, int y, int w, int h, POINT pt)
{
	return (pt.x >= x && pt.x <= x + w && pt.y >= y && pt.y <= y + h);
}

void SceneSelectClient::RefreshServerList()
{
	ClientManager::GetInstance()->RefreshAvailableServers();

	// 全サーバーを取得（state関係なく）
	auto allServers = ClientManager::GetInstance()->GetAllServers();

	m_waitingServers.clear();
	m_ingameServers.clear();

	for (const auto& server : allServers)
	{
		if (server.state == 0)
		{
			m_waitingServers.push_back(server);
		}
		else
		{
			m_ingameServers.push_back(server);
		}
	}

	NET_LOG_F("[SceneSelectClient] リスト更新: 待機中=%d ゲーム中=%d",
		(int)m_waitingServers.size(), (int)m_ingameServers.size());
}