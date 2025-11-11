//*****************************************************************************
//
// プロジェクションクラス
//
// Projection.cpp
//
// K_Yamaoka
//
// 2013/03/04
//
// 2014/11/28 エラーメッセージを追加
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "Projection.h"

//=============================================================================
// コンストラクタ
//=============================================================================
Projection::Projection()
	: m_fov(0)
	, m_aspect(0)
	, m_nearClip(0)
	, m_farClip(0)
{

}

//=============================================================================
// コンストラクタ
// 引　数：const float 視野角（ラジアン角）
// 　　　　const float アスペクト比（４：３なら 4.0 / 3.0 ）
// 　　　　const float 前方クリップ面
// 　　　　const float 後方クリップ面
//=============================================================================
Projection::Projection(const float fov, const float aspect, const float nearClip, const float farClip)
	: m_fov(fov)
	, m_aspect(aspect)
	, m_nearClip(nearClip)
	, m_farClip(farClip)
{

}

//=============================================================================
// データの設定
// 引　数：const float 視野角（ラジアン角）
// 　　　　const float アスペクト比（４：３なら 4.0 / 3.0 ）
// 　　　　const float 前方クリップ面
// 　　　　const float 後方クリップ面
//=============================================================================
void Projection::SetData(const float fov, const float aspect, const float nearClip, const float farClip)
{
	m_fov = fov;
	m_aspect = aspect;
	m_nearClip = nearClip;
	m_farClip = farClip;
}

//=============================================================================
// プロジェクション変換行列の取得
// 戻り値：D3DXMATRIX プロジェクション変換行列
//=============================================================================
D3DXMATRIX Projection::GetProjectionMatrix() const
{
	return m_matProjection;
}

//=============================================================================
// デバイスへセット
// 引　数：Engine* ３Ｄエンジンクラスへのポインタ
//=============================================================================
void Projection::SetDevice(Engine* pEngine)
{
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	//プロジェクション変換行列の作成
	D3DXMatrixPerspectiveFovLH(&m_matProjection, m_fov, m_aspect, m_nearClip, m_farClip);

	//プロジェクション変換行列をデバイスにセット
	HRESULT hResult = pDevice->SetTransform(D3DTS_PROJECTION, &m_matProjection);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PROJECTION_SETDEVICE_ERROR);
	}
}

void Projection::SetOrthoDevice(Engine* pEngine, const float width, const float height)
{
	// 正射影行列を作成
	D3DXMatrixOrthoLH(&m_matProjection, width, height, m_nearClip, m_farClip);

	// プロジェクション変換行列をデバイスにセット
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();
	HRESULT hResult = pDevice->SetTransform(D3DTS_PROJECTION, &m_matProjection);
	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PROJECTION_SETDEVICE_ERROR);
	}
}
//=============================================================================
// 変換行列の作成
// 備　考：シャドウマップ用
//=============================================================================
void Projection::CreateMatrix()
{
	//プロジェクション変換行列の作成
	D3DXMatrixPerspectiveFovLH(&m_matProjection, m_fov, m_aspect, m_nearClip, m_farClip);
}

void Projection::SetFov(const float fov)
{
	m_fov = fov;
}

float Projection::GetFov() const
{
	return m_fov;
}

void Projection::SetOrtho(const float width, const float height, const float nearClip, const float farClip)
{
	// LH = Left-Handed 座標系の正射影行列を作る
	D3DXMatrixOrthoLH(&m_matProjection, width, height, nearClip, farClip);

	m_fov = 0.0f;           
	m_aspect = width / height;  // 一応比率だけ保存
	m_nearClip = nearClip;
	m_farClip = farClip;
}






