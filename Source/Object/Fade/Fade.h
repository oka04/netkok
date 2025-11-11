#pragma once

#include "..\\..\\GameBase.h"

#include "..\\..\\Scene\\Scene\\Scene.h"

#include <fstream>
#include "..\\json.hpp" 

class Fade
{
public:
	void Initialize(Engine* pEngine);
	void Release(Engine* pEngine);
	void SetFadeIn();
	void SetFadeOut();
	bool Update(const float deltaTime);
	void Draw(Engine* pEngine);
private:
	enum FADE_STATE {
		STATE_NONE,
		STATE_IN,
		STATE_OUT,
	};

	void LoadParameter();

	float f_fadeSpeed;
	float f_minAlpha;
	float f_maxAlpha;
	float m_alpha;
	FADE_STATE m_state;
};