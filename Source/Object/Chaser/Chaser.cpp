// Chaser.cpp - プレイヤー固有の実装のみ

#define _USING_V110_SDK71_ 1

#include "Chaser.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;

Chaser::Chaser()
{
}

Chaser::~Chaser()
{
}

void Chaser::Initialize(Engine* pEngine, Map& map, Projection* projection, Camera& camera, DirectionalLight& light)
{
	if (m_bIsLocal)
		SoundManager::RegisterGameObject(ID_PALYER, "Chaser");

	LoadParameter();
	CharacterBase::Initialize(pEngine, MODEL_CHARACTER, projection, camera, light);
	m_position = map.GetPlayerStartPosition();
	m_targetPosition = m_position;
	UpdateMatrix(light);
}

void Chaser::InitializeAtPosition(Engine* pEngine, const D3DXVECTOR3& startPos, Projection* projection, Camera& camera, DirectionalLight& light)
{
	if (m_bIsLocal)
		SoundManager::RegisterGameObject(ID_PALYER, "Chaser");

	LoadParameter();
	CharacterBase::Initialize(pEngine, MODEL_CHARACTER, projection, camera, light);
	m_position = startPos;
	m_targetPosition = m_position;
	UpdateMatrix(light);
}

void Chaser::Release(Engine* pEngine)
{
	if (m_bIsLocal)
		SoundManager::UnregisterGameObject(ID_PALYER);
}

void Chaser::Update(Engine* pEngine, Map& map, Camera& camera, DirectionalLight& light, float deltaTime)
{
	m_deltaTime = deltaTime;
	SetMouseCursor(pEngine, camera);
	Input(pEngine);
	m_speed = f_walkSpeed * m_deltaTime; //スキルなどで移動速度を変えるなら変える
	Move(map);
	SetThirdPersonFromBehind(pEngine, camera, map);
	UpdateLight(pEngine);
	UpdateMatrix(light);
}

NetPlayerState Chaser::GetNetState() const
{
	NetPlayerState state = CharacterBase::GetNetState();
	return state;
}

void Chaser::UpdateFromNetwork(const NetPlayerState& state, DirectionalLight& light, float deltaTime)
{
	CharacterBase::UpdateFromNetwork(state, light, deltaTime);

}

void Chaser::Draw(Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight)
{
	if (m_bIsLocal && m_bFirstPerson)
		return;

	CharacterBase::Draw(pCamera, pProj, pAmbient, pLight);
}

void Chaser::DebugPrint(Engine* pEngine)
{
	pEngine->DrawPrintf(0, 50, FONT_GOTHIC40, Color::BLUE, "Position: %f,%f,%f", m_position.x, m_position.y, m_position.z);
	pEngine->DrawPrintf(0, 100, FONT_GOTHIC40, Color::BLUE, "depth: %f,%f,%f", m_depth.x, m_depth.y, m_depth.z);
	pEngine->DrawPrintf(0, 150, FONT_GOTHIC40, Color::BLUE, "vAngle: %f", m_vAngle);
	pEngine->DrawPrintf(0, 200, FONT_GOTHIC40, Color::BLUE, "hAngle: %f", m_hAngle);
	pEngine->DrawPrintf(0, 250, FONT_GOTHIC40, Color::BLUE, "speed: %f", m_speed);
	pEngine->DrawPrintf(0, 300, FONT_GOTHIC40, Color::BLUE, "ClientID: %u", m_clientId);
	pEngine->DrawPrintf(0, 350, FONT_GOTHIC40, Color::BLUE, "Name: %s", m_characterName.c_str());
	pEngine->DrawPrintf(0, 400, FONT_GOTHIC40, Color::BLUE, "Local: %s", m_bIsLocal ? "Yes" : "No");
}

SpotLight * Chaser::GetLights()
{
	return &m_spotLight;
}

void Chaser::UpdateLight(Engine* pEngine)
{
	m_direction.x = sinf(m_angle);
	m_direction.y = 0.0f;
	m_direction.z = cosf(m_angle);
	D3DXVec3Normalize(&m_direction, &m_direction);

	m_eyePosition = D3DXVECTOR3(m_position.x, m_position.y + f_eyePsoitionY, m_position.z);
	m_spotLight.SetPosition(m_eyePosition);
	m_spotLight.SetDirection(m_direction);
	m_spotLight.SetDevice(pEngine, 0);
}

void Chaser::LoadParameter()
{
	std::ifstream file(JSON_CHASER_PARAMETER);
	if (!file.is_open())
	{
		throw DxSystemException(DxSystemException::OM_FILE_OPEN_ERROR);
	}
	nlohmann::json config;
	file >> config;
	file.close();

	f_crouchSpeed = config["crouchSpeed"];
	f_walkSpeed = config["walkSpeed"];
	f_maxAngleV = config["maxAngleV"];
	f_minAngleV = config["minAngleV"];
	f_defaultSenseV = config["defaultSenseV"];
	f_defaultSenseH = config["defaultSenseH"];
	f_baseHAngle = config["baseAngleH"];
	f_baseVAngle = config["baseAngleV"];
	f_headSize = config["headSize"];
	f_radius = config["radius"];
	f_eyePsoitionY = config["eyePsoitionY"];
	for (int i = 0; i < 3; i++)
	{
		f_standEyePosition[i] = config["standEyePosition"][i];
		f_crouchEyePosition[i] = config["crouchEyePosition"][i];
	}

	float value0 = config["lightDiffuse"][0];
	float value1 = config["lightDiffuse"][1];
	float value2 = config["lightDiffuse"][2];
	float value3 = config["lightDiffuse"][3];
	m_spotLight.SetDiffuse(value0, value1, value2, value3);

	value0 = config["lightAttenuation"][0];
	value1 = config["lightAttenuation"][1];
	value2 = config["lightAttenuation"][2];
	m_spotLight.SetAttenuation(value0, value1, value2);

	value0 = config["lightCone"][0];
	value1 = D3DXToRadian(config["lightCone"][1]);
	m_lightFov = D3DXToRadian(config["lightCone"][2]);
	m_spotLight.SetCone(value0, value1, m_lightFov);
	
	m_lightRange = config["lightRange"];
	m_spotLight.SetRange(m_lightRange);
}