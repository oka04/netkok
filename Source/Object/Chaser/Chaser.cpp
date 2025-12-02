// Chaser.cpp - シャドウマップ実装部分（既存コードに追加）

#define _USING_V110_SDK71_ 1

#include "Chaser.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;

Chaser::Chaser()
	: m_pShadowTexture(nullptr)
	, m_pShadowSurface(nullptr)
	, m_pShadowDepthBuffer(nullptr)
	, m_shadowMapSize(0)
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

	// ★★★ シャドウマップの初期化 ★★★
	InitializeShadowMap(pEngine, 512);

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

	// ★★★ シャドウマップの初期化 ★★★
	InitializeShadowMap(pEngine, 512);

	UpdateMatrix(light);
}

void Chaser::Release(Engine* pEngine)
{
	ReleaseShadowMap();

	if (m_bIsLocal)
		SoundManager::UnregisterGameObject(ID_PALYER);
}

// ★★★ シャドウマップの初期化 ★★★
void Chaser::InitializeShadowMap(Engine* pEngine, int shadowMapSize)
{
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();
	m_shadowMapSize = shadowMapSize;

	HRESULT hr = S_OK;

	// シャドウマップ用テクスチャの作成
	hr = pDevice->CreateTexture(
		m_shadowMapSize, m_shadowMapSize, 1,
		D3DUSAGE_RENDERTARGET,
		D3DFMT_A8R8G8B8,
		D3DPOOL_DEFAULT,
		&m_pShadowTexture,
		NULL
	);

	if (FAILED(hr)) {
		NET_LOG("[Chaser] シャドウテクスチャの作成失敗");
		return;
	}

	// サーフェスの取得
	hr = m_pShadowTexture->GetSurfaceLevel(0, &m_pShadowSurface);
	if (FAILED(hr)) {
		NET_LOG("[Chaser] シャドウサーフェスの取得失敗");
		ReleaseShadowMap();
		return;
	}

	// 深度バッファの作成
	hr = pDevice->CreateDepthStencilSurface(
		m_shadowMapSize, m_shadowMapSize,
		D3DFMT_D16,
		D3DMULTISAMPLE_NONE, 0, TRUE,
		&m_pShadowDepthBuffer,
		NULL
	);

	if (FAILED(hr)) {
		NET_LOG("[Chaser] シャドウ深度バッファの作成失敗");
		ReleaseShadowMap();
		return;
	}

	NET_LOG_F("[Chaser] シャドウマップ初期化成功: %dx%d", m_shadowMapSize, m_shadowMapSize);
}

// ★★★ シャドウマップの解放 ★★★
void Chaser::ReleaseShadowMap()
{
	if (m_pShadowDepthBuffer) {
		m_pShadowDepthBuffer->Release();
		m_pShadowDepthBuffer = nullptr;
	}

	if (m_pShadowSurface) {
		m_pShadowSurface->Release();
		m_pShadowSurface = nullptr;
	}

	if (m_pShadowTexture) {
		m_pShadowTexture->Release();
		m_pShadowTexture = nullptr;
	}
}

// ★★★ ライトのビュー行列を取得 ★★★
D3DXMATRIX Chaser::GetLightViewMatrix() const
{
	return m_matLightView;
}

// ★★★ ライトのプロジェクション行列を取得 ★★★
D3DXMATRIX Chaser::GetLightProjectionMatrix() const
{
	return m_matLightProj;
}

// ★★★ ライトのビュー・プロジェクション行列を取得 ★★★
D3DXMATRIX Chaser::GetLightViewProjectionMatrix() const
{
	return m_matLightView * m_matLightProj;
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
	UpdateMatrix(light);
}

void Chaser::UpdateLight(Engine* pEngine)
{
	// 既存のライト更新処理
	m_direction = m_depth;
	D3DXVec3Normalize(&m_direction, &m_direction);
	m_eyePosition = D3DXVECTOR3(m_position.x, m_position.y + f_eyePsoitionY, m_position.z);

	m_spotLight.SetPosition(m_eyePosition);
	m_spotLight.SetDirection(m_direction);
	m_spotLight.SetDevice(pEngine, 0);

	// ★★★ シャドウマップ用のビュー・プロジェクション行列を更新 ★★★
	if (m_pShadowTexture)
	{
		// ビュー行列: ライトの位置から方向を見る
		D3DXVECTOR3 lightTarget = m_eyePosition + m_direction * m_lightRange * 0.5f;
		D3DXVECTOR3 up = D3DXVECTOR3(0, 1, 0);

		// 方向がほぼ真上/真下の場合は横を向ける
		if (fabs(m_direction.y) > 0.99f) {
			up = D3DXVECTOR3(1, 0, 0);
		}

		D3DXMatrixLookAtLH(&m_matLightView, &m_eyePosition, &lightTarget, &up);

		// プロジェクション行列: スポットライトの照射角と範囲を使用
		D3DXMatrixPerspectiveFovLH(
			&m_matLightProj,
			m_lightFov,  // 視野角
			1.0f,        // アスペクト比（正方形）
			0.1f,        // ニアクリップ
			m_lightRange // ファークリップ
		);
	}

	// デバッグログ
	static std::map<uint32_t, DWORD> lastLogPerChaser;
	DWORD now = timeGetTime();
	if (now - lastLogPerChaser[m_clientId] > 2000)
	{
		NET_LOG_F("[Chaser::UpdateLight] ID=%u Pos=(%.1f,%.1f,%.1f) Dir=(%.2f,%.2f,%.2f) Shadow=%s",
			m_clientId,
			m_eyePosition.x, m_eyePosition.y, m_eyePosition.z,
			m_direction.x, m_direction.y, m_direction.z,
			IsShadowMapEnabled() ? "ON" : "OFF");
		lastLogPerChaser[m_clientId] = now;
	}
}

// 以下は既存のコードと同じ
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