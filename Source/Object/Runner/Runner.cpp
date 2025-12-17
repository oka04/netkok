// Runner.cpp - 氷状態処理を追加

#define _USING_V110_SDK71_ 1

#include "Runner.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;

Runner::Runner()
	: m_stamina(100.0f)
	, m_staminaRecoveryTimer(0.0f)
	, m_bFatigued(false)
	, m_bFrozen(false)
	, m_frozenAmount(0.0f)
	, m_pIceBlock(nullptr)
	, f_meltRange(3.0f)
	, f_meltSpeed(0.2f)
	, m_targetMeltPlayer(0)
{
}

Runner::~Runner()
{
	if (m_pIceBlock)
	{
		delete m_pIceBlock;
		m_pIceBlock = nullptr;
	}
}

void Runner::Initialize(Engine* pEngine, Map& map, Projection* projection, Camera& camera, DirectionalLight& light)
{
	if (m_bIsLocal)
		SoundManager::RegisterGameObject(ID_PALYER, "Runner");

	LoadParameter();
	CharacterBase::Initialize(pEngine, MODEL_CHARACTER, projection, camera, light);
	m_position = map.GetPlayerStartPosition();
	m_targetPosition = m_position;
	pEngine->AddTexture(TEXTURE_STAMINA_GAUGE);
	m_stamina = f_maxStamina;
	m_staminaRecoveryTimer = 0.0f;
	m_bFatigued = false;

	// ★★★ 氷ブロック初期化 ★★★
	m_bFrozen = false;
	m_frozenAmount = 0.0f;
	m_targetMeltPlayer = 0;
	if (!m_pIceBlock)
	{
		m_pIceBlock = new IceBlock();
		D3DXVECTOR3 centerPos = GetCenterPosition();
		m_pIceBlock->Initialize(pEngine, 2.0f, 2.0f, 2.0f, centerPos, 0.0f);
		m_pIceBlock->SetColor(D3DXVECTOR4(0.6f, 0.85f, 1.0f, 0.7f));
	}

	UpdateMatrix(light);
}

void Runner::InitializeAtPosition(Engine* pEngine, const D3DXVECTOR3& startPos, Projection* projection, Camera& camera, DirectionalLight& light)
{
	if (m_bIsLocal)
		SoundManager::RegisterGameObject(ID_PALYER, "Runner");

	LoadParameter();
	CharacterBase::Initialize(pEngine, MODEL_CHARACTER, projection, camera, light);
	m_position = startPos;
	m_targetPosition = m_position;
	pEngine->AddTexture(TEXTURE_STAMINA_GAUGE);
	m_stamina = f_maxStamina;
	m_staminaRecoveryTimer = 0.0f;
	m_bFatigued = false;

	// ★★★ 氷ブロック初期化 ★★★
	m_bFrozen = false;
	m_frozenAmount = 0.0f;
	m_targetMeltPlayer = 0;
	if (!m_pIceBlock)
	{
		m_pIceBlock = new IceBlock();
		D3DXVECTOR3 centerPos = GetCenterPosition();
		m_pIceBlock->Initialize(pEngine, 2.0f, 2.0f, 2.0f, centerPos, 0.0f);
		m_pIceBlock->SetColor(D3DXVECTOR4(0.6f, 0.85f, 1.0f, 0.7f));
	}

	UpdateMatrix(light);
}

void Runner::Release(Engine* pEngine)
{
	pEngine->ReleaseTexture(TEXTURE_STAMINA_GAUGE);

	if (m_pIceBlock)
	{
		delete m_pIceBlock;
		m_pIceBlock = nullptr;
	}

	if (m_bIsLocal)
		SoundManager::UnregisterGameObject(ID_PALYER);
}

void Runner::Update(Engine* pEngine, Map& map, Camera& camera, DirectionalLight& light, float deltaTime)
{
	m_deltaTime = deltaTime;

	// ★★★ 凍結状態でも視点移動は可能 ★★★
	SetMouseCursor(pEngine, camera);

	// ★★★ 凍結状態の更新 ★★★
	UpdateFrozenState(deltaTime);

	// ★★★ 凍結状態でなければ通常の更新 ★★★
	if (!m_bFrozen)
	{
		Input(pEngine);
		UpdateStamina(deltaTime);
		ChangeSpeed();
		Move(map);
	}
	else
	{
		// 凍結中は移動不可
		m_speed = 0.0f;
		m_keyFlag = 0x00;  // すべてのキー入力をクリア
	}

	SetThirdPersonFromBehind(pEngine, camera, map);
	UpdateMatrix(light);
}

void Runner::SetFrozen(bool frozen)
{
	if (m_bFrozen == frozen)
		return;

	m_bFrozen = frozen;

	if (frozen)
	{
		// 凍結開始
		m_frozenAmount = 0.0f;
		NET_LOG_F("[Runner] ID=%u 凍結開始", m_clientId);
	}
	else
	{
		// 凍結解除
		m_frozenAmount = 1.0f;
		NET_LOG_F("[Runner] ID=%u 凍結解除", m_clientId);
	}
}

void Runner::UpdateFrozenState(float deltaTime)
{
	if (!m_pIceBlock)
		return;

	if (m_bFrozen)
	{
		// 氷ブロックの位置を更新
		D3DXVECTOR3 centerPos = GetCenterPosition();
		m_pIceBlock->SetPosition(centerPos);

		// 氷の溶け具合を設定（0.0 = 完全凍結, 1.0 = 完全解凍）
		m_pIceBlock->SetMeltAmount(m_frozenAmount);

		// 完全に解凍されたら凍結解除
		if (m_frozenAmount >= 1.0f)
		{
			m_bFrozen = false;
			NET_LOG_F("[Runner] ID=%u 完全解凍", m_clientId);
		}
	}
}

void Runner::TryMeltNearbyFrozenPlayer(Engine* pEngine, const std::vector<std::pair<uint32_t, CharacterBase*>>& players, float deltaTime)
{
	if (m_bFrozen)
		return;  // 自分が凍結中は解凍できない

	bool isAttackPressed = (m_keyFlag & ATTACK_KEY) != 0;

	if (!isAttackPressed)
	{
		// 左クリックを離した
		m_targetMeltPlayer = 0;
		return;
	}

	// ★★★ 範囲内の凍結プレイヤーを探す ★★★
	Runner* closestFrozenRunner = nullptr;
	float closestDistance = f_meltRange;
	uint32_t closestId = 0;

	for (const auto& pair : players)
	{
		uint32_t id = pair.first;
		CharacterBase* pChar = pair.second;

		if (!pChar || id == m_clientId)
			continue;

		Runner* pRunner = dynamic_cast<Runner*>(pChar);
		if (!pRunner || !pRunner->IsFrozen())
			continue;

		D3DXVECTOR3 diff = pRunner->GetPosition() - m_position;
		float distance = D3DXVec3Length(&diff);

		if (distance < closestDistance)
		{
			closestDistance = distance;
			closestFrozenRunner = pRunner;
			closestId = id;
		}
	}

	// ★★★ 解凍処理 ★★★
	if (closestFrozenRunner)
	{
		if (m_targetMeltPlayer != closestId)
		{
			m_targetMeltPlayer = closestId;
			NET_LOG_F("[Runner] ID=%u が ID=%u の解凍を開始", m_clientId, closestId);
		}

		// 氷を溶かす
		float currentAmount = closestFrozenRunner->GetFrozenAmount();
		float newAmount = currentAmount + f_meltSpeed * deltaTime;

		if (newAmount >= 1.0f)
		{
			newAmount = 1.0f;
			closestFrozenRunner->SetFrozen(false);
			m_targetMeltPlayer = 0;
			NET_LOG_F("[Runner] ID=%u が ID=%u を完全解凍", m_clientId, closestId);
		}

		// 手動で frozenAmount を更新（ネットワーク経由の更新を待たずに）
		if (closestFrozenRunner->m_pIceBlock)
		{
			closestFrozenRunner->m_frozenAmount = newAmount;
			closestFrozenRunner->m_pIceBlock->SetMeltAmount(newAmount);
		}
	}
	else
	{
		m_targetMeltPlayer = 0;
	}
}

NetPlayerState Runner::GetNetState() const
{
	NetPlayerState state = CharacterBase::GetNetState();

	// ★★★ 氷状態を追加 ★★★
	state.frozen = m_bFrozen ? 1 : 0;
	state.frozenAmount = m_frozenAmount;

	return state;
}

void Runner::UpdateFromNetwork(const NetPlayerState& state, DirectionalLight& light, float deltaTime)
{
	// ★★★ 重要: クライアントIDの一致を確認 ★★★
	if (state.clientId != m_clientId)
	{
		NET_LOG_F("[Runner::UpdateFromNetwork] 警告! 不正な状態適用: state.clientId=%u != m_clientId=%u",
			state.clientId, m_clientId);
		return;
	}

	CharacterBase::UpdateFromNetwork(state, light, deltaTime);

	// ★★★ 氷状態の同期（必ず自分の状態のみ適用）★★★
	bool wasFrozen = m_bFrozen;
	m_bFrozen = (state.frozen != 0);
	m_frozenAmount = state.frozenAmount;

	if (!wasFrozen && m_bFrozen)
	{
		NET_LOG_F("[Runner::UpdateFromNetwork] ID=%u 凍結開始（state.clientId=%u）", m_clientId, state.clientId);
	}
	else if (wasFrozen && !m_bFrozen)
	{
		NET_LOG_F("[Runner::UpdateFromNetwork] ID=%u 凍結解除（state.clientId=%u）", m_clientId, state.clientId);
	}

	// 氷状態が変化した場合は詳細ログ
	static std::map<uint32_t, bool> lastFrozenState;
	if (lastFrozenState[m_clientId] != m_bFrozen)
	{
		NET_LOG_F("[Runner::UpdateFromNetwork] ID=%u の氷状態が変化: %s -> %s (amount=%.2f)",
			m_clientId,
			lastFrozenState[m_clientId] ? "凍結" : "通常",
			m_bFrozen ? "凍結" : "通常",
			m_frozenAmount);
		lastFrozenState[m_clientId] = m_bFrozen;
	}
}

void Runner::Draw(Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight)
{
	if (m_bIsLocal && m_bFirstPerson)
		return;

	CharacterBase::Draw(pCamera, pProj, pAmbient, pLight);
}

void Runner::DrawDepth(Engine* pEngine, const D3DXMATRIX* pMatLightVP)
{
	m_model.DrawDepth(pEngine, pMatLightVP);
}

void Runner::DrawEffects(Engine* pEngine, Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight)
{
	// ★★★ 凍結中は氷ブロックを描画 ★★★
	if (m_bFrozen && m_pIceBlock)
	{
		m_pIceBlock->Draw(pEngine, pCamera, pProj, pAmbient, pLight);
	}
}

void Runner::DrawStaminaGauge(Engine* pEngine)
{
	if (!m_bIsLocal) return;

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

void Runner::DebugPrint(Engine* pEngine)
{
	pEngine->DrawPrintf(0, 50, FONT_GOTHIC40, Color::BLUE, "Position: %f,%f,%f", m_position.x, m_position.y, m_position.z);
	pEngine->DrawPrintf(0, 100, FONT_GOTHIC40, Color::BLUE, "depth: %f,%f,%f", m_depth.x, m_depth.y, m_depth.z);
	pEngine->DrawPrintf(0, 150, FONT_GOTHIC40, Color::BLUE, "vAngle: %f", m_vAngle);
	pEngine->DrawPrintf(0, 200, FONT_GOTHIC40, Color::BLUE, "hAngle: %f", m_hAngle);
	pEngine->DrawPrintf(0, 250, FONT_GOTHIC40, Color::BLUE, "speed: %f", m_speed);
	pEngine->DrawPrintf(0, 300, FONT_GOTHIC40, Color::BLUE, "ClientID: %u", m_clientId);
	pEngine->DrawPrintf(0, 350, FONT_GOTHIC40, Color::BLUE, "Name: %s", m_characterName.c_str());
	pEngine->DrawPrintf(0, 400, FONT_GOTHIC40, Color::BLUE, "Local: %s", m_bIsLocal ? "Yes" : "No");
	pEngine->DrawPrintf(0, 450, FONT_GOTHIC40, Color::BLUE, "Stamina: %.1f", m_stamina);
	pEngine->DrawPrintf(0, 500, FONT_GOTHIC40, Color::BLUE, "Frozen: %s (%.1f%%)",
		m_bFrozen ? "Yes" : "No", m_frozenAmount * 100.0f);
}

void Runner::UpdateStamina(float deltaTime)
{
	bool bMoving = (m_keyFlag & (W_KEY | S_KEY | D_KEY | A_KEY)) != 0;
	bool bDashing = (m_keyFlag & DASH_KEY) && bMoving;

	if (m_bFatigued)
	{
		if (m_stamina >= f_maxStamina * f_fatigueRecoveryThreshold)
		{
			m_bFatigued = false;
			m_staminaRecoveryTimer = f_recoveryDelayTime;
		}
		m_stamina += f_fatigueRecoveryRate * deltaTime;
		m_stamina = min(m_stamina, f_maxStamina);
		return;
	}

	if (bDashing)
	{
		m_stamina -= f_dashStaminaCost * deltaTime;
		m_staminaRecoveryTimer = 0.0f;
		if (m_stamina <= 0.0f)
		{
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
		m_stamina += ((bMoving) ? f_walkRecoveryRate : f_stopRecoveryRate) * deltaTime;
		m_stamina = min(m_stamina, f_maxStamina);
	}
	m_stamina = max(0.0f, m_stamina);
}

void Runner::LoadParameter()
{
	std::ifstream file(JSON_RUNNER_PARAMETER);
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
	f_headSize = config["headSize"];
	f_radius = config["radius"];
	f_maxStamina = config["maxStamina"];
	f_dashStaminaCost = config["dashStaminaCost"];
	f_walkRecoveryRate = config["walkRecoveryRate"];
	f_stopRecoveryRate = config["stopRecoveryRate"];
	f_fatigueRecoveryRate = config["fatigueRecoveryRate"];
	f_recoveryDelayTime = config["recoveryDelayTime"];
	f_fatigueSpeed = config["fatigueSpeed"];
	f_height = config["height"];
	f_fatigueRecoveryThreshold = config["fatigueRecoveryThreshold"];
	f_gaugeColorThresholds = config["gaugeColorThresholds"].get<std::vector<float>>();
	f_eyePsoitionY = config["eyePsoitionY"];

	// ★★★ 氷関連のパラメータ読み込み ★★★
	if (config.contains("meltRange"))
	{
		f_meltRange = config["meltRange"];
	}
	if (config.contains("meltSpeed"))
	{
		f_meltSpeed = config["meltSpeed"];
	}

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

void Runner::ChangeSpeed()
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