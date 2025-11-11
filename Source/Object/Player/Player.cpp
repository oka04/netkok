#define _USING_V110_SDK71_ 1

#include "Player.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;

const D3DXVECTOR3 Player::UP_DIRECTION = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
const D3DXVECTOR3 Player::DEPTH_DIRECTION = D3DXVECTOR3(0.0f, 0.0f, -1.0f);

void Player::Initialize(Engine* pEngine, Map& map, Projection* projection, Camera& camera, DirectionalLight &light)
{
	SoundManager::RegisterGameObject(ID_PALYER, "PLAYER");

	CharacterBase::Initialize(pEngine, MODEL_CHARACTER);
	LoadParameter();

	pEngine->AddTexture(TEXTURE_STAMINA_GAUGE);
	m_angle = D3DXToRadian(90.0f);
	m_direction = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	m_position = map.GetPlayerStartPosition();
	m_depth = DEPTH_DIRECTION;
	m_cameraFront = m_depth;

	m_speed = 0;

	m_hAngle = 0.0f;
	m_vAngle = 0.0f;
	m_fov = projection->GetFov();
	m_bFirstPerson = true;

	m_stamina = f_maxStamina;
	m_staminaRecoveryTimer = 0.0f;
	m_bFatigued = false;

	m_keyFlag = 0x00;

	SetMouseCursor(pEngine, camera);
	UpdateMatrix(light);
}

void Player::Release(Engine * pEngine)
{
	pEngine->ReleaseTexture(TEXTURE_STAMINA_GAUGE);
	SoundManager::UnregisterGameObject(ID_PALYER);
}

void Player::Update(Engine* pEngine, Map& map, Camera& camera, DirectionalLight &light, float deltaTime)
{
	SetMouseCursor(pEngine, camera);
	Input(pEngine, map);

	UpdateStamina(deltaTime);

	if (m_keyFlag & STICK_KEY)
	{
		SetThirdPersonFromBehind(pEngine, camera, map);
		StickWallMove(map, deltaTime);
	}
	else
	{
		SetFirstPersonCamera(pEngine, camera);
		NormalMove(map, deltaTime);
	}

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
	pEngine->DrawPrintf(0, 50, FONT_GOTHIC40, Color::BLUE, "Position：%f,%f,%f", m_eyePosition.x, m_eyePosition.y, m_eyePosition.z);
	pEngine->DrawPrintf(0, 100, FONT_GOTHIC40, Color::BLUE, "depth：%f,%f,%f", m_depth.x, m_depth.y, m_depth.z);
	pEngine->DrawPrintf(0, 150, FONT_GOTHIC40, Color::BLUE, "vAngle：%f", m_vAngle);
	pEngine->DrawPrintf(0, 250, FONT_GOTHIC40, Color::BLUE, "hAngle：%f", m_hAngle);
	pEngine->DrawPrintf(0, 300, FONT_GOTHIC40, Color::BLUE, "speed：%f", m_speed);
	pEngine->DrawPrintf(0, 350, FONT_GOTHIC40, Color::BLUE, "m_stickNormal：%f,%f,%f", m_stickNormal.x, m_stickNormal.y, m_stickNormal.z);
}

//一人称視点の更新
void Player::SetFirstPersonCamera(Engine* pEngine, Camera& camera)
{
	camera.m_vecEye = m_eyePosition;
	camera.m_vecAt = m_eyePosition + m_cameraFront;
	camera.m_vecUp = D3DXVec3Cross(&m_cameraFront, &m_hori);
	camera.SetDevice(pEngine);
	m_bFirstPerson = true;
}

void Player::SetThirdPersonFromBehind(Engine* pEngine, Camera& camera, Map& map)
{
	D3DXVECTOR3 desiredCameraPosition;
	D3DXMATRIX matRotationY;

	//プレイヤーの位置を基準とする
	D3DXVECTOR3 playerCenter = m_position + D3DXVECTOR3(0.0f, 1.0f, 0.0f);

	//カメラの向きを計算
	D3DXVECTOR3 cameraDirection;

	
	if (m_keyFlag & STICK_KEY)
	{
		D3DXVECTOR3 wallFront = -m_stickNormal;
		wallFront.y = 0.0f;
		D3DXVec3Normalize(&wallFront, &wallFront);

		D3DXMATRIX matRotation;
		D3DXMatrixRotationY(&matRotation, D3DXToRadian(m_hAngle) + atan2f(wallFront.x, wallFront.z));
		D3DXVECTOR3 tempDirection = -DEPTH_DIRECTION;
		cameraDirection = D3DXVec3TransformCoord(&tempDirection, &matRotation);
	}
	else
	{
		D3DXVECTOR3 playerFront = DEPTH_DIRECTION;
		playerFront.y = 0.0f;
		D3DXVec3Normalize(&playerFront, &playerFront);

		D3DXMATRIX matRotation;
		D3DXMatrixRotationY(&matRotation, D3DXToRadian(m_hAngle));
		cameraDirection = D3DXVec3TransformCoord(&playerFront, &matRotation);
	}

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

bool Player::RotateToTarget(DirectionalLight& light, const D3DXVECTOR3& targetPos, float deltaTime)
{
	if (m_keyFlag & STICK_KEY) {
		float wallAbsAngle = atan2f(m_stickNormal.x, m_stickNormal.z);
		float finalAngleRad = wallAbsAngle + D3DXToRadian(m_hAngle);
		m_hAngle = D3DXToDegree(finalAngleRad);

		m_keyFlag &= ~STICK_KEY;
	}

	D3DXVECTOR3 toTarget = m_position - targetPos;
	D3DXVECTOR2 toTarget2D(toTarget.x, toTarget.z);
	D3DXVec2Normalize(&toTarget2D, &toTarget2D);

	float targetAngle = atan2f(toTarget2D.x, toTarget2D.y);
	float diffAngle = targetAngle - m_angle;
	while (diffAngle > D3DX_PI) diffAngle -= 2.0f * D3DX_PI;
	while (diffAngle < -D3DX_PI) diffAngle += 2.0f * D3DX_PI;

	if (fabs(diffAngle) < D3DXToRadian(f_rotateFinishAngle)) {
		m_angle = targetAngle;
		m_hAngle = D3DXToDegree(m_angle);
		UpdateMatrix(light);
		return true;
	}
	else {
		float rotateAmount = diffAngle * f_rotateSpeed * deltaTime;
		float minRotateAmount = f_minRotateSpeed * deltaTime;
		if (fabs(rotateAmount) < minRotateAmount)
		{
			rotateAmount = minRotateAmount * (rotateAmount > 0 ? 1.0f : -1.0f);
		}

		m_angle += rotateAmount;
		m_hAngle = D3DXToDegree(m_angle);
	}

	UpdateMatrix(light);
	return false;
}

const float Player::GetFov()const
{
	return m_fov;
}

D3DXVECTOR3 Player::GetDepth()const
{
	return m_depth;
}

const float Player::GetRadius() const
{
	return f_radius;
}

const float Player::GetArrowAngle()const
{
	D3DXVECTOR2 dirXZ(m_cameraFront.x, m_cameraFront.z);
	D3DXVec2Normalize(&dirXZ, &dirXZ);

	float rad = atan2f(dirXZ.x, dirXZ.y);

	//０～２πになるように調節する
	if (rad < 0) rad += D3DX_PI * 2.0f;

	return rad;
}

D3DXVECTOR2 Player::GetPosition2D() const
{
	return D3DXVECTOR2(m_position.x, m_position.z);
}

void Player::UpdateMatrix(DirectionalLight &light)
{
	float currentAngle = D3DXToRadian(m_hAngle);

	D3DXVECTOR3 baseDirection;
	if (m_keyFlag & STICK_KEY)
	{
		baseDirection = -m_stickNormal;
		baseDirection.y = 0.0f;
		D3DXVec3Normalize(&baseDirection, &baseDirection);

		//キャラクターの描画回転角m_angleを壁の法線に固定する
		//m_stickNormalが壁の外を向いている場合、プレイヤーは壁を向くためD3DX_PIを加える
		m_angle = atan2f(m_stickNormal.x, m_stickNormal.z) + D3DX_PI;
	}
	else
	{
		baseDirection = DEPTH_DIRECTION;
		m_angle = currentAngle;
	}

	//m_hAngleによる回転行列を計算
	D3DXMATRIX matRotationY;
	D3DXMatrixRotationY(&matRotationY, currentAngle);

	//基準方向を回転させてm_depth（カメラの奥行き、ライトの向き）を決める
	m_depth = D3DXVec3TransformCoord(&baseDirection, &matRotationY);

	m_hori = D3DXVec3Cross(&UP_DIRECTION, &m_depth);
	D3DXVec3Normalize(&m_hori, &m_hori);

	//カメラの上下の回転は常にマウス入力から計算
	D3DXMATRIX matRotationV;
	D3DXMatrixRotationAxis(&matRotationV, &m_hori, D3DXToRadian(m_vAngle));
	m_cameraFront = D3DXVec3TransformCoord(&m_depth, &matRotationV);

	//ライトをカメラの視線方向（m_depth）に設定する
	light.SetDirection(m_depth);

	CharacterBase::UpdateMatrix();
	SoundManager::SetPosition(m_position, m_depth, UP_DIRECTION, SoundManager::ID_LISTENER);
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
	f_rotateSpeed = config["rotateSpeed"];
	f_minRotateSpeed = config["minRotateSpeed"];
	f_rotateFinishAngle = config["rotateFinishAngle"];

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

void Player::StickWallMove(Map & map, float deltaTime)
{
	m_direction = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

	D3DXVECTOR2 normal2D(m_stickNormal.x, m_stickNormal.z);
	D3DXVec2Normalize(&normal2D, &normal2D);
	D3DXVECTOR3 tangent(-normal2D.y, 0.0f, normal2D.x);


	if (m_keyFlag & D_KEY) m_direction += tangent;
	if (m_keyFlag & A_KEY) m_direction -= tangent;

	if (D3DXVec3LengthSq(&m_direction) > 0.0f)
	{
		m_speed = f_crouchSpeed * deltaTime;
		D3DXVec3Normalize(&m_direction, &m_direction);
		m_position += m_direction * m_speed;
		map.StickWallCheck(m_position, f_radius, f_radius + f_radius, nullptr);
	}

	m_eyePosition = m_position + f_standEyePosition;
}

void Player::NormalMove(Map & map, float deltaTime)
{
	m_direction = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	if (m_keyFlag & W_KEY) m_direction += m_depth;
	if (m_keyFlag & S_KEY) m_direction -= m_depth;
	if (m_keyFlag & D_KEY) m_direction += m_hori;
	if (m_keyFlag & A_KEY) m_direction -= m_hori;
	D3DXVec3Normalize(&m_direction, &m_direction);

	//移動キーを入力しているか
	if (D3DXVec3Length(&m_direction) > 0.0f)
	{
		if(m_bFatigued)
		{
			m_speed = f_fatigueSpeed * deltaTime;
		}
		else if (m_keyFlag & CROUCH_KEY)
		{
			m_speed = f_crouchSpeed * deltaTime;
		}
		else if (m_keyFlag & DASH_KEY)
		{
			m_speed = f_dashSpeed * deltaTime;
		}
		else
		{
			m_speed = f_walkSpeed * deltaTime;
		}
		D3DXVECTOR3 vector = m_direction * m_speed;
		vector.y = 0;

		map.MoveCheck(m_position, vector, f_radius);
	}

	//目の位置の調整
	m_eyePosition = m_position + ((m_keyFlag & CROUCH_KEY) ? f_crouchEyePosition : f_standEyePosition);
}

void Player::SetMouseCursor(Engine* pEngine, Camera& camera)
{
	POINT move = pEngine->GetMouseMove();

	//張り付き時
	if (m_keyFlag & STICK_KEY)
	{
		float offsetHAngle = (f_baseHAngle * move.x / f_defaultSenseH);

		m_hAngle += offsetHAngle;

		if (m_hAngle < -f_stickAngleH) m_hAngle = -f_stickAngleH;
		else if (m_hAngle > f_stickAngleH) m_hAngle = f_stickAngleH;

	}
	else
	{ //通常時
		m_hAngle += (f_baseHAngle * move.x / f_defaultSenseH);
	}

	m_vAngle += (f_baseVAngle * move.y / f_defaultSenseV);

	if (m_vAngle < f_minAngleV) m_vAngle = f_minAngleV;
	if (m_vAngle > f_maxAngleV) m_vAngle = f_maxAngleV;

	ShowCursor(FALSE);

	SetCursorPos(GetSystemMetrics(SM_CXFULLSCREEN) / 2, GetSystemMetrics(SM_CYFULLSCREEN) / 2);
}

void Player::Input(Engine* pEngine, Map& map)
{

#if _DEBUG

	if (pEngine->GetKeyState(DIK_UP))
	{
		m_position.y += 0.3f;
	}
	if (pEngine->GetKeyState(DIK_DOWN))
	{
		m_position.y -= 0.3f;
	}

	if (pEngine->GetKeyState(DIK_LEFT))
	{
		m_position.x += 0.2f;
	}
	if (pEngine->GetKeyState(DIK_RIGHT))
	{
		m_position.x -= 0.2f;
	}

#endif

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

	if (pEngine->GetKeyStateSync(DIK_SPACE))
	{
		if (m_keyFlag & STICK_KEY)
		{
			float wallAbsAngle = atan2f(m_stickNormal.x, m_stickNormal.z);
			float finalAngleRad = wallAbsAngle + D3DXToRadian(m_hAngle);
			m_hAngle = D3DXToDegree(finalAngleRad);
			
			m_keyFlag &= ~STICK_KEY;
			SoundManager::Play(AK::EVENTS::PLAY_SE_STICKWALL, ID_PALYER);
		}
		else {
			m_stickNormal = D3DXVECTOR3(0, 0, 0);
			if (map.StickWallCheck(m_position, f_radius, f_radius + f_radius, &m_stickNormal))
			{
				m_keyFlag |= STICK_KEY;
				SoundManager::Play(AK::EVENTS::PLAY_SE_STICKWALL, ID_PALYER);
				m_hAngle = 0.0f;
			}
			else
			{
				m_keyFlag &= ~STICK_KEY;
			}
		}
	}
}