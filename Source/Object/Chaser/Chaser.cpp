// Chaser.cpp - ブレス状態のネットワーク同期を追加

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

	CreateShadowMap(pEngine);

	if (!m_pIceBreath)
	{
		m_pIceBreath = new IceBreath();
		m_pIceBreath->Initialize(pEngine, m_eyePosition, m_breathDuration);
		m_pIceBreath->SetMaxDistance(m_lightRange);
		NET_LOG_F("[Chaser] IceBreath生成: ID=%u Range=%.2f", m_clientId, m_lightRange);
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

	CreateShadowMap(pEngine);

	if (!m_pIceBreath)
	{
		m_pIceBreath = new IceBreath();
		m_pIceBreath->Initialize(pEngine, m_eyePosition, m_breathDuration);
		m_pIceBreath->SetMaxDistance(m_lightRange);
		NET_LOG_F("[Chaser] IceBreath生成: ID=%u Range=%.2f", m_clientId, m_lightRange);
	}

	UpdateMatrix(light);
}

void Chaser::Release(Engine* pEngine)
{
	ReleaseShadowMap();

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

	Input(pEngine);
	UpdateBreathAttack(pEngine);
	
	if (!m_bBreathActive)
	{
		SetMouseCursor(pEngine, camera);
		m_speed = f_walkSpeed * m_deltaTime;
		Move(map);
	}
	else
	{
		m_speed = 0.0f;
	}

	SetThirdPersonFromBehind(pEngine, camera, map);
	UpdateLight(pEngine);
	UpdateLightMatrices();
	UpdateMatrix(light);
}

void Chaser::UpdateBreathAttack(Engine* pEngine)
{
    if (!m_bIsLocal || !m_pIceBreath)
        return;

    bool isAttackPressed = (m_keyFlag & ATTACK_KEY) != 0;
    bool isButtonJustPressed = isAttackPressed && !m_bBreathButtonPressed;
    m_bBreathButtonPressed = isAttackPressed;

    if (m_bBreathActive)
    {
        D3DXVECTOR3 adjustedDirection = m_depth;

        D3DXVECTOR3 rightVec;
        D3DXVECTOR3 upVec = UP_DIRECTION;
        D3DXVec3Cross(&rightVec, &upVec, &m_depth);
        D3DXVec3Normalize(&rightVec, &rightVec);

        D3DXVec3Cross(&upVec, &m_depth, &rightVec);
        D3DXVec3Normalize(&upVec, &upVec);

        D3DXMATRIX matRotation;
        D3DXMatrixRotationAxis(&matRotation, &rightVec, D3DXToRadian(15.0f));
        D3DXVec3TransformCoord(&adjustedDirection, &m_depth, &matRotation);
        D3DXVec3Normalize(&adjustedDirection, &adjustedDirection);

        m_pIceBreath->SetPosition(m_eyePosition);
        m_pIceBreath->SetDirection(adjustedDirection);

        m_pIceBreath->Update();

        if (!m_pIceBreath->IsActive())
        {
            m_bBreathActive = false;
            
            NET_LOG_F("[Chaser] ブレス終了: ID=%u", m_clientId);
        }
    }

    if (isButtonJustPressed && !m_bBreathActive && CanUseBreath())
    {
        D3DXVECTOR3 adjustedDirection = m_depth;

        D3DXVECTOR3 rightVec;
        D3DXVec3Cross(&rightVec, &UP_DIRECTION, &m_depth);
        D3DXVec3Normalize(&rightVec, &rightVec);

        D3DXMATRIX matRotation;
        D3DXMatrixRotationAxis(&matRotation, &rightVec, D3DXToRadian(10.0f));
        D3DXVec3TransformCoord(&adjustedDirection, &m_depth, &matRotation);
        D3DXVec3Normalize(&adjustedDirection, &adjustedDirection);

        m_pIceBreath->Activate(m_eyePosition, adjustedDirection);
        m_bBreathActive = true;
        m_lastBreathTime = timeGetTime();

        // ★★★ ブレス発動音を再生 ★★★
        SoundManager::SetPosition(m_eyePosition, m_depth, UP_DIRECTION, m_clientId);
        SoundManager::Play(AK::EVENTS::PLAY_SE_BRACELET, m_clientId);

        NET_LOG_F("[Chaser] ブレス発動: ID=%u Pos=(%.1f,%.1f,%.1f) Dir=(%.2f,%.2f,%.2f)",
            m_clientId,
            m_eyePosition.x, m_eyePosition.y, m_eyePosition.z,
            adjustedDirection.x, adjustedDirection.y, adjustedDirection.z);
    }
}
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

	state.breathActive = m_bBreathActive ? 1 : 0;

	if (m_pIceBreath && m_bBreathActive)
	{
		// ブレスの現在位置と方向を送信
		D3DXVECTOR3 breathPos = m_eyePosition;

		// 調整済みの方向を計算
		D3DXVECTOR3 adjustedDirection = m_depth;
		D3DXVECTOR3 rightVec;
		D3DXVECTOR3 upVec(0.0f, 1.0f, 0.0f);
		D3DXVec3Cross(&rightVec, &upVec, &m_depth);
		D3DXVec3Normalize(&rightVec, &rightVec);

		D3DXMATRIX matRotation;
		D3DXMatrixRotationAxis(&matRotation, &rightVec, D3DXToRadian(10.0f));
		D3DXVec3TransformCoord(&adjustedDirection, &m_depth, &matRotation);
		D3DXVec3Normalize(&adjustedDirection, &adjustedDirection);

		state.breathPosX = breathPos.x;
		state.breathPosY = breathPos.y;
		state.breathPosZ = breathPos.z;
		state.breathDirX = adjustedDirection.x;
		state.breathDirY = adjustedDirection.y;
		state.breathDirZ = adjustedDirection.z;
	}
	else
	{
		// ブレスが非アクティブの場合は0で初期化
		state.breathPosX = 0.0f;
		state.breathPosY = 0.0f;
		state.breathPosZ = 0.0f;
		state.breathDirX = 0.0f;
		state.breathDirY = 0.0f;
		state.breathDirZ = -1.0f;
	}

	return state;
}

void Chaser::UpdateFromNetwork(const NetPlayerState& state, DirectionalLight& light, float deltaTime)
{
	CharacterBase::UpdateFromNetwork(state, light, deltaTime);

	D3DXVECTOR3 lightPos(state.lightPosX, state.lightPosY, state.lightPosZ);
	D3DXVECTOR3 lightDir(state.lightDirX, state.lightDirY, state.lightDirZ);

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

	m_spotLight.SetPosition(lightPos);
	m_spotLight.SetDirection(lightDir);
	m_spotLight.SetRange(lightRange);

	UpdateLightMatrices();

	if (m_pIceBreath)
	{
		bool shouldBeActive = (state.breathActive != 0);

		if (shouldBeActive && !m_bBreathActive)
		{
			//ブレスを開始
			D3DXVECTOR3 breathPos(state.breathPosX, state.breathPosY, state.breathPosZ);
			D3DXVECTOR3 breathDir(state.breathDirX, state.breathDirY, state.breathDirZ);

			//x位置が無効な場合はm_eyePositionを使用
			if (D3DXVec3Length(&breathPos) < 0.01f)
			{
				breathPos = m_eyePosition;
			}

			//方向が無効な場合はm_depthを使用
			if (D3DXVec3Length(&breathDir) < 0.01f)
			{
				breathDir = m_depth;
			}

			m_pIceBreath->Activate(breathPos, breathDir);
			m_bBreathActive = true;

			NET_LOG_F("[Chaser::UpdateFromNetwork] ID=%u ブレス開始: Pos=(%.1f,%.1f,%.1f) Dir=(%.2f,%.2f,%.2f)",
				m_clientId, breathPos.x, breathPos.y, breathPos.z,
				breathDir.x, breathDir.y, breathDir.z);
		}
		else if (!shouldBeActive && m_bBreathActive)
		{
			//ブレスを終了
			m_pIceBreath->Deactivate();
			m_bBreathActive = false;

			NET_LOG_F("[Chaser::UpdateFromNetwork] ID=%u ブレス終了", m_clientId);
		}
		else if (shouldBeActive && m_bBreathActive)
		{
			//ブレス継続中 - 位置と方向を更新
			D3DXVECTOR3 breathPos(state.breathPosX, state.breathPosY, state.breathPosZ);
			D3DXVECTOR3 breathDir(state.breathDirX, state.breathDirY, state.breathDirZ);

			if (D3DXVec3Length(&breathPos) > 0.01f)
			{
				m_pIceBreath->SetPosition(breathPos);
			}

			if (D3DXVec3Length(&breathDir) > 0.01f)
			{
				m_pIceBreath->SetDirection(breathDir);
			}

			m_pIceBreath->Update();
		}
	}

	static std::map<uint32_t, DWORD> lastLogPerChaser;
	DWORD now = timeGetTime();
	if (now - lastLogPerChaser[m_clientId] > 5000)
	{
		NET_LOG_F("[Chaser::UpdateFromNetwork] ID=%u ライト更新: Pos=(%.1f,%.1f,%.1f) Dir=(%.2f,%.2f,%.2f) Range=%.2f Breath=%s",
			m_clientId,
			lightPos.x, lightPos.y, lightPos.z,
			lightDir.x, lightDir.y, lightDir.z,
			lightRange,
			m_bBreathActive ? "Active" : "Inactive");
		lastLogPerChaser[m_clientId] = now;
	}
}

void Chaser::Draw(Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight)
{
	if (m_bIsLocal && m_bFirstPerson)
		return;

	CharacterBase::Draw(pCamera, pProj, pAmbient, pLight);
}

void Chaser::DrawEffects(Engine* pEngine, Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight)
{
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

void Chaser::CreateShadowMap(Engine* pEngine)
{
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	HRESULT hr = pDevice->CreateTexture(
		SHADOW_MAP_SIZE, SHADOW_MAP_SIZE,
		1,
		D3DUSAGE_RENDERTARGET,
		D3DFMT_A8R8G8B8,
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

	hr = m_pShadowTexture->GetSurfaceLevel(0, &m_pShadowSurface);
	if (FAILED(hr))
	{
		NET_LOG("[Chaser] シャドウマップサーフェス取得失敗");
		m_pShadowTexture->Release();
		m_pShadowTexture = nullptr;
		m_bShadowMapEnabled = false;
		return;
	}

	hr = pDevice->CreateDepthStencilSurface(
		SHADOW_MAP_SIZE, SHADOW_MAP_SIZE,
		D3DFMT_D16,
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

	CreateScaleBiasMatrix();

	m_bShadowMapEnabled = true;
	NET_LOG_F("[Chaser] シャドウマップ作成成功: ID=%u", m_clientId);
}

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

	D3DXVECTOR3 lightPos = m_eyePosition;
	D3DXVECTOR3 lightTarget = m_eyePosition + m_direction;
	D3DXVECTOR3 lightUp(0.0f, 1.0f, 0.0f);

	D3DXVECTOR3 normDir;
	D3DXVec3Normalize(&normDir, &m_direction);
	if (fabs(D3DXVec3Dot(&normDir, &lightUp)) > 0.99f)
	{
		lightUp = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
	}

	D3DXMatrixLookAtLH(&m_matLightView, &lightPos, &lightTarget, &lightUp);

	float expandedFov = m_lightFov * 2.0f;
	expandedFov = min(expandedFov, D3DXToRadian(170.0f));

	D3DXMatrixPerspectiveFovLH(&m_matLightProj, expandedFov, 1.0f, 1.0f, m_lightRange * 1.2f);

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

void Chaser::CreateScaleBiasMatrix()
{
	m_matScaleBias = D3DXMATRIX(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f
	);
}

void Chaser::CheckBreathHitPlayers(const std::vector<std::pair<uint32_t, CharacterBase*>>& players)
{
	if (!m_bBreathActive || !m_pIceBreath)
		return;

	const D3DLIGHT9& light = m_spotLight.GetLight();
	D3DXVECTOR3 lightPos(light.Position.x, light.Position.y, light.Position.z);
	D3DXVECTOR3 lightDir(light.Direction.x, light.Direction.y, light.Direction.z);
	float lightRange = light.Range;
	float lightConeAngle = light.Theta;

	static DWORD lastLog = 0;
	DWORD now = timeGetTime();
	bool shouldLog = (now - lastLog > 2000);

	if (shouldLog)
	{
		NET_LOG_F("[Chaser::CheckBreathHitPlayers] 鬼[%u] ブレス判定開始: 対象プレイヤー=%d人",
			m_clientId, (int)players.size());
		NET_LOG_F("  ライト Pos=(%.1f,%.1f,%.1f) Dir=(%.2f,%.2f,%.2f) Range=%.2f",
			lightPos.x, lightPos.y, lightPos.z,
			lightDir.x, lightDir.y, lightDir.z, lightRange);
	}

	int checkedCount = 0;
	int frozenCount = 0;

	for (const auto& pair : players)
	{
		uint32_t id = pair.first;
		CharacterBase* pChar = pair.second;

		if (id == m_clientId)
		{
			if (shouldLog)
			{
				NET_LOG_F("  [%u]: スキップ（自分）", id);
			}
			continue;
		}

		if (!pChar)
		{
			if (shouldLog)
			{
				NET_LOG_F("  [%u]: スキップ（null）", id);
			}
			continue;
		}

		Runner* pRunner = dynamic_cast<Runner*>(pChar);
		if (!pRunner)
		{
			if (shouldLog)
			{
				NET_LOG_F("  [%u]: スキップ（Runnerでない）", id);
			}
			continue;
		}

		if (pRunner->IsFrozen())
		{
			if (shouldLog)
			{
				NET_LOG_F("  [%u]: スキップ（既に凍結）", id);
			}
			continue;
		}

		checkedCount++;

		D3DXVECTOR3 playerPos = pRunner->GetCenterPosition();
		D3DXVECTOR3 toPlayer = playerPos - lightPos;
		float distance = D3DXVec3Length(&toPlayer);

		if (distance > lightRange)
		{
			if (shouldLog)
			{
				NET_LOG_F("  [%u]: 範囲外 Dist=%.2f", id, distance);
			}
			continue;
		}

		D3DXVec3Normalize(&toPlayer, &toPlayer);
		float dotProduct = D3DXVec3Dot(&toPlayer, &lightDir);
		float coneThreshold = cosf(lightConeAngle * 2.0f);

		if (dotProduct < coneThreshold)
		{
			if (shouldLog)
			{
				NET_LOG_F("  [%u]: 円錐外 Dot=%.3f", id, dotProduct);
			}
			continue;
		}

		pRunner->SetFrozen(true);
		frozenCount++;

		NET_LOG_F("[Chaser] ★★★ブレスヒット★★★ 鬼[%u] -> Runner[%u]を凍結! Dist=%.2f Dot=%.3f",
			m_clientId, id, distance, dotProduct);
	}

	if (shouldLog)
	{
		NET_LOG_F("[Chaser::CheckBreathHitPlayers] 鬼[%u] 結果: チェック=%d 凍結=%d",
			m_clientId, checkedCount, frozenCount);
		lastLog = now;
	}
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
	f_height = config["height"];
	f_eyePsoitionY = config["eyePsoitionY"];
	for (int i = 0; i < 3; i++)
	{
		f_standEyePosition[i] = config["standEyePosition"][i];
		f_crouchEyePosition[i] = config["crouchEyePosition"][i];
	}

	m_spotLight.SetDiffuse(1.0f, 1.0f, 0.0f, 0.0f);

	float value0 = config["lightAttenuation"][0];
	float value1 = config["lightAttenuation"][1];
	float value2 = config["lightAttenuation"][2];
	m_spotLight.SetAttenuation(value0, value1, value2);

	value0 = config["lightCone"][0];
	value1 = D3DXToRadian(config["lightCone"][1]);
	m_lightFov = D3DXToRadian(config["lightCone"][2]);
	m_spotLight.SetCone(value0, value1, m_lightFov);

	m_lightRange = config["lightRange"];
	m_spotLight.SetRange(m_lightRange);

	if (config.contains("breathCooldown"))
	{
		f_breathCooldown = config["breathCooldown"];
	}
	if (config.contains("breathDuration"))
	{
		m_breathDuration = config["breathDuration"];
	}
}