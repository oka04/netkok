//*****************************************************************************
//
// アンビエントライトクラス
//
// Ambient.cpp
//
// K_Yamaoka
//
// 2013/03/04
//
// 2014/11/28 エラーメッセージを追加
//
// 2020/08/24 SetColor（引数をfloatにしたもの）、GetColorVector関数を追加
//
// 2020/08/27 色の設定にアルファを加えた、GetColorVectorをGetColorValueに変更
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "Ambient.h"

//=============================================================================
// コンストラクタ
//=============================================================================
AmbientLight::AmbientLight()
{

}

//=============================================================================
// コンストラクタ
// 引　数：const D3DCOLOR ディフューズ色
//=============================================================================
AmbientLight::AmbientLight(const D3DCOLOR diffuse)
	: m_diffuse(diffuse)
{

}

//=============================================================================
// コンストラクタ
// 引　数：const BYTE アルファ（０～２５５）
// 　　　　const BYTE 赤（０～２５５）
// 　　　　const BYTE 緑（０～２５５）
// 　　　　const BYTE 青（０～２５５）
//=============================================================================
AmbientLight::AmbientLight(const BYTE a, const BYTE r, const BYTE g, const BYTE b)
	: m_diffuse(D3DCOLOR_ARGB(a, r, g, b))
{

}

//=============================================================================
// ディフューズ色の設定
// 引　数：const D3DCOLOR ディフューズ色
//=============================================================================
void AmbientLight::SetColor(const D3DCOLOR diffuse)
{
	m_diffuse = diffuse;
}

//=============================================================================
// ディフューズ色の設定
// 引　数：const BYTE アルファ（０～２５５）
// 　　　　const BYTE 赤（０～２５５）
// 　　　　const BYTE 緑（０～２５５）
// 　　　　const BYTE 青（０～２５５）
//=============================================================================
void AmbientLight::SetColor(const BYTE a, const BYTE r, const BYTE g, const BYTE b)
{
	m_diffuse = D3DCOLOR_ARGB(a, r, g, b);
}

//=============================================================================
// ディフューズ色の設定
// 引　数：const float アルファ（０～１．０）
// 　　　　const float 赤（０～１．０）
// 　　　　const float 緑（０～１．０）
// 　　　　const float 青（０～１．０）
//=============================================================================
void AmbientLight::SetColor(const float a, const float r, const float g, const float b)
{
	BYTE ba = (BYTE)(a * 255);
	BYTE br = (BYTE)(r * 255);
	BYTE bg = (BYTE)(g * 255);
	BYTE bb = (BYTE)(b * 255);
	SetColor(ba, br, bg, bb);
}

//=============================================================================
// ディフューズ色の取得
// 戻り値：D3DCOLOR ディフューズ色
//=============================================================================
D3DCOLOR AmbientLight::GetColor() const
{
	return m_diffuse;
}

//=============================================================================
// ディフューズ色の取得
// 戻り値：D3DCOLORVALUE ディフューズ色
//=============================================================================
D3DCOLORVALUE AmbientLight::GetColorValue() const
{
	BYTE ba = (BYTE)((m_diffuse & 0xFF000000) >> 24);
	BYTE br = (BYTE)((m_diffuse & 0x00FF0000) >> 16);
	BYTE bg = (BYTE)((m_diffuse & 0x0000FF00) >> 8);
	BYTE bb = (BYTE)(m_diffuse & 0x000000FF);

	D3DCOLORVALUE color;
	color.a = ba / 255.0f;
	color.r = br / 255.0f;
	color.g = bg / 255.0f;
	color.b = bb / 255.0f;

	return color;
}

//=============================================================================
// デバイスへセット
// 引　数：Engine* ３Ｄエンジンクラスへのポインタ
//=============================================================================
void AmbientLight::SetDevice(Engine* pEngine)
{
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	//アンビエントライトをデバイスにセット
	HRESULT hResult = pDevice->SetRenderState(D3DRS_AMBIENT, m_diffuse);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_LIGHT_AMBIENT_SETDEVICE_ERROR);
	}
}






































