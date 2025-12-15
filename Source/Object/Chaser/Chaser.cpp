// Chaser.cpp - プレイヤー固有の実装 + ブレス攻撃機能

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
	, m_pIceBreath(nullptr)
	, m_bBreathActive(false)
	, m_bBreathButtonPressed(false)
	, m_lastBreathTime(0)
	, f_breathCooldown(3.0f)
	, m_breathDuration(3000)
{
	D3DXMatrixIdentity(&m_matLightView);
	D3DXMatrixIdentity(&m_matLightProj);
}

Chaser::~Chaser()
{
	ReleaseShadowMap();

	// ★★★ エフェクトを削除 ★★★
	if (m_pIceBreath)
	{
		delete m_pIceBreath;
		m_pIceBreath = nullptr;
	}
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

	// ★★★ ブレスエフェクトを1回だけ生成（再利用） ★★★
	if (!m_pIceBreath)
	{
		m_pIceBreath = new IceBreath();
		m_pIceBreath->Initialize(pEngine, m_eyePosition, m_breathDuration);
		NET_LOG_F("[Chaser] IceBreath生成: ID=%u", m_clientId);
	}

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

	// ★★★ ブレスエフェクトを1回だけ生成（再利用） ★★★
	if (!m_pIceBreath)
	{
		m_pIceBreath = new IceBreath();
		m_pIceBreath->Initialize(pEngine, m_eyePosition, m_breathDuration);
		NET_LOG_F("[Chaser] IceBreath生成: ID=%u", m_clientId);
	}

	UpdateMatrix(light);
}

void Chaser::Release(Engine* pEngine)
{
	ReleaseShadowMap();

	// ★★★ エフェクトを削除 ★★★
	if (m_pIceBreath)
	{
		delete m_pIceBreath;
		m_pIceBreath = nullptr;
	}

	if (m_bIsLocal)
		SoundManager::UnregisterGameObject(ID_PALYER);
}

void Chaser::Update(Engine* pEngine, Map& map, Camera& camera, DirectionalLight& light, float deltaTime)
{
	m_deltaTime = deltaTime;
	SetMouseCursor(pEngine, camera);
	Input(pEngine);

	// ★★★ ブレス攻撃の更新 ★★★
	UpdateBreathAttack(pEngine);

	m_speed = f_walkSpeed * m_deltaTime;
	Move(map);
	SetThirdPersonFromBehind(pEngine, camera, map);
	UpdateLight(pEngine);
	UpdateLightMatrices();
	UpdateMatrix(light);
}

// ★★★ ブレス攻撃の更新処理（エフェクト再利用版） ★★★
void Chaser::UpdateBreathAttack(Engine* pEngine)
{
	// ローカルプレイヤーのみブレス攻撃を処理
	if (!m_bIsLocal)
		return;

	// エフェクトが生成されていない場合は何もしない
	if (!m_pIceBreath)
		return;

	// 左クリックが押されているかチェック
	bool isAttackPressed = (m_keyFlag & ATTACK_KEY) != 0;

	// ボタンが押された瞬間（立ち上がりエッジ）を検出
	bool isButtonJustPressed = isAttackPressed && !m_bBreathButtonPressed;
	m_bBreathButtonPressed = isAttackPressed;

	// ★★★ ブレスが発動中の場合は更新（表示中） ★★★
	if (m_bBreathActive)
	{
		m_pIceBreath->Update();

		// ★★★ エフェクトが非アクティブになったら終了 ★★★
		if (!m_pIceBreath->IsActive())
		{
			m_bBreathActive = false;
			NET_LOG_F("[Chaser] ブレス終了: ID=%u", m_clientId);
		}
	}

	// ★★★ 左クリックが押された瞬間で、クールタイムが経過していればブレス発動 ★★★
	if (isButtonJustPressed && !m_bBreathActive && CanUseBreath())
	{
		// ★★★ エフェクトを再利用（Activate） ★★★
		m_pIceBreath->Activate(m_eyePosition, m_depth);
		m_bBreathActive = true;
		m_lastBreathTime = timeGetTime();

		NET_LOG_F("[Chaser] ブレス発動: ID=%u Pos=(%.1f,%.1f,%.1f) Dir=(%.2f,%.2f,%.2f)",
			m_clientId,
			m_eyePosition.x, m_eyePosition.y, m_eyePosition.z,
			m_depth.x, m_depth.y, m_depth.z);
	}
}

// ★★★ ブレスが使用可能かチェック ★★★
bool Chaser::CanUseBreath() const
{
	DWORD now = timeGetTime();
	float elapsedTime = (now - m_lastBreathTime) / 1000.0f;
	return elapsedTime >= f_breathCooldown;
}

NetPlayerState Chaser::GetNetState() const
{
	NetPlayerState state = CharacterBase::GetNetState();

	const D3DLIGHT9& light = m_spotLight.GetLight();
	state.lightPosX = light.Position.x;
	state.lightPosY = light.Position.y;
	state.lightPosZ = light.Position.z;
	state.lightDirX = light.Direction.x;
	state.lightDirY = light.Direction.y;
	state.lightDirZ = light.Direction.z;
	state.lightRange = light.Range;

	return state;
}

void Chaser::UpdateFromNetwork(const NetPlayerState& state, DirectionalLight& light, float deltaTime)
{
	// 基底クラスの更新
	CharacterBase::UpdateFromNetwork(state, light, deltaTime);

	// ★★★ 重要: ライト情報を適用 ★★★
	D3DXVECTOR3 lightPos(state.lightPosX, state.lightPosY, state.lightPosZ);
	D3DXVECTOR3 lightDir(state.lightDirX, state.lightDirY, state.lightDirZ);

	// ライト情報の検証
	if (D3DXVec3Length(&lightPos) < 0.01f)
	{
		lightPos = m_eyePosition;
	}

	if (D3DXVec3Length(&lightDir) < 0.01f)
	{
		lightDir = m_depth;
	}

	float lightRange = state.lightRange;
	if (lightRange < 0.1f)
	{
		lightRange = m_lightRange;
	}

	// ★★★ SpotLightに設定 ★★★
	m_spotLight.SetPosition(lightPos);
	m_spotLight.SetDirection(lightDir);
	m_spotLight.SetRange(lightRange);

	// ★★★ 重要追加: ライト行列を更新 ★★★
	UpdateLightMatrices();

	// デバッグログ（頻度制限）
	static std::map<uint32_t, DWORD> lastLogPerChaser;
	DWORD now = timeGetTime();
	if (now - lastLogPerChaser[m_clientId] > 5000)
	{
		NET_LOG_F("[Chaser::UpdateFromNetwork] ID=%u ライト更新: Pos=(%.1f,%.1f,%.1f) Dir=(%.2f,%.2f,%.2f) Range=%.2f",
			m_clientId,
			lightPos.x, lightPos.y, lightPos.z,
			lightDir.x, lightDir.y, lightDir.z,
			lightRange);
		lastLogPerChaser[m_clientId] = now;
	}
}

void Chaser::Draw(Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight)
{
	if (m_bIsLocal && m_bFirstPerson)
		return;

	CharacterBase::Draw(pCamera, pProj, pAmbient, pLight);
}

// ★★★ エフェクトの描画（エフェクト内部でm_bActiveをチェック） ★★★
void Chaser::DrawEffects(Camera* pCamera, Projection* pProj)
{
	// ★★★ エフェクトが生成されている場合のみ描画を試みる ★★★
	// （内部でm_bActiveがfalseなら描画スキップ）
	if (m_pIceBreath)
	{
		m_pIceBreath->Draw(pCamera, pProj);
	}
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

	// ★★★ ブレス攻撃のデバッグ情報 ★★★
	pEngine->DrawPrintf(0, 450, FONT_GOTHIC40, Color::BLUE, "Breath: %s", m_bBreathActive ? "Active" : "Inactive");

	if (m_bIsLocal)
	{
		DWORD now = timeGetTime();
		float cooldownRemaining = f_breathCooldown - ((now - m_lastBreathTime) / 1000.0f);
		if (cooldownRemaining < 0) cooldownRemaining = 0;
		pEngine->DrawPrintf(0, 500, FONT_GOTHIC40, Color::BLUE, "Cooldown: %.1fs", cooldownRemaining);
	}
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

	// ★★★ 重要追加: ライト行列も更新 ★★★
	UpdateLightMatrices();

	static std::map<uint32_t, DWORD> lastLogPerChaser;
	DWORD now = timeGetTime();
	if (now - lastLogPerChaser[m_clientId] > 5000)
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

	// シャドウマップテクスチャの作成（R8G8B8A8フォーマットに変更）
	HRESULT hr = pDevice->CreateTexture(
		SHADOW_MAP_SIZE, SHADOW_MAP_SIZE,
		1,
		D3DUSAGE_RENDERTARGET,
		D3DFMT_A8R8G8B8,  // shadowプロジェクトと同じフォーマット
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

	// サーフェスの取得
	hr = m_pShadowTexture->GetSurfaceLevel(0, &m_pShadowSurface);
	if (FAILED(hr))
	{
		NET_LOG("[Chaser] シャドウマップサーフェス取得失敗");
		m_pShadowTexture->Release();
		m_pShadowTexture = nullptr;
		m_bShadowMapEnabled = false;
		return;
	}

	// 深度バッファの作成（D16フォーマットに変更）
	hr = pDevice->CreateDepthStencilSurface(
		SHADOW_MAP_SIZE, SHADOW_MAP_SIZE,
		D3DFMT_D16,  // shadowプロジェクトと同じフォーマット
		D3DMULTISAMPLE_NONE,
		0,
		TRUE,
		&m_pShadowDepthSurface,
		NULL
	);

	if (FAILED(hr))
	{
		NET_LOG("[Chaser] シャドウマップ深度バッファ作成失敗");
		m_pShadowSurface->Release();
		m_pShadowSurface = nullptr;
		m_pShadowTexture->Release();
		m_pShadowTexture = nullptr;
		m_bShadowMapEnabled = false;
		return;
	}

	// スケールバイアス行列の作成
	CreateScaleBiasMatrix();

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

	// ★★★ 重要: ライトのFOVを1.5倍に拡大（端まで影を描画） ★★★
	// 元のFOVが狭すぎるため、ライトの視錐台を広げる
	float expandedFov = m_lightFov * 2.0f;

	// ★★★ 最大でも170度以下に制限（180度だと逆転するため） ★★★
	expandedFov = min(expandedFov, D3DXToRadian(170.0f));

	// ★★★ 修正: ニアプレーンを0.1fに、ファープレーンを大きく ★★★
	D3DXMatrixPerspectiveFovLH(&m_matLightProj, expandedFov, 1.0f, 1.0f, m_lightRange * 1.2f);

	// ★★★ デバッグログ追加 ★★★
	static DWORD lastLog = 0;
	DWORD now = timeGetTime();
	if (now - lastLog > 5000)
	{
		NET_LOG_F("[Chaser::UpdateLightMatrices] ID=%u FOV=%.2f度 ExpandedFOV=%.2f度 Range=%.2f",
			m_clientId,
			D3DXToDegree(m_lightFov),
			D3DXToDegree(expandedFov),
			m_lightRange);
		lastLog = now;
	}
}

// CreateScaleBiasMatrix関数
void Chaser::CreateScaleBiasMatrix()
{
	m_matScaleBias = D3DXMATRIX(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f
	);
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

	// ★★★ ブレス攻撃のパラメータを読み込み ★★★
	if (config.contains("breathCooldown"))
	{
		f_breathCooldown = config["breathCooldown"];
	}
	if (config.contains("breathDuration"))
	{
		m_breathDuration = config["breathDuration"];
	}
}