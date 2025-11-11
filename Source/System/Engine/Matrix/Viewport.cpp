//*****************************************************************************
//
// ビューポートクラス
//
// Viewport.cpp
//
// K_Yamaoka
//
// 2013/03/04
//
// 2014/11/28 エラーメッセージを追加
//
// 2019/11/28 GetViewportMatrix関数を追加
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "Viewport.h"

//=============================================================================
// コンストラクタ
//=============================================================================
Viewport::Viewport()
{

}

//=============================================================================
// ビューポートの追加
// 引　数：const D3DVIEWPORT9* ビューポート構造体へのポインタ
//=============================================================================
void Viewport::Add(const D3DVIEWPORT9* pViewport)
{
	//可変長配列にプッシュ
	m_container.push_back(*pViewport);
}

//=============================================================================
// ビューポートの追加
// 引　数：const DWORD 左上のＸ座標
// 　　　　const DWORD 左上のＹ座標
// 　　　　const DWORD 幅
// 　　　　const DWORD 高さ
// 　　　　const float ３Ｄ→２Ｄ変換時のＺの最小値
// 　　　　const float ３Ｄ→２Ｄ変換時のＺの最大値
//=============================================================================
void Viewport::Add(const DWORD x, const DWORD y, const DWORD width, const DWORD height, const float minZ, const float maxZ)
{
	D3DVIEWPORT9 work;

	work.X = x;
	work.Y = y;
	work.Width = width;
	work.Height = height;
	work.MinZ = minZ;
	work.MaxZ = maxZ;

	Add(&work);
}

//=============================================================================
// ビューポートの削除
// 引　数：const int インデックス
//=============================================================================
void Viewport::Delete(const int index)
{
	std::vector<D3DVIEWPORT9>::iterator it = m_container.begin();

	std::advance(it, index);

	m_container.erase(it);
}

//=============================================================================
// 現在のビューポート数の取得
// 戻り値：int ビューポートの数
//=============================================================================
int Viewport::GetViewportCount() const
{
	return m_container.size();
}

//=============================================================================
// 指定したビューポート構造体の取得
// 戻り値：D3DVIEWPORT9 ビューポート構造体
// 引　数：const int インデックス
//=============================================================================
D3DVIEWPORT9 Viewport::GetData(const int index) const
{
	return m_container[index];
}

//=============================================================================
// デバイスへセット
// 引　数：Engine* ３Ｄエンジンクラスへのポインタ
// 　　　　const int インデックス
//=============================================================================
void Viewport::SetDevice(Engine* pEngine, const int index)
{
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	//ビューポート構造体をデバイスにセット
	HRESULT hResult = pDevice->SetViewport(&m_container[index]);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_VIEWPORT_SETDEVICE_ERROR);
	}
}

//=============================================================================
// ビューポート変換行列の取得
// 戻り値：D3DXMATRIX ビューポート変換行列
//=============================================================================
D3DXMATRIX Viewport::GetViewportMatrix(const int index) const
{
	D3DXMATRIX mat;

	D3DXMatrixIdentity(&mat);

	mat.m[0][0] = m_container[index].Width / 2.0f;
	mat.m[1][1] = -(m_container[index].Height / 2.0f);
	mat.m[2][2] = m_container[index].MaxZ - m_container[index].MinZ;
	mat.m[3][0] = m_container[index].X + m_container[index].Width / 2.0f;
	mat.m[3][1] = m_container[index].Height / 2.0f + m_container[index].Y;
	mat.m[3][2] = m_container[index].MinZ;

	return mat;
}

















