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
	, m_bFullyMelted(false)
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

	SetMouseCursor(pEngine, camera);

	UpdateFrozenState(deltaTime);

	if (!m_bFrozen)
	{
		Input(pEngine);
		UpdateStamina(deltaTime);
		ChangeSpeed();
		Move(map);
	}
	else
	{
		//凍結中は完全に動けない
		m_speed = 0.0f;

		// 氷ブロックの位置を毎フレーム更新
		if (m_pIceBlock)
		{
			m_pIceBlock->SetMeltAmount(m_frozenAmount);
		}

		static DWORD lastLog = 0;
		DWORD now = timeGetTime();
		if (now - lastLog > 2000)
		{
			NET_LOG_F("[Runner::Update] ID=%u 凍結中で動けない: amount=%.2f",
				m_clientId, m_frozenAmount);
			lastLog = now;
		}
	}

	SetThirdPersonFromBehind(pEngine, camera, map);
	UpdateMatrix(light);
}



void Runner::DrawMeltGauge(Engine* pEngine, Camera* pCamera, Projection* pProj, float viewerDistance)
{
	if (!m_bFrozen) return;

	const float MAX_GAUGE_DISTANCE = 10.0f;
	if (viewerDistance > MAX_GAUGE_DISTANCE) return;

	D3DXVECTOR3 headPos = GetCenterPosition();
	headPos.y += f_height / 2.0f + 0.5f;  
	D3DVIEWPORT9 viewport;
	pEngine->GetDevice()->GetViewport(&viewport);

	D3DXMATRIX matView = pCamera->GetViewMatrix();
	D3DXMATRIX matProj = pProj->GetProjectionMatrix();
	D3DXMATRIX matIdentity;
	D3DXMatrixIdentity(&matIdentity);

	D3DXVECTOR3 screenPos;
	D3DXVec3Project(&screenPos, &headPos, &viewport, &matProj, &matView, &matIdentity);

	if (screenPos.z > 1.0f || screenPos.z < 0.0f) return;

	int gaugeWidth = 100;
	int gaugeHeight = 10;
	int x = (int)screenPos.x - gaugeWidth / 2;
	int y = (int)screenPos.y - 20;
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	// レンダーステートの保存
	DWORD oldAlphaBlend, oldSrcBlend, oldDestBlend, oldZEnable, oldZWrite;
	pDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &oldAlphaBlend);
	pDevice->GetRenderState(D3DRS_SRCBLEND, &oldSrcBlend);
	pDevice->GetRenderState(D3DRS_DESTBLEND, &oldDestBlend);
	pDevice->GetRenderState(D3DRS_ZENABLE, &oldZEnable);
	pDevice->GetRenderState(D3DRS_ZWRITEENABLE, &oldZWrite);

	// アルファブレンドを有効化
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

	//外枠（黒、半透明）
	DrawGaugeRect(pDevice, x - 2, y - 2, gaugeWidth + 4, gaugeHeight + 4, D3DCOLOR_ARGB(200, 0, 0, 0));

	//背景（暗いグレー）
	DrawGaugeRect(pDevice, x, y, gaugeWidth, gaugeHeight, D3DCOLOR_ARGB(255, 50, 50, 50));

	//進捗バー（水色→緑へのグラデーション）
	float progress = m_frozenAmount;
	int progressWidth = (int)(gaugeWidth * progress);

	if (progressWidth > 0)
	{
		//解凍が進むにつれて色を変化させる
		int r = 100;
		int g = (int)(200 + 55 * progress);
		int b = (int)(255 - 155 * progress);
		DrawGaugeRect(pDevice, x, y, progressWidth, gaugeHeight, D3DCOLOR_ARGB(255, r, g, b));
	}

	//枠線（白）
	DrawGaugeRect(pDevice, x, y, gaugeWidth, 1, D3DCOLOR_ARGB(255, 255, 255, 255)); 
	DrawGaugeRect(pDevice, x, y + gaugeHeight - 1, gaugeWidth, 1, D3DCOLOR_ARGB(255, 255, 255, 255)); 
	DrawGaugeRect(pDevice, x, y, 1, gaugeHeight, D3DCOLOR_ARGB(255, 255, 255, 255));
	DrawGaugeRect(pDevice, x + gaugeWidth - 1, y, 1, gaugeHeight, D3DCOLOR_ARGB(255, 255, 255, 255)); 

	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, oldAlphaBlend);
	pDevice->SetRenderState(D3DRS_SRCBLEND, oldSrcBlend);
	pDevice->SetRenderState(D3DRS_DESTBLEND, oldDestBlend);
	pDevice->SetRenderState(D3DRS_ZENABLE, oldZEnable);
	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, oldZWrite);
}
void Runner::DrawMeltGaugeThroughWalls(Engine* pEngine, Camera* pCamera, Projection* pProj, float viewerDistance, float alpha)
{
	if (!m_bFrozen) return;

	D3DXVECTOR3 headPos = GetCenterPosition();
	headPos.y += f_height / 2.0f + 0.5f; 

	D3DVIEWPORT9 viewport;
	pEngine->GetDevice()->GetViewport(&viewport);

	D3DXMATRIX matView = pCamera->GetViewMatrix();
	D3DXMATRIX matProj = pProj->GetProjectionMatrix();
	D3DXMATRIX matIdentity;
	D3DXMatrixIdentity(&matIdentity);

	D3DXVECTOR3 screenPos;
	D3DXVec3Project(&screenPos, &headPos, &viewport, &matProj, &matView, &matIdentity);

	if (screenPos.x < 0 || screenPos.x > viewport.Width ||
		screenPos.y < 0 || screenPos.y > viewport.Height)
		return;

	// ゲージのサイズと位置
	int gaugeWidth = 100;
	int gaugeHeight = 10;
	int x = (int)screenPos.x - gaugeWidth / 2;
	int y = (int)screenPos.y - 20;  

	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	// レンダーステートの保存
	DWORD oldAlphaBlend, oldSrcBlend, oldDestBlend, oldZEnable, oldZWrite;
	pDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &oldAlphaBlend);
	pDevice->GetRenderState(D3DRS_SRCBLEND, &oldSrcBlend);
	pDevice->GetRenderState(D3DRS_DESTBLEND, &oldDestBlend);
	pDevice->GetRenderState(D3DRS_ZENABLE, &oldZEnable);
	pDevice->GetRenderState(D3DRS_ZWRITEENABLE, &oldZWrite);

	//壁貫通設定（Z-test無効）
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

	int alphaValue = (int)(alpha * 255);

	//外枠（黒、半透明）
	DrawGaugeRect(pDevice, x - 2, y - 2, gaugeWidth + 4, gaugeHeight + 4, D3DCOLOR_ARGB(alphaValue * 200 / 255, 0, 0, 0));

	//背景（暗いグレー）
	DrawGaugeRect(pDevice, x, y, gaugeWidth, gaugeHeight, D3DCOLOR_ARGB(alphaValue, 50, 50, 50));

	//進捗バー（水色→緑へのグラデーション）
	float progress = m_frozenAmount;  // 0.0 = 完全凍結, 1.0 = 完全解凍
	int progressWidth = (int)(gaugeWidth * progress);

	if (progressWidth > 0)
	{
		int r = 100;
		int g = (int)(200 + 55 * progress);
		int b = (int)(255 - 155 * progress);
		DrawGaugeRect(pDevice, x, y, progressWidth, gaugeHeight, D3DCOLOR_ARGB(alphaValue, r, g, b));
	}

	//枠線（白）
	DrawGaugeRect(pDevice, x, y, gaugeWidth, 1, D3DCOLOR_ARGB(alphaValue, 255, 255, 255)); 
	DrawGaugeRect(pDevice, x, y + gaugeHeight - 1, gaugeWidth, 1, D3DCOLOR_ARGB(alphaValue, 255, 255, 255));
	DrawGaugeRect(pDevice, x, y, 1, gaugeHeight, D3DCOLOR_ARGB(alphaValue, 255, 255, 255)); 
	DrawGaugeRect(pDevice, x + gaugeWidth - 1, y, 1, gaugeHeight, D3DCOLOR_ARGB(alphaValue, 255, 255, 255));
																									
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, oldAlphaBlend);
	pDevice->SetRenderState(D3DRS_SRCBLEND, oldSrcBlend);
	pDevice->SetRenderState(D3DRS_DESTBLEND, oldDestBlend);
	pDevice->SetRenderState(D3DRS_ZENABLE, oldZEnable);
	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, oldZWrite);
}
void Runner::DrawGaugeRect(LPDIRECT3DDEVICE9 pDevice, int x, int y, int width, int height, D3DCOLOR color)
{
	// 2D頂点構造体
	struct VERTEX_2D
	{
		float x, y, z, rhw;
		D3DCOLOR color;
	};

	// 矩形の4頂点を定義
	VERTEX_2D vertices[4] = {
		{ (float)x, (float)y, 0.0f, 1.0f, color },
		{ (float)(x + width), (float)y, 0.0f, 1.0f, color },
		{ (float)x, (float)(y + height), 0.0f, 1.0f, color },
		{ (float)(x + width), (float)(y + height), 0.0f, 1.0f, color },
	};

	// FVFとテクスチャを設定
	pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	pDevice->SetTexture(0, NULL);

	// トライアングルストリップで矩形を描画
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(VERTEX_2D));
}


void Runner::SetFrozen(bool frozen)
{
	if (m_bFrozen == frozen)
		return;

	m_bFrozen = frozen;

	if (frozen)
	{
		m_frozenAmount = 0.0f;
		m_bFullyMelted = false;  

		if (m_pIceBlock)
		{
			D3DXVECTOR3 centerPos = GetCenterPosition();
			m_pIceBlock->SetPosition(centerPos);
			m_pIceBlock->SetMeltAmount(0.0f);
		}

		NET_LOG_F("[Runner] ID=%u 凍結開始 Pos=(%.1f,%.1f,%.1f)",
			m_clientId, m_position.x, m_position.y, m_position.z);
	}
	else
	{
		m_frozenAmount = 1.0f;
		m_bFullyMelted = true;  

		if (m_pIceBlock)
		{
			m_pIceBlock->SetMeltAmount(1.0f);
		}

		NET_LOG_F("[Runner] ID=%u 凍結解除（完全解凍）", m_clientId);
	}
}

void Runner::SetFrozenAmount(float amount)
{
	m_frozenAmount = max(0.0f, min(1.0f, amount));

	if (m_pIceBlock)
	{
		m_pIceBlock->SetMeltAmount(m_frozenAmount);
	}

	// ★★★ デバッグ: 設定後の値を確認 ★★★
	static DWORD lastLog = 0;
	DWORD now = timeGetTime();
	if (now - lastLog > 200)
	{
		NET_LOG_F("[Runner::SetFrozenAmount] ID=%u amount設定: %.3f -> %.3f",
			m_clientId, amount, m_frozenAmount);
		lastLog = now;
	}

	// ★★★ 重要: 1.0に達したら凍結解除 ★★★
	if (m_frozenAmount >= 1.0f && m_bFrozen)
	{
		m_bFrozen = false;
		m_bFullyMelted = true;
		NET_LOG_F("[Runner::SetFrozenAmount] ID=%u 完全解凍（SetFrozenAmount経由）", m_clientId);
	}
}

void Runner::UpdateFrozenState(float deltaTime)
{
	if (!m_pIceBlock)
		return;

	if (m_bFrozen)
	{
		D3DXVECTOR3 centerPos = GetCenterPosition();
		m_pIceBlock->SetPosition(centerPos);

		m_pIceBlock->SetMeltAmount(m_frozenAmount);

		m_pIceBlock->Update(deltaTime);

		if (m_frozenAmount >= 1.0f)
		{
			m_bFrozen = false;
			NET_LOG_F("[Runner] ID=%u 完全解凍", m_clientId);
		}

		static std::map<uint32_t, DWORD> lastLog;
		DWORD now = timeGetTime();
		if (now - lastLog[m_clientId] > 2000)
		{
			NET_LOG_F("[Runner] ID=%u 凍結中: amount=%.2f Pos=(%.1f,%.1f,%.1f)",
				m_clientId, m_frozenAmount, centerPos.x, centerPos.y, centerPos.z);
			lastLog[m_clientId] = now;
		}
	}
}

void Runner::UpdateMeltTarget(const std::vector<std::pair<uint32_t, CharacterBase*>>& players)
{
	// ★★★ 自分が凍結中はターゲットをクリア ★★★
	if (m_bFrozen)
	{
		if (m_targetMeltPlayer != 0)
		{
			m_targetMeltPlayer = 0;
			NET_LOG_F("[Runner::UpdateMeltTarget] ID=%u 自分が凍結中なのでターゲットクリア", m_clientId);
		}
		return;
	}

	// ★★★ 左クリックの状態を確認 ★★★
	bool isAttackPressed = (m_keyFlag & ATTACK_KEY) != 0;

	static DWORD lastClickLog = 0;
	DWORD now = timeGetTime();
	if (isAttackPressed && now - lastClickLog > 1000)
	{
		NET_LOG_F("[Runner::UpdateMeltTarget] ★★★ ID=%u 左クリック検出！ KeyFlag=0x%02X ★★★",
			m_clientId, m_keyFlag);
		lastClickLog = now;
	}

	// ★★★ 左クリックが離されたらターゲットクリア ★★★
	if (!isAttackPressed)
	{
		if (m_targetMeltPlayer != 0)
		{
			NET_LOG_F("[Runner::UpdateMeltTarget] ID=%u 左クリックを離した - 解凍中止 (ターゲット=%u)",
				m_clientId, m_targetMeltPlayer);
			m_targetMeltPlayer = 0;
		}
		return;
	}

	// ★★★ 範囲内の凍結したRunnerを探す ★★★
	Runner* closestFrozenRunner = nullptr;
	float closestDistance = f_meltRange;
	uint32_t closestId = 0;

	int checkedCount = 0;
	int frozenCount = 0;

	for (const auto& pair : players)
	{
		uint32_t id = pair.first;
		CharacterBase* pChar = pair.second;

		// ★★★ 自分はスキップ ★★★
		if (id == m_clientId)
		{
			continue;
		}

		if (!pChar)
		{
			continue;
		}

		Runner* pRunner = dynamic_cast<Runner*>(pChar);
		if (!pRunner)
		{
			continue;
		}

		checkedCount++;

		// ★★★ 凍結していないプレイヤーはスキップ ★★★
		if (!pRunner->IsFrozen())
		{
			continue;
		}

		frozenCount++;

		D3DXVECTOR3 diff = pRunner->GetPosition() - m_position;
		float distance = D3DXVec3Length(&diff);

		// ★★★ 範囲内で最も近いプレイヤーを記憶 ★★★
		if (distance < closestDistance)
		{
			closestDistance = distance;
			closestFrozenRunner = pRunner;
			closestId = id;
		}
	}

	// ★★★ デバッグログ ★★★
	if (now - lastClickLog > 1000 && isAttackPressed)
	{
		NET_LOG_F("[Runner::UpdateMeltTarget] ID=%u 検索結果: チェック=%d 凍結=%d 最短距離=%.2f 最短ID=%u",
			m_clientId, checkedCount, frozenCount, closestDistance, closestId);
	}

	// ★★★ ターゲットを設定 ★★★
	if (closestFrozenRunner)
	{
		if (m_targetMeltPlayer != closestId)
		{
			m_targetMeltPlayer = closestId;
			NET_LOG_F("[Runner] ★★★ID=%u が ID=%u の解凍ターゲットに設定★★★ Distance=%.2f",
				m_clientId, closestId, closestDistance);
		}
	}
	else
	{
		if (m_targetMeltPlayer != 0)
		{
			NET_LOG_F("[Runner::UpdateMeltTarget] ID=%u 範囲内に凍結プレイヤーなし - 解凍中止 (前回ターゲット=%u)",
				m_clientId, m_targetMeltPlayer);
		}
		m_targetMeltPlayer = 0;
	}
}

NetPlayerState Runner::GetNetState() const
{
	NetPlayerState state = CharacterBase::GetNetState();

	state.frozen = m_bFrozen ? 1 : 0;
	state.frozenAmount = m_frozenAmount;
	state.meltTargetId = m_targetMeltPlayer; 

											 
	static DWORD lastLog = 0;
	DWORD now = timeGetTime();
	if (m_targetMeltPlayer != 0 && now - lastLog > 1000)
	{
		NET_LOG_F("[Runner::GetNetState] ★ID=%u が meltTarget=%u を送信★ Frozen=%d Amount=%.2f",
			m_clientId, m_targetMeltPlayer, state.frozen, state.frozenAmount);
		lastLog = now;
	}

	return state;
}

void Runner::UpdateMeltTarget(Engine* pEngine, const std::vector<std::pair<uint32_t, CharacterBase*>>& players, float deltaTime)
{
	if (m_bFrozen)
	{
		if (m_targetMeltPlayer != 0)
		{
			m_targetMeltPlayer = 0;
			NET_LOG_F("[Runner::UpdateMeltTarget] ID=%u 自分が凍結中なので解凍中止", m_clientId);
		}
		return;
	}

	bool isAttackPressed = (m_keyFlag & ATTACK_KEY) != 0;

	static DWORD lastClickLog = 0;
	DWORD now = timeGetTime();
	if (isAttackPressed && now - lastClickLog > 1000)
	{
		NET_LOG_F("[Runner::UpdateMeltTarget] ★★★ ID=%u 左クリック検出！ KeyFlag=0x%02X ★★★", m_clientId, m_keyFlag);
		lastClickLog = now;
	}

	if (!isAttackPressed)
	{
		if (m_targetMeltPlayer != 0)
		{
			NET_LOG_F("[Runner::UpdateMeltTarget] ID=%u 左クリックを離した - 解凍中止 (ターゲット=%u)",
				m_clientId, m_targetMeltPlayer);
			m_targetMeltPlayer = 0;
		}
		return;
	}

	Runner* closestFrozenRunner = nullptr;
	float closestDistance = f_meltRange;
	uint32_t closestId = 0;

	int checkedCount = 0;
	int frozenCount = 0;

	for (const auto& pair : players)
	{
		uint32_t id = pair.first;
		CharacterBase* pChar = pair.second;

		if (id == m_clientId)
		{
			continue;
		}

		if (!pChar)
		{
			continue;
		}

		Runner* pRunner = dynamic_cast<Runner*>(pChar);
		if (!pRunner)
		{
			continue;
		}

		checkedCount++;

		if (!pRunner->IsFrozen())
		{
			continue;
		}

		frozenCount++;

		D3DXVECTOR3 diff = pRunner->GetPosition() - m_position;
		float distance = D3DXVec3Length(&diff);

		if (distance < closestDistance)
		{
			closestDistance = distance;
			closestFrozenRunner = pRunner;
			closestId = id;
		}
	}

	if (now - lastClickLog > 1000 && isAttackPressed)
	{
		NET_LOG_F("[Runner::UpdateMeltTarget] ID=%u 検索結果: チェック=%d 凍結=%d 最短距離=%.2f 最短ID=%u",
			m_clientId, checkedCount, frozenCount, closestDistance, closestId);
	}

	if (closestFrozenRunner)
	{
		if (m_targetMeltPlayer != closestId)
		{
			m_targetMeltPlayer = closestId;
			NET_LOG_F("[Runner] ★★★ID=%u が ID=%u の解凍ターゲットに設定★★★ Distance=%.2f",
				m_clientId, closestId, closestDistance);
		}
	}
	else
	{
		if (m_targetMeltPlayer != 0)
		{
			NET_LOG_F("[Runner::UpdateMeltTarget] ID=%u 範囲内に凍結プレイヤーなし - 解凍中止 (前回ターゲット=%u)",
				m_clientId, m_targetMeltPlayer);
		}
		m_targetMeltPlayer = 0;
	}
}
void Runner::UpdateFromNetwork(const NetPlayerState& state, DirectionalLight& light, float deltaTime)
{
	CharacterBase::UpdateFromNetwork(state, light, deltaTime);

	bool wasFrozen = m_bFrozen;
	bool newFrozen = (state.frozen != 0);
	float netAmount = state.frozenAmount;

	// ★★★ リモートプレイヤーの場合、meltTargetを更新 ★★★
	if (!m_bIsLocal)
	{
		m_targetMeltPlayer = state.meltTargetId;
	}

	// ★★★ 新規凍結 ★★★
	if (!wasFrozen && newFrozen)
	{
		m_bFrozen = true;
		m_frozenAmount = netAmount;
		m_bFullyMelted = false;

		if (m_pIceBlock)
		{
			D3DXVECTOR3 centerPos = GetCenterPosition();
			m_pIceBlock->SetPosition(centerPos);
			m_pIceBlock->SetMeltAmount(m_frozenAmount);
		}

		NET_LOG_F("[Runner::UpdateFromNetwork] ID=%u 新規凍結 amount=%.2f",
			m_clientId, m_frozenAmount);
	}
	// ★★★ 凍結継続中の更新 ★★★
	else if (wasFrozen && newFrozen)
	{
		float oldAmount = m_frozenAmount;

		// ★★★ 重要: 解凍は進行のみ許可（後退は許可しない）★★★
		if (netAmount > m_frozenAmount)
		{
			m_frozenAmount = netAmount;

			if (m_pIceBlock)
			{
				m_pIceBlock->SetMeltAmount(m_frozenAmount);
			}

			// ★★★ デバッグ: 解凍の進行を確認 ★★★
			static std::map<uint32_t, DWORD> lastLog;
			DWORD now = timeGetTime();
			if (now - lastLog[m_clientId] > 200)
			{
				NET_LOG_F("[Runner::UpdateFromNetwork] ID=%u 解凍更新: %.3f -> %.3f",
					m_clientId, oldAmount, netAmount);
				lastLog[m_clientId] = now;
			}
		}

		// ★★★ 完全解凍の判定 ★★★
		if (m_frozenAmount >= 1.0f)
		{
			m_bFrozen = false;
			m_bFullyMelted = true;
			NET_LOG_F("[Runner::UpdateFromNetwork] ID=%u 完全解凍", m_clientId);
		}
	}
	// ★★★ 凍結解除 ★★★
	else if (wasFrozen && !newFrozen)
	{
		m_bFrozen = false;
		m_frozenAmount = 1.0f;
		m_bFullyMelted = true;

		if (m_pIceBlock)
		{
			m_pIceBlock->SetMeltAmount(1.0f);
		}

		NET_LOG_F("[Runner::UpdateFromNetwork] ID=%u 凍結解除", m_clientId);
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
	if (m_bFrozen && m_pIceBlock)
	{
		D3DXVECTOR3 centerPos = GetCenterPosition();
		m_pIceBlock->SetPosition(centerPos);
		m_pIceBlock->SetMeltAmount(m_frozenAmount);

		m_pIceBlock->Draw(pEngine, pCamera, pProj, pAmbient, pLight);

		static std::map<uint32_t, DWORD> lastLog;
		DWORD now = timeGetTime();
		if (now - lastLog[m_clientId] > 2000)
		{
			NET_LOG_F("[Runner::DrawEffects] ID=%u 氷ブロック描画: Frozen=%d amount=%.2f Pos=(%.1f,%.1f,%.1f)",
				m_clientId, m_bFrozen, m_frozenAmount, centerPos.x, centerPos.y, centerPos.z);
			lastLog[m_clientId] = now;
		}
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
	pEngine->DrawPrintf(0, 550, FONT_GOTHIC40, Color::BLUE, "MeltTarget: %u", m_targetMeltPlayer);
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

	f_meltRange = config["meltRange"];
	f_meltSpeed = config["meltSpeed"];

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