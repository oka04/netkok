#define _USING_V110_SDK71_ 1

#include "Player.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;


void Player::Initialize(Engine* pEngine, Map& map, Projection* projection, Camera& camera, DirectionalLight &light)
{
	SoundManager::RegisterGameObject(ID_PALYER, "PLAYER");

	LoadParameter();
	CharacterBase::Initialize(pEngine, MODEL_CHARACTER, projection, camera, light);

	m_position = map.GetPlayerStartPosition();
	pEngine->AddTexture(TEXTURE_STAMINA_GAUGE);
	m_stamina = f_maxStamina;
	m_staminaRecoveryTimer = 0.0f;
	m_bFatigued = false;
	UpdateMatrix(light);
}	

void Player::Release(Engine * pEngine)
{
	pEngine->ReleaseTexture(TEXTURE_STAMINA_GAUGE);
	SoundManager::UnregisterGameObject(ID_PALYER);
}

void Player::Update(Engine* pEngine, Map& map, Camera& camera, DirectionalLight &light, float deltaTime)
{
	m_deltaTime = deltaTime;

	SetMouseCursor(pEngine, camera);
	Input(pEngine);

	UpdateStamina(deltaTime);

	ChangeSpeed();
	Move(map);
	SetThirdPersonFromBehind(pEngine, camera, map);

	UpdateMatrix(light);
}

void Player::Draw(Camera * pCamera, Projection * pProj, AmbientLight * pAmbient, DirectionalLight * pLight)
{
	if (!m_bFirstPerson)
	{
		CharacterBase::Draw(pCamera, pProj, pAmbient, pLight);
	}
}

void Player::DrawStaminaGauge(Engine * pEngine)
{
	RECT sour, dest;

	float gaugeRate = m_stamina / f_maxStamina;

	int gaugeColorIndex = (int)f_gaugeColorThresholds.size();

	if (m_bFatigued)
	{
		gaugeColorIndex = FATIGUE;
	}
	else
	{
		for (int i = 0; i < (int)f_gaugeColorThresholds.size(); ++i)
		{
			if (gaugeRate >= f_gaugeColorThresholds[i])
			{
				gaugeColorIndex = i;
				break;
			}
		}
	}


	int wx, wy;
	wy = f_gaugeSourSize.y * GRAY;
	SetRect(&sour, 0, wy, f_gaugeSourSize.x, wy + f_gaugeSourSize.y);

	wx = f_gaugePosition.x; 
	wy = f_gaugePosition.y;
	SetRect(&dest, wx, wy, wx + f_gaugeDestSize.x, wy + f_gaugeDestSize.y);
	pEngine->Blt(&dest, TEXTURE_STAMINA_GAUGE, &sour, f_gaugeAlpha, 0.0f);

	if (gaugeRate > 0.0f)
	{
		wy = f_gaugeSourSize.y * gaugeColorIndex;
		SetRect(&sour, 0, wy, f_gaugeSourSize.x, wy + f_gaugeSourSize.y);

		wx = f_gaugePosition.x;
		wy = f_gaugePosition.y;
		SetRect(&dest, wx, wy, wx + (int)(f_gaugeDestSize.x * gaugeRate), wy + f_gaugeDestSize.y);
		pEngine->Blt(&dest, TEXTURE_STAMINA_GAUGE, &sour, f_gaugeAlpha, 0.0f);
	}
}

void Player::DebugPrint(Engine * pEngine)
{
	pEngine->DrawPrintf(0, 50, FONT_GOTHIC40, Color::BLUE, "Position：%f,%f,%f", m_position.x, m_position.y, m_position.z);
	pEngine->DrawPrintf(0, 100, FONT_GOTHIC40, Color::BLUE, "depth：%f,%f,%f", m_depth.x, m_depth.y, m_depth.z);
	pEngine->DrawPrintf(0, 150, FONT_GOTHIC40, Color::BLUE, "vAngle：%f", m_vAngle);
	pEngine->DrawPrintf(0, 250, FONT_GOTHIC40, Color::BLUE, "hAngle：%f", m_hAngle);
	pEngine->DrawPrintf(0, 300, FONT_GOTHIC40, Color::BLUE, "speed：%f", m_speed);
}

void Player::UpdateStamina(float deltaTime)
{
	//移動キー入力とダッシュキーが押されているか
	bool bMoving = (m_keyFlag & (W_KEY | S_KEY | D_KEY | A_KEY)) != 0;
	bool bDashing = (m_keyFlag & DASH_KEY) && bMoving;

	//疲労状態の処理
	if (m_bFatigued)
	{
		if (m_stamina >= f_maxStamina * f_fatigueRecoveryThreshold)
		{
			//スタミナが一定数以上回復したら疲労状態を解除
			m_bFatigued = false;
			m_staminaRecoveryTimer = f_recoveryDelayTime;
		}

		m_stamina += f_fatigueRecoveryRate * deltaTime;
		m_stamina = min(m_stamina, f_maxStamina);
		return;
	}


	if (bDashing)
	{
		//ダッシュ時のスタミナ消費
		m_stamina -= f_dashStaminaCost * deltaTime;
		m_staminaRecoveryTimer = 0.0f; 

		if (m_stamina <= 0.0f)
		{
			//スタミナ切れ
			m_stamina = 0.0f;
			m_bFatigued = true;
		}
		return;
	}
	else
	{
		m_staminaRecoveryTimer += deltaTime;
	}

	if (m_staminaRecoveryTimer >= f_recoveryDelayTime)
	{
		//回復遅延時間を経過したら回復開始
		m_stamina += ((bMoving) ? f_walkRecoveryRate: f_stopRecoveryRate) *deltaTime;
		m_stamina = min(m_stamina, f_maxStamina);
	}

	//念のためスタミナの範囲を制限
	m_stamina = max(0.0f, m_stamina);
}

void Player::LoadParameter()
{
	//jsonのファイルデータを取得
	//jsonのファイルと呼び出しの文字が必ず一致するようにする
	std::ifstream file(JSON_PLAYER_PARAMETER);
	if (!file.is_open())
	{
		throw DxSystemException(DxSystemException::OM_FILE_OPEN_ERROR);
	}

	nlohmann::json config;
	file >> config;
	file.close();
	f_gaugeAlpha = config["gaugeAlpha"];
	f_crouchSpeed = config["crouchSpeed"];
	f_walkSpeed = config["walkSpeed"];
	f_dashSpeed = config["dashSpeed"];
	f_maxAngleV = config["maxAngleV"];
	f_minAngleV = config["minAngleV"];
	f_defaultSenseV = config["defaultSenseV"];
	f_defaultSenseH = config["defaultSenseH"];
	f_baseHAngle = config["baseAngleH"];
	f_baseVAngle = config["baseAngleV"];
	f_stickAngleH = config["stickAngleH"];
	f_headSize = config["headSize"];
	f_radius = config["radius"];

	f_maxStamina = config["maxStamina"];
	f_dashStaminaCost = config["dashStaminaCost"];
	f_walkRecoveryRate = config["walkRecoveryRate"];
	f_stopRecoveryRate = config["stopRecoveryRate"];
	f_fatigueRecoveryRate = config["fatigueRecoveryRate"];
	f_recoveryDelayTime = config["recoveryDelayTime"];
	f_fatigueSpeed = config["fatigueSpeed"];
	f_fatigueRecoveryThreshold = config["fatigueRecoveryThreshold"];
	f_gaugeColorThresholds = config["gaugeColorThresholds"].get<std::vector<float>>();

	for (int i = 0; i < 2; i++)
	{
		f_gaugeSourSize[i] = config["gaugeSourSize"][i];
		f_gaugeDestSize[i] = config["gaugeDestSize"][i];
		f_gaugePosition[i] = config["gaugePosition"][i];
	}

	for (int i = 0; i < 3; i++)
	{
		f_standEyePosition[i] = config["standEyePosition"][i];
		f_crouchEyePosition[i] = config["crouchEyePosition"][i];
	} 
}

void Player::ChangeSpeed()
{
	if (m_bFatigued)
	{
		m_speed = f_fatigueSpeed * m_deltaTime;
	}
	else if (m_keyFlag & CROUCH_KEY)
	{
		m_speed = f_crouchSpeed * m_deltaTime;
	}
	else if (m_keyFlag & DASH_KEY)
	{
		m_speed = f_dashSpeed * m_deltaTime;
	}
	else
	{
		m_speed = f_walkSpeed * m_deltaTime;
	}
}
