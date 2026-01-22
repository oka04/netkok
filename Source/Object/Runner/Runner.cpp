// Runner.cpp - 解凍同期の改善版
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
	, m_meltingSoundId(AK_INVALID_PLAYING_ID)
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
	m_position = map.GetRunnerStartPosition();
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

	// ★★★ 追加: 解凍音を停止 ★★★
	if (m_meltingSoundId != AK_INVALID_PLAYING_ID)
	{
		SoundManager::StopEvent(m_meltingSoundId);
		m_meltingSoundId = AK_INVALID_PLAYING_ID;
		NET_LOG_F("[Runner::Release] 解凍音停止: ID=%u", m_clientId);
	}

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

		// ★★★ ローカルプレイヤーの凍結状態を音に反映 ★★★
		if (m_bIsLocal)
		{
			SoundManager::UpdatePlayerState(false);
		}
	}
	else
	{
		// 凍結中は完全に動けない
		m_speed = 0.0f;

		// ★★★ ローカルプレイヤーの凍結状態を音に反映 ★★★
		if (m_bIsLocal)
		{
			SoundManager::UpdatePlayerState(true);
		}

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

	//枠線（白、半透明）
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
	struct CUSTOMVERTEX
	{
		float x, y, z, rhw;
		D3DCOLOR color;
	};

	CUSTOMVERTEX vertices[4] =
	{
		{ (float)x,         (float)y,          0.0f, 1.0f, color },
		{ (float)(x + width), (float)y,          0.0f, 1.0f, color },
		{ (float)x,         (float)(y + height), 0.0f, 1.0f, color },
		{ (float)(x + width), (float)(y + height), 0.0f, 1.0f, color }
	};

	pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	pDevice->SetTexture(0, NULL);
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(CUSTOMVERTEX));
}

void Runner::SetFrozen(bool frozen)
{
	bool wasFullyMelted = m_bFullyMelted;
	bool wasFrozen = m_bFrozen;

	m_bFrozen = frozen;

	if (frozen && !wasFrozen)
	{
		// ★★★ 新規凍結 ★★★
		m_frozenAmount = 0.0f;
		m_bFullyMelted = false;  // ★★★ 重要: 完全解凍フラグをリセット ★★★
		m_targetMeltPlayer = 0;

		if (m_pIceBlock)
		{
			m_pIceBlock->SetMeltAmount(0.0f);
		}

		NET_LOG_F("[Runner::SetFrozen] ID=%u 新規凍結", m_clientId);
	}
	else if (!frozen && wasFrozen)
	{
		// ★★★ 凍結解除 ★★★
		m_frozenAmount = 1.0f;
		m_bFullyMelted = true;
		m_targetMeltPlayer = 0;

		if (m_pIceBlock)
		{
			m_pIceBlock->SetMeltAmount(1.0f);
		}

		NET_LOG_F("[Runner::SetFrozen] ID=%u 凍結解除 amount=1.0", m_clientId);
	}
	else if (!frozen && !wasFrozen)
	{
		// ★★★ 解凍完了状態を維持 ★★★
		if (m_frozenAmount < 1.0f)
		{
			m_frozenAmount = 1.0f;
		}
		m_bFullyMelted = true;

		if (m_pIceBlock)
		{
			m_pIceBlock->SetMeltAmount(1.0f);
		}
	}
}


void Runner::SetFrozenAmount(float amount)
{
	float oldAmount = m_frozenAmount;
	m_frozenAmount = amount;

	// ★★★ 完全解凍の判定 ★★★
	if (m_frozenAmount >= 1.0f && m_bFrozen)
	{
		m_frozenAmount = 1.0f;
		m_bFrozen = false;
		m_bFullyMelted = true;
		m_targetMeltPlayer = 0;

		NET_LOG_F("[Runner::SetFrozenAmount] ID=%u 完全解凍 amount=1.0維持", m_clientId);
	}
	// ★★★ 解凍完了後は1.0を維持 ★★★
	else if (m_frozenAmount >= 1.0f && !m_bFrozen)
	{
		m_frozenAmount = 1.0f;
		m_bFullyMelted = true;
	}

	// ★★★ 氷ブロックの更新 ★★★
	if (m_pIceBlock)
	{
		m_pIceBlock->SetMeltAmount(m_frozenAmount);
	}

	// ★★★ デバッグログ（変化があった場合のみ）★★★
	static std::map<uint32_t, DWORD> lastLog;
	DWORD now = timeGetTime();
	if (abs(oldAmount - m_frozenAmount) > 0.01f && now - lastLog[m_clientId] > 500)
	{
		NET_LOG_F("[Runner::SetFrozenAmount] ID=%u amount: %.3f -> %.3f (frozen=%d)",
			m_clientId, oldAmount, m_frozenAmount, m_bFrozen);
		lastLog[m_clientId] = now;
	}
}


void Runner::UpdateFrozenState(float deltaTime)
{
	if (m_bFrozen)
	{
		static DWORD lastLog = 0;
		DWORD now = timeGetTime();
		if (now - lastLog > 2000)
		{
			NET_LOG_F("[Runner::UpdateFrozenState] ID=%u 凍結中: amount=%.2f",
				m_clientId, m_frozenAmount);
			lastLog = now;
		}
	}
}
void Runner::UpdateMeltTarget(const std::vector<std::pair<uint32_t, CharacterBase*>>& players)
{
	if (!m_bIsLocal || m_bFrozen)
	{
		// 自分が凍っているかローカルでなければ音を止める
		if (m_meltingSoundId != AK_INVALID_PLAYING_ID)
		{
			SoundManager::StopEvent(m_meltingSoundId);
			m_meltingSoundId = AK_INVALID_PLAYING_ID;
		}
		m_targetMeltPlayer = 0;
		return;
	}

	bool isHelping = (m_keyFlag & ATTACK_KEY) != 0;

	// ボタンを離したら即座に音を止める
	if (!isHelping)
	{
		if (m_meltingSoundId != AK_INVALID_PLAYING_ID)
		{
			SoundManager::StopEvent(m_meltingSoundId);
			m_meltingSoundId = AK_INVALID_PLAYING_ID;
		}
		m_targetMeltPlayer = 0;
		return;
	}

	uint32_t newTarget = 0;
	D3DXVECTOR3 myPos = GetCenterPosition();
	float minDist = f_meltRange;

	for (const auto& kv : players)
	{
		if (kv.first == m_clientId) continue;
		Runner* other = dynamic_cast<Runner*>(kv.second);
		if (!other || !other->IsFrozen()) continue;

		D3DXVECTOR3 otherPos = other->GetCenterPosition();
		D3DXVECTOR3 diff = otherPos - myPos;
		float dist = D3DXVec3Length(&diff);

		if (dist < minDist)
		{
			minDist = dist;
			newTarget = kv.first;
		}
	}

	// ターゲットが変わった場合
	if (newTarget != m_targetMeltPlayer)
	{
		// 前の音を止める
		if (m_meltingSoundId != AK_INVALID_PLAYING_ID)
		{
			SoundManager::StopEvent(m_meltingSoundId);
			m_meltingSoundId = AK_INVALID_PLAYING_ID;
		}

		// 新しいターゲットがいれば再生
		if (newTarget != 0)
		{
			SoundManager::SetPosition(m_position, m_depth, UP_DIRECTION, m_clientId);
			// ここで一度だけ再生
			m_meltingSoundId = SoundManager::Play(AK::EVENTS::PLAY_SE_THAWING, m_clientId);
		}
		m_targetMeltPlayer = newTarget;
	}
	else if (m_targetMeltPlayer != 0)
	{
		// ターゲットが同じなら位置更新だけ
		SoundManager::SetPosition(m_position, m_depth, UP_DIRECTION, m_clientId);

		// 万が一音が止まっていたら再開（保険）
		if (m_meltingSoundId == AK_INVALID_PLAYING_ID)
		{
			m_meltingSoundId = SoundManager::Play(AK::EVENTS::PLAY_SE_THAWING, m_clientId);
		}
	}
}

NetPlayerState Runner::GetNetState() const
{
	NetPlayerState state = CharacterBase::GetNetState();
	state.frozen = m_bFrozen ? 1 : 0;
	state.frozenAmount = m_frozenAmount;
	state.meltTargetId = m_targetMeltPlayer;
	return state;
}

void Runner::UpdateFromNetwork(const NetPlayerState& state, DirectionalLight& light, float deltaTime)
{
	CharacterBase::UpdateFromNetwork(state, light, deltaTime);

	bool wasFrozen = m_bFrozen;
	bool wasFullyMelted = m_bFullyMelted;
	bool newFrozen = (state.frozen != 0);
	float netAmount = state.frozenAmount;

	static std::map<uint32_t, DWORD> lastLogTime;
	static std::map<uint32_t, AkPlayingID> meltingSounds;
	DWORD now = timeGetTime();

	if (!wasFrozen && wasFullyMelted)
	{
		if (newFrozen && netAmount < 0.1f)
		{
			NET_LOG_F("[Runner::UpdateFromNetwork] ID=%u 完全解凍後の新規凍結を検出！ amount=%.3f",
				m_clientId, netAmount);
		}
		else if (newFrozen && netAmount > 0.5f)
		{
			static std::map<uint32_t, DWORD> lastIgnoreLog;
			if (now - lastIgnoreLog[m_clientId] > 1000)
			{
				NET_LOG_F("[Runner::UpdateFromNetwork] ID=%u 完全解凍済み - 古い凍結状態を無視 (amount=%.3f)",
					m_clientId, netAmount);
				lastIgnoreLog[m_clientId] = now;
			}
			return;
		}
		else if (!newFrozen)
		{
			if (m_frozenAmount < 1.0f)
			{
				m_frozenAmount = 1.0f;
				if (m_pIceBlock)
				{
					m_pIceBlock->SetMeltAmount(1.0f);
				}
			}
			return;
		}
	}

	if (!wasFrozen && newFrozen)
	{
		m_bFrozen = true;
		m_frozenAmount = netAmount;
		m_bFullyMelted = false;
		m_targetMeltPlayer = 0;

		if (m_pIceBlock)
		{
			m_pIceBlock->SetMeltAmount(m_frozenAmount);
		}

		// ★★★ 凍結音を確実に再生 ★★★
		SoundManager::SetPosition(m_position, m_depth, UP_DIRECTION, m_clientId);
		AkPlayingID freezeId = SoundManager::Play(AK::EVENTS::PLAY_SE_FREEZE, m_clientId);

		NET_LOG_F("[Runner::UpdateFromNetwork] ID=%u ★新規凍結★ 凍結音再生 PlayingID=%u amount=%.3f",
			m_clientId, freezeId, netAmount);

		if (freezeId == AK_INVALID_PLAYING_ID)
		{
			NET_LOG_F("[Runner::UpdateFromNetwork] ★★★エラー★★★ ID=%u 凍結音再生失敗！", m_clientId);
		}
	}
	else if (wasFrozen && newFrozen)
	{
		float oldAmount = m_frozenAmount;

		if (abs(netAmount - m_frozenAmount) > 0.001f)
		{
			m_frozenAmount = netAmount;

			if (m_pIceBlock)
			{
				m_pIceBlock->SetMeltAmount(m_frozenAmount);
			}

			// ★★★ 修正: 解凍が進んでいる場合のみ音を再生 ★★★
			if (m_frozenAmount > oldAmount)
			{
				if (meltingSounds[m_clientId] == AK_INVALID_PLAYING_ID)
				{
					SoundManager::SetPosition(m_position, m_depth, UP_DIRECTION, m_clientId);
					meltingSounds[m_clientId] = SoundManager::Play(AK::EVENTS::PLAY_SE_THAWING, m_clientId);
					NET_LOG_F("[Runner::UpdateFromNetwork] ID=%u 解凍音再生開始 PlayingID=%u",
						m_clientId, meltingSounds[m_clientId]);
				}
			}

			static std::map<uint32_t, DWORD> lastLog;
			if (now - lastLog[m_clientId] > 200)
			{
				NET_LOG_F("[Runner::UpdateFromNetwork] ID=%u 解凍更新: %.3f -> %.3f",
					m_clientId, oldAmount, netAmount);
				lastLog[m_clientId] = now;
			}

			if (m_frozenAmount >= 1.0f)
			{
				m_bFrozen = false;
				m_bFullyMelted = true;
				m_frozenAmount = 1.0f;
				m_targetMeltPlayer = 0;

				// ★★★ 解凍完了音を再生 ★★★
				SoundManager::SetPosition(m_position, m_depth, UP_DIRECTION, m_clientId);
				AkPlayingID completeId = SoundManager::Play(AK::EVENTS::PLAY_SE_THAW_COMPLETE, m_clientId);

				// ★★★ 解凍中の音を停止 ★★★
				if (meltingSounds[m_clientId] != AK_INVALID_PLAYING_ID)
				{
					SoundManager::StopEvent(meltingSounds[m_clientId]);
					meltingSounds[m_clientId] = AK_INVALID_PLAYING_ID;
				}

				NET_LOG_F("[Runner::UpdateFromNetwork] ID=%u ★完全解凍★ 完了音再生 PlayingID=%u",
					m_clientId, completeId);
			}
		}
		else
		{
			// ★★★ 新規追加: 解凍が進んでいない（誰も助けていない）場合、音を止める ★★★
			if (meltingSounds[m_clientId] != AK_INVALID_PLAYING_ID)
			{
				SoundManager::StopEvent(meltingSounds[m_clientId]);
				meltingSounds[m_clientId] = AK_INVALID_PLAYING_ID;

				NET_LOG_F("[Runner::UpdateFromNetwork] ID=%u ★解凍音停止★ (誰も助けていない) amount=%.3f",
					m_clientId, m_frozenAmount);
			}
		}
	}
	else if (wasFrozen && !newFrozen)
	{
		m_bFrozen = false;
		m_frozenAmount = 1.0f;
		m_bFullyMelted = true;
		m_targetMeltPlayer = 0;

		if (m_pIceBlock)
		{
			m_pIceBlock->SetMeltAmount(m_frozenAmount);
		}

		// ★★★ 解凍完了音を再生 ★★★
		SoundManager::SetPosition(m_position, m_depth, UP_DIRECTION, m_clientId);
		AkPlayingID completeId = SoundManager::Play(AK::EVENTS::PLAY_SE_THAW_COMPLETE, m_clientId);

		// ★★★ 解凍中の音を停止 ★★★
		if (meltingSounds[m_clientId] != AK_INVALID_PLAYING_ID)
		{
			SoundManager::StopEvent(meltingSounds[m_clientId]);
			meltingSounds[m_clientId] = AK_INVALID_PLAYING_ID;
		}

		NET_LOG_F("[Runner::UpdateFromNetwork] ID=%u 凍結解除 完了音再生 PlayingID=%u",
			m_clientId, completeId);
	}

	if (state.meltTargetId != m_targetMeltPlayer)
	{
		static std::map<uint32_t, DWORD> lastMeltLog;
		if (now - lastMeltLog[m_clientId] > 1000)
		{
			NET_LOG_F("[Runner::UpdateFromNetwork] ID=%u meltTarget更新: %u -> %u",
				m_clientId, m_targetMeltPlayer, state.meltTargetId);
			lastMeltLog[m_clientId] = now;
		}
		m_targetMeltPlayer = state.meltTargetId;
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