#define _USING_V110_SDK71_ 1

#include "CharacterBase.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;

const D3DXVECTOR3 CharacterBase::UP_DIRECTION = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
const D3DXVECTOR3 CharacterBase::DEPTH_DIRECTION = D3DXVECTOR3(0.0f, 0.0f, -1.0f);

//初期化
// CharacterBase.cpp - 初期化順序を修正

void CharacterBase::Initialize(Engine *pEngine, std::string filename, Projection* projection, Camera& camera, DirectionalLight &light)
{
	m_model.SetModel(pEngine->GetModel(filename));
	m_angle = D3DXToRadian(90.0f);
	m_direction = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	m_depth = DEPTH_DIRECTION;
	m_cameraFront = m_depth;

	m_speed = 0;

	m_hAngle = 0.0f;
	m_vAngle = 0.0f;
	m_fov = projection->GetFov();
	m_bFirstPerson = true;

	m_keyFlag = 0x00;

	// ★★★ 重要: ネットワーク関連の初期化時にIDをリセットしない ★★★
	// m_clientId と m_characterName は SpawnPlayerWithRole() で既に設定済み
	// ここでリセットすると Wwise 登録が無効になる

	// m_clientId = 0;  // ★★★ コメントアウト！
	// m_characterName = "";  // ★★★ コメントアウト！

	// ★★★ m_bIsLocal だけは初期化時に設定されていない場合のみデフォルト値を設定 ★★★
	// （既に SetIsLocal() で設定済みの場合は上書きしない）
	// m_bIsLocal = true;  // ★★★ コメントアウト！

	m_targetPosition = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	m_targetHAngle = 0.0f;
	m_targetVAngle = 0.0f;
	m_interpolationSpeed = 30.0f;
	m_adaptiveInterpolationSpeed = 30.0f;
	m_velocity = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	m_predictedPosition = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	m_smoothedVelocity = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	m_velocitySmoothingFactor = 0.3f;
	m_positionHistoryIndex = 0;
	m_positionHistoryCount = 0;
	m_lastUpdateTime = 0;
	m_timeSinceLastUpdate = 0.0f;
	m_soundEvents = 0;
	for (int i = 0; i < MAX_POSITION_HISTORY; i++)
	{
		m_positionHistory[i] = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	}

	SetMouseCursor(pEngine, camera);
	UpdateMatrix(light);
}

void CharacterBase::UpdateMatrix(DirectionalLight &light)
{
	// ★★★ ローカルプレイヤーのみフラグをクリア（リモートは保持） ★★★
	if (m_bIsLocal)
	{
		m_soundEvents = 0;
	}

	float currentAngle = D3DXToRadian(m_hAngle);

	D3DXVECTOR3 baseDirection;

	baseDirection = DEPTH_DIRECTION;
	m_angle = currentAngle;

	D3DXMATRIX matRotationY;
	D3DXMatrixRotationY(&matRotationY, currentAngle);

	m_depth = D3DXVec3TransformCoord(&baseDirection, &matRotationY);

	m_hori = D3DXVec3Cross(&UP_DIRECTION, &m_depth);
	D3DXVec3Normalize(&m_hori, &m_hori);

	D3DXMATRIX matRotationV;
	D3DXMatrixRotationAxis(&matRotationV, &m_hori, D3DXToRadian(m_vAngle));
	m_cameraFront = D3DXVec3TransformCoord(&m_depth, &matRotationV);

	light.SetDirection(m_depth);

	// ★★★ ローカルプレイヤーのみサウンド位置を更新 ★★★
	if (m_bIsLocal)
	{
		SoundManager::SetPosition(m_position, m_depth, UP_DIRECTION, SoundManager::ID_LISTENER);
	}
	else
	{
		// ★★★ リモートプレイヤーは自分のIDで位置更新 ★★★
		SoundManager::SetPosition(m_position, m_depth, UP_DIRECTION, m_clientId);
	}

	D3DXMatrixRotationY(&m_matRotate, m_angle);
	D3DXMatrixTranslation(&m_matTrans, &m_position);

	m_matWorld = m_matRotate * m_matTrans;

	m_model.SetWorldTransform(&m_matWorld);
}

CharacterBase::CharacterBase()
	: m_seFootId(AK_INVALID_PLAYING_ID)
{
}

//描画
void CharacterBase::Draw(Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight)
{
	m_model.Draw(pCamera, pProj, pAmbient, pLight);
}

void CharacterBase::Input(Engine * pEngine)
{
	if (pEngine->GetKeyState(DIK_W))
	{
		m_keyFlag |= W_KEY;
	}
	else
	{
		m_keyFlag &= ~W_KEY;
	}

	if (pEngine->GetKeyState(DIK_D))
	{
		m_keyFlag |= D_KEY;
	}
	else
	{
		m_keyFlag &= ~D_KEY;
	}

	if (pEngine->GetKeyState(DIK_S))
	{
		m_keyFlag |= S_KEY;
	}
	else
	{
		m_keyFlag &= ~S_KEY;
	}

	if (pEngine->GetKeyState(DIK_A))
	{
		m_keyFlag |= A_KEY;
	}
	else
	{
		m_keyFlag &= ~A_KEY;
	}

	if (pEngine->GetKeyState(DIK_LCONTROL) || pEngine->GetKeyState(DIK_C))
	{
		m_keyFlag |= CROUCH_KEY;
	}
	else
	{
		m_keyFlag &= ~CROUCH_KEY;
	}

	if (pEngine->GetKeyState(DIK_LSHIFT))
	{
		m_keyFlag |= DASH_KEY;
	}
	else
	{
		m_keyFlag &= ~DASH_KEY;
	}

	// ★★★ 左クリック攻撃入力 - 修正版 ★★★
	// 0 は左ボタンのインデックス（DIMOFS_BUTTON0）
	if (pEngine->GetMouseButton(0))
	{
		m_keyFlag |= ATTACK_KEY;

		// デバッグログ（頻度を下げる）
		static DWORD lastLog = 0;
		DWORD now = timeGetTime();
		if (now - lastLog > 500)
		{
			NET_LOG_F("[CharacterBase::Input] ID=%u 左クリック検出！ KeyFlag=0x%02X", m_clientId, m_keyFlag);
			lastLog = now;
		}
	}
	else
	{
		m_keyFlag &= ~ATTACK_KEY;
	}

	if (pEngine->GetKeyState(DIK_UP)) {
		m_position.y += 3;
	}
	if (pEngine->GetKeyState(DIK_DOWN)) {
		m_position.y -= 3;
	}

	if (pEngine->GetKeyState(DIK_RIGHT)) {
		m_position.x += 0.3f;
	}

	if (pEngine->GetKeyState(DIK_LEFT)) {
		m_position.x -= 0.3f;
	}
}

void CharacterBase::Move(Map & map)
{
	m_direction = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	if (m_keyFlag & W_KEY) m_direction += m_depth;
	if (m_keyFlag & S_KEY) m_direction -= m_depth;
	if (m_keyFlag & D_KEY) m_direction += m_hori;
	if (m_keyFlag & A_KEY) m_direction -= m_hori;
	D3DXVec3Normalize(&m_direction, &m_direction);

	float moveLength = D3DXVec3Length(&m_direction);

	if (moveLength > 0.0f)
	{
		D3DXVECTOR3 vector = m_direction * m_speed;
		vector.y = 0;

		map.MoveCheck(m_position, vector, f_radius);

		// ★★★ 修正: ローカルプレイヤーのみ足音フラグを設定 ★★★
		if (m_bIsLocal)
		{
			m_soundEvents |= SOUND_FOOTSTEP;
		}
	}
	else
	{
		// ★★★ ローカルプレイヤーのみフラグをクリア ★★★
		if (m_bIsLocal)
		{
			m_soundEvents &= ~SOUND_FOOTSTEP;
		}
	}

	m_eyePosition = m_position + ((m_keyFlag & CROUCH_KEY) ? f_crouchEyePosition : f_standEyePosition);
}
void CharacterBase::PlayFootstepSound()
{
	// 既に足音が再生中であれば二重再生を防ぐ
	if (m_seFootId != AK_INVALID_PLAYING_ID) return;

	// フットスピードパラメータを決定
	float footspeedParam = 0.0f;

	if (m_keyFlag & DASH_KEY)
	{
		footspeedParam = 1.0f; // ダッシュ時は速く
	}
	else if (m_keyFlag & CROUCH_KEY)
	{
		footspeedParam = 0.3f; // しゃがみ時は遅く
	}
	else
	{
		footspeedParam = 0.6f; // 歩行時は中間
	}

	// RTPC で足音の速度を設定
	AK::SoundEngine::SetRTPCValue(AK::GAME_PARAMETERS::FOOTSPEED, footspeedParam, m_clientId);

	// 足音を再生
	SoundManager::SetPosition(m_position, m_depth, UP_DIRECTION, m_clientId);

	// デバッグログ（頻度を下げる）
	static std::map<uint32_t, DWORD> lastPlayLog;
	DWORD now = timeGetTime();
	if (now - lastPlayLog[m_clientId] > 2000)
	{
		NET_LOG_F("[CharacterBase::PlayFootstepSound] ID=%u で足音再生開始 (IsLocal=%s)",
			m_clientId, m_bIsLocal ? "Yes" : "No");
		lastPlayLog[m_clientId] = now;
	}

	m_seFootId = SoundManager::Play(AK::EVENTS::PLAY_SE_FOOT, m_clientId);

	// 再生に失敗したらログを出す
	if (m_seFootId == AK_INVALID_PLAYING_ID)
	{
		NET_LOG_F("[CharacterBase::PlayFootstepSound] エラー: ID=%u の足音再生に失敗！", m_clientId);
	}
}

void CharacterBase::StopFootstepSound()
{
	if (m_seFootId != AK_INVALID_PLAYING_ID)
	{
		SoundManager::StopEvent(m_seFootId);
		m_seFootId = AK_INVALID_PLAYING_ID;
	}
}

void CharacterBase::SetMouseCursor(Engine * pEngine, Camera & camera)
{
	POINT move = pEngine->GetMouseMove();

	m_hAngle += (f_baseHAngle * move.x / f_defaultSenseH);
	m_vAngle += (f_baseVAngle * move.y / f_defaultSenseV);

	if (m_vAngle < f_minAngleV) m_vAngle = f_minAngleV;
	if (m_vAngle > f_maxAngleV) m_vAngle = f_maxAngleV;

	ShowCursor(FALSE);

	SetCursorPos(GetSystemMetrics(SM_CXFULLSCREEN) / 2, GetSystemMetrics(SM_CYFULLSCREEN) / 2);
}

void CharacterBase::SetFirstPersonCamera(Engine * pEngine, Camera & camera)
{
	camera.m_vecEye = m_eyePosition;
	camera.m_vecAt = m_eyePosition + m_cameraFront;
	camera.m_vecUp = D3DXVec3Cross(&m_cameraFront, &m_hori);
	camera.SetDevice(pEngine);
	m_bFirstPerson = true;
}

void CharacterBase::SetThirdPersonFromBehind(Engine * pEngine, Camera & camera, Map & map)
{
	D3DXVECTOR3 desiredCameraPosition;
	D3DXMATRIX matRotationY;

	//プレイヤーの位置を基準とする
	D3DXVECTOR3 playerCenter = m_position + D3DXVECTOR3(0.0f, 1.5f, 0.0f);

	//カメラの向きを計算
	D3DXVECTOR3 cameraDirection;

	D3DXMATRIX matRotation;
	D3DXMatrixRotationY(&matRotation, D3DXToRadian(m_hAngle));
	cameraDirection = D3DXVec3TransformCoord(&DEPTH_DIRECTION, &matRotation);

	//上下方向の回転を加える
	D3DXVECTOR3 hori;
	D3DXVec3Cross(&hori, &UP_DIRECTION, &cameraDirection);
	D3DXVec3Normalize(&hori, &hori);
	D3DXMATRIX matRotationV;
	D3DXMatrixRotationAxis(&matRotationV, &hori, D3DXToRadian(m_vAngle));
	cameraDirection = D3DXVec3TransformCoord(&cameraDirection, &matRotationV);

	//カメラの位置
	float cameraDistance = 3.0f;
	desiredCameraPosition = playerCenter - cameraDirection * cameraDistance;


	//注視点はプレイヤーの中心
	D3DXVECTOR3 targetPosition = playerCenter;

	D3DXVECTOR3 finalCameraPosition = map.AdjustCameraPosition(targetPosition, desiredCameraPosition);

	camera.m_vecEye = finalCameraPosition;
	camera.m_vecAt = targetPosition;
	camera.m_vecUp = UP_DIRECTION;
	camera.SetDevice(pEngine);
	m_bFirstPerson = false;
}

const D3DXVECTOR3& CharacterBase::GetPosition()const
{
	return m_position;
}

const D3DXVECTOR3& CharacterBase::GetDirection() const
{
	return m_direction;
}

const D3DXVECTOR3 & CharacterBase::GetDepth() const
{
	return m_depth;
}

const D3DXVECTOR2 CharacterBase::GetPosition2D() const
{
	return D3DXVECTOR2(m_position.x, m_position.z);
}

const float & CharacterBase::GetFov() const
{
	return m_fov;
}

const float & CharacterBase::GetRadius() const
{
	return f_radius;
}

const float & CharacterBase::GetArrowAngle() const
{
	// staticにしてアドレスを安全に返せるようにする
	static float rad = 0.0f;

	D3DXVECTOR2 dirXZ(m_cameraFront.x, m_cameraFront.z);
	D3DXVec2Normalize(&dirXZ, &dirXZ);

	rad = atan2f(dirXZ.x, dirXZ.y);

	//０～２πになるように調節する
	if (rad < 0) rad += D3DX_PI * 2.0f;

	return rad;
}


NetPlayerState CharacterBase::GetNetState() const
{
	NetPlayerState state;
	state.clientId = m_clientId;
	state.posX = m_position.x;
	state.posY = m_position.y;
	state.posZ = m_position.z;
	state.hAngle = m_hAngle;
	state.vAngle = m_vAngle;
	state.depthX = m_depth.x;
	state.depthY = m_depth.y;
	state.depthZ = m_depth.z;
	state.keyFlag = m_keyFlag;
	state.flags = 0;
	state.SetFirstPerson(m_bFirstPerson);

	// ★★★ 追加: 音イベントフラグを送信 ★★★
	state.soundEvents = m_soundEvents;

	// ★★★ ライト情報を初期化（派生クラスで上書き）★★★
	state.lightPosX = 0.0f;
	state.lightPosY = 0.0f;
	state.lightPosZ = 0.0f;
	state.lightDirX = 0.0f;
	state.lightDirY = 0.0f;
	state.lightDirZ = -1.0f;
	state.lightRange = 0.0f;

	// ★★★ ブレス情報を初期化 ★★★
	state.breathActive = 0;
	state.breathPosX = 0.0f;
	state.breathPosY = 0.0f;
	state.breathPosZ = 0.0f;
	state.breathDirX = 0.0f;
	state.breathDirY = 0.0f;
	state.breathDirZ = -1.0f;

	// ★★★ 氷状態を初期化（派生クラスで上書き）★★★
	state.frozen = 0;
	state.frozenAmount = 0.0f;

	// ★★★ 解凍ターゲットを初期化（派生クラスで上書き）★★★
	state.meltTargetId = GetMeltTargetId();

	return state;
}

void CharacterBase::UpdateFromNetwork(const NetPlayerState& state, DirectionalLight& light, float deltaTime)
{
	DWORD now = timeGetTime();

	// タイムスタンプ管理
	if (m_lastUpdateTime != 0)
	{
		m_timeSinceLastUpdate = (now - m_lastUpdateTime) / 1000.0f;
	}
	else
	{
		m_timeSinceLastUpdate = deltaTime;
	}
	m_lastUpdateTime = now;

	// 新しいターゲット位置を設定
	D3DXVECTOR3 newTargetPos = D3DXVECTOR3(state.posX, state.posY, state.posZ);

	// 速度の計算（予測移動用）
	D3DXVECTOR3 rawVelocity = (newTargetPos - m_targetPosition) / max(0.001f, m_timeSinceLastUpdate);

	// 速度のスムージング（急激な変化を抑制）
	m_smoothedVelocity = m_smoothedVelocity * (1.0f - m_velocitySmoothingFactor) +
		rawVelocity * m_velocitySmoothingFactor;

	// 新しいターゲット位置を設定
	m_targetPosition = newTargetPos;
	m_targetHAngle = state.hAngle;
	m_targetVAngle = state.vAngle;

	// 位置履歴に追加（ジッター対策）
	AddPositionToHistory(m_targetPosition);

	// 現在位置との距離を計算
	D3DXVECTOR3 diff = m_targetPosition - m_position;
	float dist = D3DXVec3Length(&diff);

	// デバッグログ（頻度を下げる）
	static std::map<uint32_t, DWORD> lastLogPerPlayer;
	if (now - lastLogPerPlayer[state.clientId] > 2000)
	{
		NET_LOG_F("[CharacterBase] UpdateFromNetwork: ID=%u Dist=%.2f Speed=%.2f",
			m_clientId, dist, D3DXVec3Length(&m_smoothedVelocity));
		lastLogPerPlayer[state.clientId] = now;
	}

	// 適応的テレポート閾値（速度に応じて調整）
	float speedFactor = D3DXVec3Length(&m_smoothedVelocity);
	float teleportThreshold = 1.5f + speedFactor * 0.1f;
	teleportThreshold = min(teleportThreshold, 5.0f);

	if (dist > teleportThreshold)
	{
		// 距離が大きい場合は即座にテレポート
		m_position = m_targetPosition;
		m_velocity = m_smoothedVelocity;
		NET_LOG_F("[CharacterBase] テレポート: ID=%u Dist=%.2f", m_clientId, dist);
	}
	else if (dist > 0.01f)
	{
		// 適応的補間速度（距離に応じて調整）
		float distanceFactor = min(dist * 2.0f, 1.0f);
		m_adaptiveInterpolationSpeed = m_interpolationSpeed * (1.0f + distanceFactor * 2.0f);

		// 補間係数の計算
		float t = min(1.0f, m_adaptiveInterpolationSpeed * deltaTime);

		// 位置の補間
		m_position += diff * t;

		// 速度の更新
		m_velocity = m_smoothedVelocity;
	}
	else
	{
		// ほぼ到達している場合
		m_position = m_targetPosition;
		m_velocity = D3DXVECTOR3(0, 0, 0);
	}

	// 角度の補間（改善版）
	float hDiff = m_targetHAngle - m_hAngle;
	while (hDiff > 180.0f) hDiff -= 360.0f;
	while (hDiff < -180.0f) hDiff += 360.0f;

	// 角度も適応的に補間
	float angleLerpSpeed = m_interpolationSpeed * 1.5f;
	m_hAngle += hDiff * min(1.0f, angleLerpSpeed * deltaTime);

	float vDiff = m_targetVAngle - m_vAngle;
	m_vAngle += vDiff * min(1.0f, angleLerpSpeed * deltaTime);

	// 向きベクトルとその他の状態を更新
	m_depth = D3DXVECTOR3(state.depthX, state.depthY, state.depthZ);
	m_keyFlag = state.keyFlag;
	m_bFirstPerson = state.IsFirstPerson();

	// ★★★ 追加: 音イベントフラグを受信して設定 ★★★
	m_soundEvents = state.soundEvents;

	// 目の位置を更新
	m_eyePosition = m_position + ((m_keyFlag & CROUCH_KEY) ? f_crouchEyePosition : f_standEyePosition);

	// 行列を更新
	UpdateMatrix(light);
}

// ★★★ 予測移動（フレーム間の補間）★★★
void CharacterBase::PredictMovement(float deltaTime)
{
	if (m_bIsLocal) return;  // ローカルキャラクターは予測不要

							 // 速度ベースの予測
	if (D3DXVec3Length(&m_velocity) > 0.01f)
	{
		D3DXVECTOR3 prediction = m_velocity * deltaTime;
		m_predictedPosition = m_position + prediction;

		// 予測位置とターゲット位置の間で補間
		float blend = 0.3f;  // 30%予測、70%現在位置
		m_position = m_position * (1.0f - blend) + m_predictedPosition * blend;
	}
}

// ★★★ 位置履歴の追加 ★★★
void CharacterBase::AddPositionToHistory(const D3DXVECTOR3& pos)
{
	m_positionHistory[m_positionHistoryIndex] = pos;
	m_positionHistoryIndex = (m_positionHistoryIndex + 1) % MAX_POSITION_HISTORY;

	if (m_positionHistoryCount < MAX_POSITION_HISTORY)
	{
		m_positionHistoryCount++;
	}
}

// ★★★ 平均位置の取得（ジッター対策）★★★
D3DXVECTOR3 CharacterBase::GetAveragedPosition() const
{
	if (m_positionHistoryCount == 0)
	{
		return m_position;
	}

	D3DXVECTOR3 sum(0, 0, 0);
	for (int i = 0; i < m_positionHistoryCount; i++)
	{
		sum += m_positionHistory[i];
	}

	return sum / (float)m_positionHistoryCount;
}
