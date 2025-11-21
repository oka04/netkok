#define _USING_V110_SDK71_ 1

#include "CharacterBase.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;

const D3DXVECTOR3 CharacterBase::UP_DIRECTION = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
const D3DXVECTOR3 CharacterBase::DEPTH_DIRECTION = D3DXVECTOR3(0.0f, 0.0f, -1.0f);
//初期化
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

	SetMouseCursor(pEngine, camera);
	UpdateMatrix(light);
}

//更新
void CharacterBase::UpdateMatrix(DirectionalLight &light)
{
	float currentAngle = D3DXToRadian(m_hAngle);

	D3DXVECTOR3 baseDirection;
	
	baseDirection = DEPTH_DIRECTION;
	m_angle = currentAngle;

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

	SoundManager::SetPosition(m_position, m_depth, UP_DIRECTION, SoundManager::ID_LISTENER);
	D3DXMatrixRotationY(&m_matRotate, m_angle);

	D3DXMatrixTranslation(&m_matTrans, &m_position);

	m_matWorld = m_matRotate * m_matTrans;
	
	m_model.SetWorldTransform(&m_matWorld);
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
}

//m_speedを変えてから呼び出す
void CharacterBase::Move(Map & map)
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
		//m_speedだけ呼び出す先のクラスで決める
		
		D3DXVECTOR3 vector = m_direction * m_speed;
		vector.y = 0;

		map.MoveCheck(m_position, vector, f_radius);
	}

	//目の位置の調整
	m_eyePosition = m_position + ((m_keyFlag & CROUCH_KEY) ? f_crouchEyePosition : f_standEyePosition);
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

void CharacterBase::UpdateStamina()
{
}

void CharacterBase::DrawStaminaGauge()
{
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

const D3DXVECTOR2 & CharacterBase::GetPosition2D() const
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
	D3DXVECTOR2 dirXZ(m_cameraFront.x, m_cameraFront.z);
	D3DXVec2Normalize(&dirXZ, &dirXZ);

	float rad = atan2f(dirXZ.x, dirXZ.y);

	//０～２πになるように調節する
	if (rad < 0) rad += D3DX_PI * 2.0f;

	return rad;
}
