// Chaser.cpp - プレイヤー固有の実装のみ

#define _USING_V110_SDK71_ 1

#include "Chaser.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;

Chaser::Chaser()
	: m_bShadowMapEnabled(false)
	, m_pShadowTexture(nullptr)
	, m_pShadowSurface(nullptr)
	, m_pShadowDepthSurface(nullptr)
{
	D3DXMatrixIdentity(&m_matLightView);
	D3DXMatrixIdentity(&m_matLightProj);
}

Chaser::~Chaser()
{
	ReleaseShadowMap();
}

void Chaser::Initialize(Engine* pEngine, Map& map, Projection* projection, Camera& camera, DirectionalLight& light)
{
	if (m_bIsLocal)
		SoundManager::RegisterGameObject(ID_PALYER, "Chaser");

	LoadParameter();
	CharacterBase::Initialize(pEngine, MODEL_CHARACTER, projection, camera, light);
	m_position = map.GetPlayerStartPosition();
	m_targetPosition = m_position;

	// ★★★ シャドウマップの作成 ★★★
	CreateShadowMap(pEngine);

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

	// ★★★ シャドウマップの作成 ★★★
	CreateShadowMap(pEngine);

	UpdateMatrix(light);
}

void Chaser::Release(Engine* pEngine)
{
	ReleaseShadowMap();

	if (m_bIsLocal)
		SoundManager::UnregisterGameObject(ID_PALYER);
}

void Chaser::Update(Engine* pEngine, Map& map, Camera& camera, DirectionalLight& light, float deltaTime)
{
	m_deltaTime = deltaTime;
	SetMouseCursor(pEngine, camera);
	Input(pEngine);
	m_speed = f_walkSpeed * m_deltaTime;
	Move(map);
	SetThirdPersonFromBehind(pEngine, camera, map);
	UpdateLight(pEngine);
	UpdateLightMatrices();  // ★★★ ライト行列の更新 ★★★
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

SpotLight* Chaser::GetLights()
{
	return &m_spotLight;
}

void Chaser::UpdateLight(Engine* pEngine)
{
	m_direction = m_depth;
	D3DXVec3Normalize(&m_direction, &m_direction);

	m_eyePosition = D3DXVECTOR3(m_position.x, m_position.y + f_eyePsoitionY, m_position.z);

	m_spotLight.SetPosition(m_eyePosition);
	m_spotLight.SetDirection(m_direction);
	m_spotLight.SetDevice(pEngine, 0);

	static std::map<uint32_t, DWORD> lastLogPerChaser;
	DWORD now = timeGetTime();
	if (now - lastLogPerChaser[m_clientId] > 2000)
	{
		NET_LOG_F("[Chaser::UpdateLight] ID=%u Pos=(%.1f,%.1f,%.1f) Dir=(%.2f,%.2f,%.2f) IsLocal=%s",
			m_clientId,
			m_eyePosition.x, m_eyePosition.y, m_eyePosition.z,
			m_direction.x, m_direction.y, m_direction.z,
			m_bIsLocal ? "Yes" : "No");
		lastLogPerChaser[m_clientId] = now;
	}
}

void Chaser::DrawDepth(Engine* pEngine, const D3DXMATRIX* pMatLightVP)
{
	m_model.DrawDepth(pEngine, pMatLightVP);
}

// ★★★ シャドウマップの作成 ★★★
void Chaser::CreateShadowMap(Engine* pEngine)
{
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	// シャドウマップテクスチャの作成
	HRESULT hr = pDevice->CreateTexture(
		SHADOW_MAP_SIZE, SHADOW_MAP_SIZE,
		1,
		D3DUSAGE_RENDERTARGET,
		D3DFMT_R32F,
		D3DPOOL_DEFAULT,
		&m_pShadowTexture,
		NULL
	);

	if (FAILED(hr))
	{
		NET_LOG("[Chaser] シャドウマップテクスチャ作成失敗");
		m_bShadowMapEnabled = false;
		return;
	}

	// 深度バッファの作成
	hr = pDevice->CreateDepthStencilSurface(
		SHADOW_MAP_SIZE, SHADOW_MAP_SIZE,
		D3DFMT_D24S8,
		D3DMULTISAMPLE_NONE,
		0,
		TRUE,
		&m_pShadowDepthSurface,
		NULL
	);

	if (FAILED(hr))
	{
		NET_LOG("[Chaser] シャドウマップ深度バッファ作成失敗");
		m_pShadowTexture->Release();
		m_pShadowTexture = nullptr;
		m_bShadowMapEnabled = false;
		return;
	}

	m_bShadowMapEnabled = true;
	NET_LOG_F("[Chaser] シャドウマップ作成成功: ID=%u", m_clientId);
}

// ★★★ シャドウマップの解放 ★★★
void Chaser::ReleaseShadowMap()
{
	if (m_pShadowDepthSurface)
	{
		m_pShadowDepthSurface->Release();
		m_pShadowDepthSurface = nullptr;
	}

	if (m_pShadowSurface)
	{
		m_pShadowSurface->Release();
		m_pShadowSurface = nullptr;
	}

	if (m_pShadowTexture)
	{
		m_pShadowTexture->Release();
		m_pShadowTexture = nullptr;
	}

	m_bShadowMapEnabled = false;
}

void Chaser::UpdateLightMatrices()
{
	if (!m_bShadowMapEnabled) return;

	// ライトのビュー行列を作成
	D3DXVECTOR3 lightPos = m_eyePosition;
	D3DXVECTOR3 lightTarget = m_eyePosition + m_direction;
	D3DXVECTOR3 lightUp(0.0f, 1.0f, 0.0f);

	// 方向ベクトルが上ベクトルと平行な場合の対策
	D3DXVECTOR3 normDir;
	D3DXVec3Normalize(&normDir, &m_direction);
	if (fabs(D3DXVec3Dot(&normDir, &lightUp)) > 0.99f)
	{
		lightUp = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
	}

	D3DXMatrixLookAtLH(&m_matLightView, &lightPos, &lightTarget, &lightUp);

	// ★★★ 修正: ニアプレーンを0.05に（さらに小さく） ★★★
	D3DXMatrixPerspectiveFovLH(&m_matLightProj, m_lightFov, 1.0f, 0.05f, m_lightRange);
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