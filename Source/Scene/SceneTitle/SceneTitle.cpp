//*****************************************************************************
//
// タイトルシーン (修正版：テキスト幅キャッシュ／クリックで文字間カーソル／点滅修正)
//
//*****************************************************************************
#define _USING_V110_SDK71_ 1

#include "..\\Object\\Network\\ClientManager\\ClientManager.h"
#include "..\\Object\\Network\\ServerManager\\ServerManager.h"
#include "SceneTitle.h"
#include <cstdlib>
#include <ctime>

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;
using namespace std;

SceneTitle::SceneTitle(Engine *pEngine)
	: Scene(pEngine)
{
}

SceneTitle::~SceneTitle()
{
	Exit();
}

void SceneTitle::Start()
{
	m_buttons.clear();
	m_buttons.push_back(HOST_BUTTON);
	m_buttons.push_back(FIND_BUTTON);
	m_buttons.push_back(OPERATION_BUTTON);
	m_buttons.push_back(EXIT_BUTTON);
	m_nowSceneNumber = m_nowSceneData.GetNowScene();
	Initialize(m_pEngine);

	m_camera.m_vecEye = D3DXVECTOR3(5.0f, 10.0f, 6.0f);
	m_camera.m_vecAt = D3DXVECTOR3(30.0f, 0.0f, 17.0f);
	m_camera.m_vecUp = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	m_camera.RecalculateUpDirection();
	m_camera.SetDevice(m_pEngine);

	m_projection.SetData(D3DXToRadian(90.0f), 4.0f / 3.0f, 0.1f, 100.0f);
	m_projection.SetDevice(m_pEngine);

	m_viewport.Add(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0f, 1.0f);
	m_viewport.SetDevice(m_pEngine, 0);

	m_ambient.SetColor(0.7f, 0.1f, 0.1f, 0.1f);
	m_light.SetDiffuse(0.1f, 0.1f, 0.1f, 0.1f);
	D3DXVECTOR3 direction = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
	direction = D3DXVec3Normalize(&direction);
	m_light.SetDirection(direction);

	SetBackColor(0x00000000);
	m_pEngine->AddModel(MODEL_CHARACTER);

	m_patrollerManager.Initialize(m_pEngine, m_nowSceneData.GetNowScene());
	m_lastTime = timeGetTime();

	Camera camera = m_camera;
	m_map.Initialize(m_pEngine, m_patrollerManager, &camera, &m_projection, &m_ambient, &m_light, 0);

	m_patrollerManager.ReleaseSE();
	SoundManager::Play(AK::EVENTS::PLAY_BGM_TITLE, ID_BGM);

	srand((unsigned int)time(NULL));

	// 定数の初期化
	f_defaultPlayerNamePrefix = "Player";
	f_randomNumberDigits = 3;
	f_randomNumberMin = 0;
	f_randomNumberMax = 999;
	f_emptyNamePlaceholder = "(入力してください)";
	f_playerNameLabelText = "プレイヤー名";
	// プレイヤー名を空文字列で初期化
	m_playerName = "";
	f_clickMargin = 100;

	f_playerNameMaxCount = 12;
	m_cursorPos = 0;
	m_bInputActive = false;
	m_cursorVisible = true;
	m_playerTimer = 0.0f;

	f_playerNameLabelPos = { 1400, 50 };
	f_playerNameInputPos = { 1580, 120 };

	m_playerTextWidth = 0;
	m_playerCharCumulative.clear();
	m_playerMaxWidth = 0;
	int m_playerMaxHeight = 0;

	std::string maxFill(f_playerNameMaxCount, 'W');
	m_pEngine->GetTextSize(FONT_GOTHIC60, maxFill.c_str(), &m_playerMaxWidth, &m_playerMaxHeight);

	auto RecalcPlayerTextMetrics = [&]() {
		m_playerCharCumulative.clear();
		int totalW = 0;
		for (size_t i = 0; i < m_playerName.size(); ++i)
		{
			char cstr[2] = { m_playerName[i], '\0' };
			int cw = 0, ch = 0;
			m_pEngine->GetTextSize(FONT_GOTHIC60, cstr, &cw, &ch);
			totalW += cw;
			m_playerCharCumulative.push_back(totalW);
		}
		m_playerTextWidth = totalW;
	};
	RecalcPlayerTextMetrics();
}

void SceneTitle::Update()
{
	DWORD nowTime = timeGetTime();
	m_deltaTime = (nowTime - m_lastTime) / 1000.0f;
	m_lastTime = nowTime;

	UpdatePlayerNameInput();

	m_patrollerManager.Update(m_pEngine, m_map, m_camera.m_vecEye, nullptr, m_deltaTime);

	if (MenuManager::Update(m_pEngine, m_gameData, m_deltaTime))
	{
		int buttonKind = m_buttons[m_selectNumber];

		// プレイヤー名が空の場合、自動生成
		if (m_playerName.empty())
		{
			GenerateDefaultPlayerName();
		}

		if (buttonKind == HOST_BUTTON)
		{
			SceneLobby::SetRequestedMode(REQUEST_MODE::HOST);
			ClientManager::GetInstance()->SetPlayerName(m_playerName);
			m_nowSceneData.Set(Common::SCENE_SELECT_HOST, false, nullptr);
		}
		else if (buttonKind == FIND_BUTTON)
		{
			SceneLobby::SetRequestedMode(REQUEST_MODE::FIND);
			ClientManager::GetInstance()->SetPlayerName(m_playerName);
			m_nowSceneData.Set(Common::SCENE_SELECT_CLIENT, false, nullptr);
		}
		else
		{
			m_nowSceneData.Set(m_gameData.m_nextSceneNumber, false, nullptr);
		}
	}
}

void SceneTitle::Draw()
{
	vector<SpotLight>* lights = m_patrollerManager.GetLights(m_camera.m_vecEye, m_camera.m_vecAt, m_projection.GetFov());
	m_map.DrawMap(m_pEngine, &m_camera, &m_projection, &m_ambient, &m_light, lights);
	m_patrollerManager.Draw(m_pEngine, &m_camera, &m_projection, &m_ambient, &m_light);

	m_pEngine->SpriteBegin();
	MenuManager::Draw(m_pEngine, m_gameData);

	DrawPlayerNameInput();

#if _DEBUG	
	m_pEngine->DrawPrintf(0, 1000, FONT_GOTHIC60, Color::WHITE, "%f", (float)m_pEngine->GetFPS());
#endif
	m_pEngine->SpriteEnd();
}

void SceneTitle::PostEffect()
{
}

void SceneTitle::Exit()
{
	SoundManager::StopAll(ID_BGM);
	Release(m_pEngine);
	m_map.Release(m_pEngine);
	m_patrollerManager.Release(m_pEngine);

	m_pEngine->ReleaseModel(MODEL_CHARACTER);
}

#ifdef USE_IMGUI
void SceneTitle::ImGuiFrameProcess()
{
}
#endif

void SceneTitle::GenerateDefaultPlayerName()
{
	int randomNum = f_randomNumberMin + (rand() % (f_randomNumberMax - f_randomNumberMin + 1));
	char buffer[32];
	sprintf_s(buffer, "%s%0*d", f_defaultPlayerNamePrefix, f_randomNumberDigits, randomNum);
	m_playerName = buffer;
}

void SceneTitle::UpdatePlayerNameInput()
{
	const float BLINK_INTERVAL = 0.5f;
	m_playerTimer += m_deltaTime;
	if (m_playerTimer >= BLINK_INTERVAL)
	{
		m_cursorVisible = !m_cursorVisible;
		m_playerTimer = 0.0f;
	}

	if (GetAsyncKeyState(VK_LBUTTON) & 0x0001)
	{
		POINT p = m_pEngine->GetMousePosition();

		if (m_playerCharCumulative.size() != m_playerName.size())
		{
			m_playerCharCumulative.clear();
			int totalW = 0;
			for (size_t i = 0; i < m_playerName.size(); ++i)
			{
				char cstr[2] = { m_playerName[i], '\0' };
				int cw = 0, ch = 0;
				m_pEngine->GetTextSize(FONT_GOTHIC60, cstr, &cw, &ch);
				totalW += cw;
				m_playerCharCumulative.push_back(totalW);
			}
			m_playerTextWidth = totalW;
		}

		int textWidth = m_playerTextWidth;
		int textHeight = 0;
		if (m_playerName.empty())
		{
			int phw = 0;
			m_pEngine->GetTextSize(FONT_GOTHIC60, f_emptyNamePlaceholder, &phw, &textHeight);
		}
		else
		{
			int th = 0;
			m_pEngine->GetTextSize(FONT_GOTHIC60, m_playerName.c_str(), &textWidth, &th);
			textHeight = th;
		}

		int startX = (int)f_playerNameInputPos.x - textWidth / 2;
		int endX = (int)f_playerNameInputPos.x + textWidth / 2;

		int extendedEndX = (int)f_playerNameInputPos.x - textWidth / 2 + m_playerMaxWidth;

		if (p.x >= startX - f_clickMargin && p.x <= extendedEndX + f_clickMargin && p.y >= (int)f_playerNameInputPos.y && p.y <= (int)f_playerNameInputPos.y + textHeight)
		{
			m_bInputActive = true;
			m_cursorVisible = true;
			m_playerTimer = 0.0f;

			if (p.x >= endX)
			{
				m_cursorPos = (int)m_playerName.size();
			}
			else if (p.x <= startX)
			{
				m_cursorPos = 0;
			}
			else
			{
				int relativeX = p.x - startX;
				int idx = 0;
				while (idx < (int)m_playerCharCumulative.size() && m_playerCharCumulative[idx] < relativeX)
					idx++;
				m_cursorPos = idx;
				if (m_cursorPos < 0) m_cursorPos = 0;
				if (m_cursorPos >(int)m_playerName.size()) m_cursorPos = (int)m_playerName.size();
			}
		}
		else
		{
			m_bInputActive = false;
			m_cursorVisible = false;
		}
	}

	if (!m_bInputActive) return;

	std::string& input = m_playerName;
	bool changed = false;

	if ((int)input.size() < f_playerNameMaxCount)
	{
		for (int key = 'A'; key <= 'Z'; key++)
		{
			if (GetAsyncKeyState(key) & 0x0001)
			{
				char ch = static_cast<char>(key);
				if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) == 0)
					ch = (char)(key + 32);
				input.insert(m_cursorPos, 1, ch);
				m_cursorPos++;
				changed = true;
				break;
			}
		}
		for (int key = '0'; key <= '9'; key++)
		{
			if (GetAsyncKeyState(key) & 0x0001)
			{
				input.insert(m_cursorPos, 1, static_cast<char>(key));
				m_cursorPos++;
				changed = true;
				break;
			}
		}
		if (GetAsyncKeyState(VK_SPACE) & 0x0001)
		{
			input.insert(m_cursorPos, 1, ' ');
			m_cursorPos++;
			changed = true;
		}
	}

	// 削除 / カーソル移動
	if (GetAsyncKeyState(VK_LEFT) & 0x0001)
	{
		if (m_cursorPos > 0) m_cursorPos--;
	}
	if (GetAsyncKeyState(VK_RIGHT) & 0x0001)
	{
		if (m_cursorPos < (int)input.size()) m_cursorPos++;
	}
	if (GetAsyncKeyState(VK_BACK) & 0x0001)
	{
		if (m_cursorPos > 0)
		{
			input.erase(m_cursorPos - 1, 1);
			m_cursorPos--;
			changed = true;
		}
	}
	if (GetAsyncKeyState(VK_DELETE) & 0x0001)
	{
		if (m_cursorPos < (int)input.size())
		{
			input.erase(m_cursorPos, 1);
			changed = true;
		}
	}

	// 文字列が変わったら幅キャッシュを再計算
	if (changed)
	{
		m_playerCharCumulative.clear();
		int totalW = 0;
		for (size_t i = 0; i < m_playerName.size(); ++i)
		{
			char cstr[2] = { m_playerName[i], '\0' };
			int cw = 0, ch = 0;
			m_pEngine->GetTextSize(FONT_GOTHIC60, cstr, &cw, &ch);
			totalW += cw;
			m_playerCharCumulative.push_back(totalW);
		}
		m_playerTextWidth = totalW;
	}

	// 入力中は点滅表示を即リセットして見やすくする
	if (changed)
	{
		m_cursorVisible = true;
		m_playerTimer = 0.0f;
	}
}

void SceneTitle::DrawPlayerNameInput()
{
	m_pEngine->DrawPrintf(f_playerNameLabelPos.x, f_playerNameLabelPos.y, FONT_GOTHIC60, Color::WHITE, f_playerNameLabelText);

	if (!m_playerName.empty())
		m_pEngine->DrawPrintfCenter((int)f_playerNameInputPos.x, (int)f_playerNameInputPos.y, FONT_GOTHIC60, (m_bInputActive ? Color::YELLOW : Color::WHITE), m_playerName.c_str());
	else
		m_pEngine->DrawPrintfCenter((int)f_playerNameInputPos.x, (int)f_playerNameInputPos.y, FONT_GOTHIC60, Color::GRAY, f_emptyNamePlaceholder);

	if (m_bInputActive && m_cursorVisible)
	{
		if (m_playerCharCumulative.size() != m_playerName.size())
		{
			// 再計算（念のため）
			m_playerCharCumulative.clear();
			int totalW = 0;
			for (size_t i = 0; i < m_playerName.size(); ++i)
			{
				char cstr[2] = { m_playerName[i], '\0' };
				int cw = 0, ch = 0;
				m_pEngine->GetTextSize(FONT_GOTHIC60, cstr, &cw, &ch);
				totalW += cw;
				m_playerCharCumulative.push_back(totalW);
			}
			m_playerTextWidth = totalW;
		}

		int textWidth = m_playerTextWidth;
		int textHeight = 0;
		if (m_playerName.empty())
		{
			int phw = 0;
			m_pEngine->GetTextSize(FONT_GOTHIC60, f_emptyNamePlaceholder, &phw, &textHeight);
		}
		else
		{
			int th = 0;
			m_pEngine->GetTextSize(FONT_GOTHIC60, m_playerName.c_str(), &textWidth, &th);
			textHeight = th;
		}

		int startX = (int)f_playerNameInputPos.x - textWidth / 2;

		int prefixWidth = 0;
		if (m_cursorPos > 0 && m_cursorPos <= (int)m_playerCharCumulative.size())
			prefixWidth = m_playerCharCumulative[m_cursorPos - 1];

		int cursorX = startX + prefixWidth;
		int cursorY = (int)f_playerNameInputPos.y;
		m_pEngine->DrawLine(cursorX, cursorY, cursorX, cursorY + textHeight, Color::YELLOW);
	}
}