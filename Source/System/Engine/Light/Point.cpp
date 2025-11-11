//*****************************************************************************
//
// ポイントライトクラス
//
// Point.cpp
//
// K_Yamaoka
//
// 2013/06/18
//
// 2014/11/28 エラーメッセージを追加
//
// 2020/08/27 色の設定にアルファを加えた
//
// 2020/09/04 シェーダーに対応していないため使用不可
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "Point.h"

#include <Windows.h>
#include <cassert>

//=============================================================================
// コンストラクタ
//=============================================================================
PointLight::PointLight()
	: NOTSETDEVICE(-1)
	, m_index(NOTSETDEVICE)
{
	ZeroMemory(&m_light, sizeof(D3DLIGHT9));

	m_light.Type = D3DLIGHT_POINT;
}

//=============================================================================
// コンストラクタ
// 引　数：const D3DLIGHT9* ライト構造体へのポインタ
//=============================================================================
PointLight::PointLight(const D3DLIGHT9* pLight)
	: m_light(*pLight)
	, NOTSETDEVICE(-1)
	, m_index(NOTSETDEVICE)
{

}

//=============================================================================
// ライトの設定
// 引　数：const D3DLIGHT9* ライト構造体へのポインタ
//=============================================================================
void PointLight::SetLight(const D3DLIGHT9* pLight)
{
	m_light = *pLight;
}

//=============================================================================
// ディフューズ色の設定
// 引　数：const float アルファ（０～１．０）
// 　　　　const float 赤（０～１．０）
// 　　　　const float 緑（０～１．０）
// 　　　　const float 青（０～１．０）
//=============================================================================
void PointLight::SetDiffuse(const float a, const float r, const float g, const float b)
{
	m_light.Diffuse.a = a;
	m_light.Diffuse.r = r;
	m_light.Diffuse.g = g;
	m_light.Diffuse.b = b;
}

//=============================================================================
// アンビエント色の設定
// 引　数：const float アルファ（０～１．０）
// 　　　　const float 赤（０～１．０）
// 　　　　const float 緑（０～１．０）
// 　　　　const float 青（０～１．０）
//=============================================================================
void PointLight::SetAmbient(const float a, const float r, const float g, const float b)
{
	m_light.Ambient.a = a;
	m_light.Ambient.r = r;
	m_light.Ambient.g = g;
	m_light.Ambient.b = b;
}

//=============================================================================
// 位置の設定
// 引　数：const D3DXVECTOR3* 位置ベクトルへのポインタ
//=============================================================================
void PointLight::SetPosition(const D3DXVECTOR3* pVecPosition)
{
	m_light.Position = *(D3DVECTOR*)pVecPosition;
}

//=============================================================================
// 位置の設定
// 引　数：const D3DXVECTOR3 位置ベクトル
//=============================================================================
void PointLight::SetPosition(const D3DXVECTOR3 vecPosition)
{
	m_light.Position = (D3DVECTOR)vecPosition;
}

//=============================================================================
// 範囲の設定
// 引　数：const float 範囲
//=============================================================================
void PointLight::SetRange(const float range)
{
	m_light.Range = range;
}

//=============================================================================
// 減衰の設定
// 引　数：const float 定常減衰係数
//         const float 線形減衰係数
//         const float 平方減衰係数
//=============================================================================
void PointLight::SetAttenuation(const float attenuation0, const float attenuation1, const float attenuation2)
{
	m_light.Attenuation0 = attenuation0;
	m_light.Attenuation1 = attenuation1;
	m_light.Attenuation2 = attenuation2;
}

//=============================================================================
// ライト構造体の取得
// 戻り値：D3DLIGHT9 ライト構造体
//=============================================================================
D3DLIGHT9 PointLight::GetLight() const
{
	return m_light;
}

//=============================================================================
// デバイスへセット
// 引　数：Engine*   ３Ｄエンジンクラスへのポインタ
// 　　　　const int ライトインデックス
//=============================================================================
void PointLight::SetDevice(Engine* pEngine, const int index)
{
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	m_index = index;

	//ポイントライトをデバイスにセット
	//第１引数はライトのインデックス
	//デバイスには複数のライトがセットできるため、インデックスで管理する。
	HRESULT hResult = pDevice->SetLight(m_index, &m_light);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_LIGHT_POINT_SETDEVICE_ERROR);
	}

	//ライトを点ける
	On(pEngine);
}

//=============================================================================
// ライトを消す
// 引　数：Engine*   ３Ｄエンジンクラスへのポインタ
//=============================================================================
void PointLight::Off(Engine* pEngine)
{
	assert(m_index != NOTSETDEVICE);

	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	HRESULT hResult = pDevice->LightEnable(m_index, false);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_LIGHT_POINT_DISABLE_ERROR);
	}
}

//=============================================================================
// ライトを再点灯（Off後に使用）
// 引　数：Engine*   ３Ｄエンジンクラスへのポインタ
//=============================================================================
void PointLight::On(Engine* pEngine)
{
	assert(m_index != NOTSETDEVICE);

	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	HRESULT hResult = pDevice->LightEnable(m_index, true);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_LIGHT_POINT_ENABLE_ERROR);
	}
}







































