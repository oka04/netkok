//*****************************************************************************
//
// ３Ｄベクトルサポートクラス
//
// Vector3D.cpp
//
// K_Yamaoka
//
// 2013/05/28
//
// 2014/10/24　関数説明の追加
//
// 2017/12/14　ベクトルの長さを求める関数を追加
//
// 2020/11/12　GetMatrixFromTwoVector関数を追加
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "Vector3D.h"

//=============================================================================
// ３Ｄベクトルと行列の乗算
// 戻り値：計算後の３Ｄベクトル
// 引　数：const D3DXVECTOR3*　３Ｄベクトルのアドレス
// 　　　　const D3DXMATRIX* 　行列のアドレス
//=============================================================================
D3DXVECTOR3 D3DXVec3TransformCoord(const D3DXVECTOR3* pV, const D3DXMATRIX* pM)
{
	D3DXVECTOR3 work;

	D3DXVec3TransformCoord(&work, pV, pM);

	return work;
}

//=============================================================================
// ３Ｄベクトルの正規化
// 戻り値：正規化された３Ｄベクトル
// 引　数：const D3DXVECTOR3*　正規化したい３Ｄベクトルのアドレス
//=============================================================================
D3DXVECTOR3 D3DXVec3Normalize(const D3DXVECTOR3* pV)
{
	D3DXVECTOR3 work;

	D3DXVec3Normalize(&work, pV);

	return work;
}

//=============================================================================
// ３Ｄベクトルの外積計算
// 戻り値：外積計算結果
// 引　数：const D3DXVECTOR3*　左辺値としての３Ｄベクトルのアドレス
// 　　　　const D3DXVECTOR3*　右辺値としての３Ｄベクトルのアドレス
//=============================================================================
D3DXVECTOR3 D3DXVec3Cross(const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2)
{
	D3DXVECTOR3 work;

	D3DXVec3Cross(&work, pV1, pV2);

	return work;
}

//=============================================================================
// 法線を求める
// 戻り値：正規化された法線ベクトル
// 引　数：const D3DXVECTOR3*　１つ目の３Ｄベクトルのアドレス
// 　　　　const D3DXVECTOR3*　２つ目の３Ｄベクトルのアドレス
// 　　　　const D3DXVECTOR3*　３つ目の３Ｄベクトルのアドレス
//=============================================================================
D3DXVECTOR3 D3DXComputeNormal(const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2, const D3DXVECTOR3* pV3)
{
	D3DXVECTOR3 normal;

	D3DXVECTOR3 v1 = *pV2 - *pV1;
	D3DXVECTOR3 v2 = *pV3 - *pV1;

	normal = D3DXVec3Cross(&v1, &v2);

	normal = D3DXVec3Normalize(&normal);

	return normal;
}

//=============================================================================
// 法線を求める
// 戻り値：正規化された法線ベクトル
// 引　数：const D3DXVECTOR3 [3]　ベクトル配列の先頭番地
//=============================================================================
D3DXVECTOR3 D3DXComputeNormal(const D3DXVECTOR3 vertex[3])
{
	D3DXVECTOR3 normal;

	D3DXVECTOR3 v1 = vertex[1] - vertex[0];
	D3DXVECTOR3 v2 = vertex[2] - vertex[0];

	normal = D3DXVec3Cross(&v1, &v2);

	normal = D3DXVec3Normalize(&normal);

	return normal;
}

//=============================================================================
// ３Ｄベクトルの長さを求める
// 戻り値：長さ
// 引　数：const D3DXVECTOR3*　３Ｄベクトルのアドレス
// 　　　　const D3DXVECTOR3*　３Ｄベクトルのアドレス
//=============================================================================
FLOAT D3DXVec3Length(const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2)
{
	D3DXVECTOR3 work = *pV1 - *pV2;

	return D3DXVec3Length(&work);
}

//=============================================================================
// ベクトル１をベクトル２に向かせる回転行列を求める
// 戻り値：回転行列
// 引　数：const D3DXVECTOR3　ベクトル１
// 　　　　const D3DXVECTOR3　ベクトル２
//=============================================================================
D3DXMATRIX GetMatrixFromTwoVector(const D3DXVECTOR3 v1, const D3DXVECTOR3 v2)
{
	D3DXVECTOR3 a = D3DXVec3Normalize(&v1);
	D3DXVECTOR3 b = D3DXVec3Normalize(&v2);
	float q[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	D3DXVECTOR3 c = D3DXVec3Cross(&b, &a);
	float d = -D3DXVec3Length(&c);
	c = D3DXVec3Normalize(&c);

	float epsilon = 0.0002f;
	float ip = D3DXVec3Dot(&a, &b);

	if (-epsilon < d || 1.0f < ip) {
		if (ip < (epsilon - 1.0f)) {
			D3DXVECTOR3 a2(-a.y, a.z, a.x);
			c = D3DXVec3Cross(&a2, &a);
			c = D3DXVec3Normalize(&c);
			q[0] = 0.0f;
			q[1] = c.x;
			q[2] = c.y;
			q[3] = c.z;
		}
		else {
			q[0] = 1.0f; q[1] = 0.0f; q[2] = 0.0f; q[3] = 0.0f;
		}
	}
	else {
		D3DXVECTOR3 e = c * (float)sqrt(0.5f * (1.0f - ip));
		q[0] = (float)sqrt(0.5f * (1.0f + ip));
		q[1] = e.x;
		q[2] = e.y;
		q[3] = e.z;
	}

	D3DXMATRIX m_matWorld;

	float q2[4];
	q2[0] = q[1];
	q2[1] = q[2];
	q2[2] = q[3];
	q2[3] = q[0];

	m_matWorld.m[0][0] = q2[3] * q2[3] + q2[0] * q2[0] - q2[1] * q2[1] - q[2] * q2[2];
	m_matWorld.m[0][1] = 2.0f * q2[0] * q2[1] - 2.0f * q2[3] * q2[2];
	m_matWorld.m[0][2] = 2.0f * q2[0] * q2[2] + 2.0f * q2[3] * q2[1];
	m_matWorld.m[0][3] = 0.0f;

	m_matWorld.m[1][0] = 2.0f * q2[0] * q2[1] + 2.0f * q2[3] * q2[2];
	m_matWorld.m[1][1] = q2[3] * q2[3] - q2[0] * q2[0] + q2[1] * q2[1] - q2[2] * q2[2];
	m_matWorld.m[1][2] = 2.0f * q2[1] * q2[2] - 2.0f * q2[3] * q2[0];
	m_matWorld.m[1][3] = 0.0f;

	m_matWorld.m[2][0] = 2.0f * q2[0] * q2[2] - 2.0f * q2[3] * q2[1];
	m_matWorld.m[2][1] = 2.0f * q2[1] * q2[2] + 2.0f * q2[3] * q2[0];
	m_matWorld.m[2][2] = q2[3] * q2[3] - q2[0] * q2[0] - q2[1] * q2[1] + q2[2] * q2[2];
	m_matWorld.m[2][3] = 0.0f;

	m_matWorld.m[3][0] = 0.0f;
	m_matWorld.m[3][1] = 0.0f;
	m_matWorld.m[3][2] = 0.0f;
	m_matWorld.m[3][3] = q2[3] * q2[3] + q2[0] * q2[0] + q2[1] * q2[1] + q2[2] * q2[2];

	float k = m_matWorld.m[3][3];

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			m_matWorld.m[i][j] /= k;
		}
	}

	m_matWorld.m[3][3] = 1.0f;

	return m_matWorld;
}