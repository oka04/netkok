//*****************************************************************************
//
// ２Ｄベクトルサポートクラス
//
// Vector2D.cpp
//
// K_Yamaoka
//
// 2013/05/28
//
// 2013/11/13　D3DXVec2Rotate関数を追加
//
// 2014/10/24　関数説明の追加
//
// 2017/12/14　ベクトルの長さを求める関数を追加
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "Vector2D.h"
#include <cmath>

//=============================================================================
// ２Ｄベクトルの正規化
// 戻り値：正規化された２Ｄベクトル
// 引　数：const D3DXVECTOR2*　正規化したい２Ｄベクトルのアドレス
//=============================================================================
D3DXVECTOR2 D3DXVec2Normalize(const D3DXVECTOR2* pV)
{
	D3DXVECTOR2 work;

	D3DXVec2Normalize(&work, pV);

	return work;
}

//=============================================================================
// ２Ｄベクトルの外積計算
// 戻り値：外積計算結果
// 引　数：const D3DXVECTOR2*　左辺値としての２Ｄベクトルのアドレス
// 　　　　const D3DXVECTOR2*　右辺値としての２Ｄベクトルのアドレス
//=============================================================================
FLOAT D3DXVec2Cross(const D3DXVECTOR2* pV1, const D3DXVECTOR2* pV2)
{
	return pV1->x * pV2->y - pV1->y * pV2->x;
}

//=============================================================================
// ２Ｄベクトルの回転
// 戻り値：回転後の２Ｄベクトル
// 引　数：const D3DXVECTOR2*　回転前２Ｄベクトルのアドレス
// 　　　　const double        回転角度
//=============================================================================
D3DXVECTOR2 D3DXVec2Rotate(const D3DXVECTOR2* pV, const double angle)
{
	return D3DXVECTOR2(
		static_cast<FLOAT>(pV->x * cos(angle) + pV->y * -sin(angle)),
		static_cast<FLOAT>(pV->x * sin(angle) + pV->y * cos(angle)));
}

//=============================================================================
// ２Ｄベクトルの回転
// 戻り値：回転後の２Ｄベクトル
// 引　数：const D3DXVECTOR2*　半径を表す２Ｄベクトルのアドレス
// 　　　　const D3DXVECTOR2*　中心座標を表す２Ｄベクトルのアドレス
// 　　　　const double        回転角度
//=============================================================================
D3DXVECTOR2 D3DXVec2Rotate(const D3DXVECTOR2* pRadius, const D3DXVECTOR2* pCenter, const float angle)
{
	return D3DXVECTOR2(
		cos(angle) * pRadius->x + pCenter->x,
		sin(angle) * pRadius->y + pCenter->y);
}

//=============================================================================
// ２Ｄベクトルの長さ
// 戻り値：長さ
// 引　数：const D3DXVECTOR2*　２Ｄベクトルのアドレス
// 　　　　const D3DXVECTOR2*　２Ｄベクトルのアドレス
//=============================================================================
FLOAT D3DXVec2Length(const D3DXVECTOR2* pV1, const D3DXVECTOR2* pV2)
{
	D3DXVECTOR2 work = *pV1 - *pV2;

	return D3DXVec2Length(&work);
}
