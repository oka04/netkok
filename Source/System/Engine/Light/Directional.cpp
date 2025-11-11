//*****************************************************************************
//
// ディレクショナルライトクラス
//
// Directional.cpp
//
// K_Yamaoka
//
// 2013/03/04
//
// 2013/06/18 メンバ変数にm_indexを追加
//            SetDevice関数の引数にindexを追加
//            On,Off用関数を追加
//
// 2014/11/28 エラーメッセージを追加
//
// 2020/08/27 色の設定にアルファを加えた
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "Directional.h"

#include <Windows.h>
#include <cassert>

//=============================================================================
// コンストラクタ
//=============================================================================
DirectionalLight::DirectionalLight()
	: NOTSETDEVICE(-1)
	, m_index(NOTSETDEVICE)
{
	ZeroMemory(&m_light, sizeof(D3DLIGHT9));

	m_light.Type = D3DLIGHT_DIRECTIONAL;
}

//=============================================================================
// コンストラクタ
// 引　数：const D3DLIGHT9* ライト構造体へのポインタ
//=============================================================================
DirectionalLight::DirectionalLight(const D3DLIGHT9* pLight)
	: m_light(*pLight)
	, NOTSETDEVICE(-1)
	, m_index(NOTSETDEVICE)
{

}

//=============================================================================
// ライトの設定
// 引　数：const D3DLIGHT9* ライト構造体へのポインタ
//=============================================================================
void DirectionalLight::SetLight(const D3DLIGHT9* pLight)
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
void DirectionalLight::SetDiffuse(const float a, const float r, const float g, const float b)
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
void DirectionalLight::SetAmbient(const float a, const float r, const float g, const float b)
{
	m_light.Ambient.a = a;
	m_light.Ambient.r = r;
	m_light.Ambient.g = g;
	m_light.Ambient.b = b;
}

//=============================================================================
// 方向の設定
// 引　数：const D3DXVECTOR3* 方向ベクトルへのポインタ（単位ベクトル）
//=============================================================================
void DirectionalLight::SetDirection(const D3DXVECTOR3* pVecDirection)
{
	m_light.Direction = *(D3DVECTOR*)pVecDirection;
}

//=============================================================================
// 方向の設定
// 引　数：const D3DXVECTOR3 方向ベクトル（単位ベクトル）
//=============================================================================
void DirectionalLight::SetDirection(const D3DXVECTOR3& vecDirection)
{
	m_light.Direction = (D3DVECTOR)vecDirection;
}

//=============================================================================
// ライト構造体の取得
// 戻り値：D3DLIGHT9 ライト構造体
//=============================================================================
D3DLIGHT9 DirectionalLight::GetLight() const
{
	return m_light;
}

//=============================================================================
// デバイスへセット
// 引　数：Engine*   ３Ｄエンジンクラスへのポインタ
// 　　　　const int ライトインデックス
//=============================================================================
void DirectionalLight::SetDevice(Engine* pEngine, const int index)
{
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	m_index = index;

	//ディレクショナルライトをデバイスにセット
	//第１引数はライトのインデックス
	//デバイスには複数のライトがセットできるため、インデックスで管理する。
	HRESULT hResult = pDevice->SetLight(m_index, &m_light);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_D3D_CREATE_ERROR);
	}

	//ライトを点ける
	On(pEngine);
}

//=============================================================================
// ライトを消す
// 引　数：Engine*   ３Ｄエンジンクラスへのポインタ
//=============================================================================
void DirectionalLight::Off(Engine* pEngine)
{
	assert(m_index != NOTSETDEVICE);

	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	HRESULT hResult = pDevice->LightEnable(m_index, false);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_LIGHT_DIRECTIONAL_DISABLE_ERROR);
	}
}

//=============================================================================
// ライトを再点灯（Off後に使用）
// 引　数：Engine*   ３Ｄエンジンクラスへのポインタ
//=============================================================================
void DirectionalLight::On(Engine* pEngine)
{
	assert(m_index != NOTSETDEVICE);

	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	HRESULT hResult = pDevice->LightEnable(m_index, true);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_LIGHT_DIRECTIONAL_ENABLE_ERROR);
	}
}







































