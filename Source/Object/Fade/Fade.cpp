#define _USING_V110_SDK71_ 1

#include "Fade.h"
using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;

void Fade::Initialize(Engine* pEngine)
{
	m_alpha = f_minAlpha;
	m_state = STATE_NONE;
	LoadParameter();
	pEngine->AddTexture(TEXTURE_FADE);
}

void Fade::Release(Engine* pEngine)
{
	pEngine->ReleaseTexture(TEXTURE_FADE);
}

void Fade::SetFadeIn()
{
	m_state = STATE_IN;
	m_alpha = f_maxAlpha;
}

void Fade::SetFadeOut()
{
	m_state = STATE_OUT;
	m_alpha = f_minAlpha;
}

bool Fade::Update(const float deltaTime)
{
	if (m_state == STATE_IN)
	{
		m_alpha -= f_fadeSpeed * deltaTime;
		if (m_alpha <= f_minAlpha)
		{
			m_alpha = f_minAlpha;
			m_state = STATE_NONE;
			return true;
		}
	}
	else if (m_state == STATE_OUT)
	{
		m_alpha += f_fadeSpeed * deltaTime;
		if (m_alpha >= f_maxAlpha)
		{
			m_alpha = f_maxAlpha;
			return true;
		}
	}
	return false;
}

void Fade::Draw(Engine* pEngine)
{
	if (m_state == STATE_NONE) return;

	RECT rect;
	SetRect(&rect, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	pEngine->Blt(&rect, TEXTURE_FADE, &rect, (int)m_alpha, 0.0f);
}

void Fade::LoadParameter()
{
	std::ifstream file(JSON_FADE_PARAMETER);
	if (!file.is_open())
	{
		throw DxSystemException(DxSystemException::OM_FILE_OPEN_ERROR);
	}

	nlohmann::json config;
	file >> config;
	file.close();

	f_fadeSpeed = config["fadeSpeed"];
	f_minAlpha = config["minAlpha"];
	f_maxAlpha = config["maxAlpha"];
}
