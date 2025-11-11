//*****************************************************************************
//
// カメラクラス
//
// Camera.cpp
//
// K_Yamaoka
//
// 2013/03/04
//
// 2014/11/28 エラーメッセージを追加
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "Camera.h"

//=============================================================================
// コンストラクタ
//=============================================================================
Camera::Camera()
{

}

//=============================================================================
// コンストラクタ
// 引　数：const D3DXVECTOR3* 視点へのポインタ
// 　　　　const D3DXVECTOR3* 注目点へのポインタ
// 　　　　const D3DXVECTOR3* 上方向へのポインタ（おおよその方向を示していれば再計算される）
//=============================================================================
Camera::Camera(const D3DXVECTOR3* pVecEye, const D3DXVECTOR3* pVecAt, const D3DXVECTOR3* pVecUp)
	: m_vecEye(*pVecEye)
	, m_vecAt(*pVecAt)
	, m_vecUp(*pVecUp)
{
	RecalculateUpDirection();
}

//=============================================================================
// 上方向の再計算
//=============================================================================
void Camera::RecalculateUpDirection()
{
	//前方向ベクトル計算
	D3DXVECTOR3 vecFront = m_vecAt - m_vecEye;
	D3DXVec3Normalize(&vecFront, &vecFront);

	//右方向ベクトル計算
	D3DXVECTOR3 vecRight;
	D3DXVec3Cross(&vecRight, &vecFront, &m_vecUp);
	D3DXVec3Normalize(&vecRight, &vecRight);

	//上方向ベクトル計算
	D3DXVec3Cross(&m_vecUp, &vecRight, &vecFront);
	D3DXVec3Normalize(&m_vecUp, &m_vecUp);
}

//=============================================================================
// ビュー変換行列の取得
// 戻り値：D3DXMATRIX ビュー変換行列
//=============================================================================
D3DXMATRIX Camera::GetViewMatrix() const
{
	return m_matView;
}

//=============================================================================
// ビルボード用行列の取得
// 戻り値：D3DXMATRIX ビュー変換行列から平行移動成分を抜いた逆行列
//=============================================================================
D3DXMATRIX Camera::GetBillboardMatrix() const
{
	D3DXMATRIX matWork = m_matView;

	matWork.m[3][0] = 0;
	matWork.m[3][1] = 0;
	matWork.m[3][2] = 0;

	D3DXMatrixInverse(&matWork, nullptr, &matWork);

	return matWork;
}

//=============================================================================
// ビルボード用行列の取得
// 戻り値：D3DXMATRIX ビュー変換行列から平行移動、ＸＺ回転成分を抜いた逆行列
//=============================================================================
D3DXMATRIX Camera::GetYBillboardMatrix() const
{
	D3DXMATRIX matWork = m_matView;

	matWork.m[0][1] = 0;
	matWork.m[1][0] = 0;
	matWork.m[1][1] = 1;
	matWork.m[1][2] = 0;
	matWork.m[2][1] = 0;

	matWork.m[3][0] = 0;
	matWork.m[3][1] = 0;
	matWork.m[3][2] = 0;

	D3DXMatrixInverse(&matWork, nullptr, &matWork);

	return matWork;
}

//=============================================================================
// カメラ回転：注目点中心にＸ軸回転
// 引　数：const float 回転角度（ラジアン角）
//=============================================================================
void Camera::RotateXAroundAt(const float angle)
{
	D3DXMATRIX matRotate;

	D3DXMatrixRotationX(&matRotate, angle);

	m_vecEye = m_vecEye - m_vecAt;

	D3DXVec3TransformCoord(&m_vecEye, &m_vecEye, &matRotate);

	m_vecEye = m_vecEye + m_vecAt;

	D3DXVec3TransformCoord(&m_vecUp, &m_vecUp, &matRotate);
}

//=============================================================================
// カメラ回転：注目点中心にＹ軸回転
// 引　数：const float 回転角度（ラジアン角）
//=============================================================================
void Camera::RotateYAroundAt(const float angle)
{
	D3DXMATRIX matRotate;

	D3DXMatrixRotationY(&matRotate, angle);

	m_vecEye = m_vecEye - m_vecAt;

	D3DXVec3TransformCoord(&m_vecEye, &m_vecEye, &matRotate);

	m_vecEye = m_vecEye + m_vecAt;

	D3DXVec3TransformCoord(&m_vecUp, &m_vecUp, &matRotate);
}

//=============================================================================
// カメラ回転：注目点中心にＺ軸回転
// 引　数：const float 回転角度（ラジアン角）
//=============================================================================
void Camera::RotateZAroundAt(const float angle)
{
	D3DXMATRIX matRotate;

	D3DXMatrixRotationZ(&matRotate, angle);

	m_vecEye = m_vecEye - m_vecAt;

	D3DXVec3TransformCoord(&m_vecEye, &m_vecEye, &matRotate);

	m_vecEye = m_vecEye + m_vecAt;

	D3DXVec3TransformCoord(&m_vecUp, &m_vecUp, &matRotate);
}

//=============================================================================
// カメラ回転：視点中心にＸ軸回転
// 引　数：const float 回転角度（ラジアン角）
//=============================================================================
void Camera::RotateXAroundEye(const float angle)
{
	D3DXMATRIX matRotate;

	D3DXMatrixRotationX(&matRotate, angle);

	m_vecAt = m_vecAt - m_vecEye;

	D3DXVec3TransformCoord(&m_vecAt, &m_vecAt, &matRotate);

	m_vecAt = m_vecAt + m_vecEye;

	D3DXVec3TransformCoord(&m_vecUp, &m_vecUp, &matRotate);
}

//=============================================================================
// カメラ回転：視点中心にＹ軸回転
// 引　数：const float 回転角度（ラジアン角）
//=============================================================================
void Camera::RotateYAroundEye(const float angle)
{
	D3DXMATRIX matRotate;

	D3DXMatrixRotationY(&matRotate, angle);

	m_vecAt = m_vecAt - m_vecEye;

	D3DXVec3TransformCoord(&m_vecAt, &m_vecAt, &matRotate);

	m_vecAt = m_vecAt + m_vecEye;

	D3DXVec3TransformCoord(&m_vecUp, &m_vecUp, &matRotate);
}

//=============================================================================
// カメラ回転：視点中心にＺ軸回転
// 引　数：const float 回転角度（ラジアン角）
//=============================================================================
void Camera::RotateZAroundEye(const float angle)
{
	D3DXMATRIX matRotate;

	D3DXMatrixRotationZ(&matRotate, angle);

	m_vecAt = m_vecAt - m_vecEye;

	D3DXVec3TransformCoord(&m_vecAt, &m_vecAt, &matRotate);

	m_vecAt = m_vecAt + m_vecEye;

	D3DXVec3TransformCoord(&m_vecUp, &m_vecUp, &matRotate);
}

//=============================================================================
// 一人称視点
// 引　数：const D3DXVECTOR3* 視点へのポインタ
// 　　　　const D3DXVECTOR3* 方向ベクトルへのポインタ（単位ベクトル）
//=============================================================================
void Camera::SetFirstPerson(const D3DXVECTOR3* pVecEye, const D3DXVECTOR3* pVecDirection)
{
	m_vecEye = *pVecEye;
	m_vecAt = *pVecEye + *pVecDirection;

	RecalculateUpDirection();
}

//=============================================================================
// 三人称視点（カメラは固定）
// 引　数：const D3DXVECTOR3* 注目点へのポインタ
// 　　　　const D3DXVECTOR3* 注目点からの相対位置へのポインタ
//=============================================================================
void Camera::SetThirdPerson(const D3DXVECTOR3* pVecAt, const D3DXVECTOR3* pVecRelative)
{
	m_vecEye = *pVecAt + *pVecRelative;
	m_vecAt = *pVecAt;

	RecalculateUpDirection();
}

//=============================================================================
// 三人称視点（カメラは対象の回転に合わせて回転）
// 引　数：const D3DXVECTOR3* 注目点へのポインタ
// 　　　　const D3DXVECTOR3* 注目点からの相対位置へのポインタ
// 　　　　const float        （対象物の）回転角度
//=============================================================================
void Camera::SetThirdPersonFromBehind(const D3DXVECTOR3* pVecAt, const D3DXVECTOR3* pVecRelative, const float angleY)
{
	//回転後の座標を求める
	D3DXVECTOR3 vecRelativeBehind;

	D3DXMATRIX matRotate;
	D3DXMatrixRotationY(&matRotate, angleY);

	D3DXVec3TransformCoord(&vecRelativeBehind, pVecRelative, &matRotate);

	m_vecEye = *pVecAt + vecRelativeBehind;
	m_vecAt = *pVecAt;
	m_vecUp = D3DXVECTOR3(0, 1, 0);

	RecalculateUpDirection();
}

//=============================================================================
// デバイスへセット
// 引　数：Engine* ３Ｄエンジンクラスへのポインタ
//=============================================================================
void Camera::SetDevice(Engine* pEngine)
{
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	//ビュー変換行列の作成
	D3DXMatrixLookAtLH(&m_matView, &m_vecEye, &m_vecAt, &m_vecUp);

	//ビュー変換行列をデバイスにセット
	HRESULT hResult = pDevice->SetTransform(D3DTS_VIEW, &m_matView);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_CAMERA_SETDEVICE_ERROR);
	}
}


//=============================================================================
// 変換行列の作成
// 備　考：シャドウマップ用
//=============================================================================
void Camera::CreateMatrix()
{
	//ビュー変換行列の作成
	D3DXMatrixLookAtLH(&m_matView, &m_vecEye, &m_vecAt, &m_vecUp);
}














