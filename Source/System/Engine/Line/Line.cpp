//*****************************************************************************
//
// ラインクラス
//
// Line.cpp
//
// K_Yamaoka
//
// 2017/06/19
//
// 2020/11/26 Draw（３Ｄ用）をシェーダー対応にするため追加
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "Line.h"

//=============================================================================
// コンストラクタ
//=============================================================================
Line::Line()
	: m_pLine(nullptr)
	, m_pEffect(nullptr)
	, m_pVertexDeclaration(nullptr)
{

}

//=============================================================================
// デストラクタ
//=============================================================================
Line::~Line()
{
	if (m_pLine) {
		m_pLine->Release();
		m_pLine = nullptr;
	}

	if (m_pVertexDeclaration) {
		m_pVertexDeclaration->Release();
		m_pVertexDeclaration = nullptr;
	}

	if (m_pEffect) {
		m_pEffect->Release();
		m_pEffect = nullptr;
	}
}

//=============================================================================
// 初期化
// 引　数：デバイスのポインタ
//=============================================================================
void Line::Initialize(LPDIRECT3DDEVICE9 pDevice)
{
	m_pDevice = pDevice;

	HRESULT hResult = D3DXCreateLine(m_pDevice, &m_pLine);

	if (FAILED(hResult)) {
		throw new DxSystemException(DxSystemException::OM_CREATE_LINE_OBJECT_ERROR);
	}

	//シェーダーの読み込み
	if (!m_pEffect) {

#ifdef _DEBUG
		HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_LINE3D_EFFECT), nullptr, nullptr, D3DXSHADER_DEBUG, nullptr, &m_pEffect, nullptr);
#else
		HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_LINE3D_EFFECT), nullptr, nullptr, 0, nullptr, &m_pEffect, nullptr);
#endif
		if (FAILED(hr)) {
			throw DxSystemException(DxSystemException::OM_PARTICLE_LOAD_RESOURCE_ERROR);
		}
	}

	//頂点宣言
	D3DVERTEXELEMENT9 VertexElement[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
		D3DDECL_END()
	};

	//頂点宣言の作成
	if (!m_pVertexDeclaration) {
		HRESULT hr = pDevice->CreateVertexDeclaration(VertexElement, &m_pVertexDeclaration);
		if (FAILED(hr)) {
			throw DxSystemException(DxSystemException::OM_PARTICLE_DECLARE_ERROR);
		}
	}
}

//=============================================================================
// 線の描画（２Ｄ）
// 引　数：頂点配列
// 　　　　頂点の個数
// 　　　　頂点カラー
//=============================================================================
void Line::Draw(const D3DXVECTOR2 *pVertexList, DWORD dwVertexListCount, D3DCOLOR color)
{
	m_pLine->Begin();

	m_pLine->Draw(pVertexList, dwVertexListCount, color);

	m_pLine->End();
}

//=============================================================================
// 直線の描画（３Ｄ）
// 引　数：開始位置（３Ｄベクトル）
// 　　　　終了位置（３Ｄベクトル）
// 　　　　線の色
// 　　　　ビュー変換行列のポインタ
// 　　　　プロジェクション変換行列のポインタ
//=============================================================================
void Line::Draw(const D3DXVECTOR3 startPosition, const D3DXVECTOR3 endPosition, D3DCOLOR color, const D3DXMATRIX* pView, const D3DXMATRIX* pProjection)
{
	Line3DData line[2];

	line[0].position = startPosition;
	line[1].position = endPosition;

	D3DCOLORVALUE colorValue;	
	colorValue.a = (color >> 24) / 255.0f;
	colorValue.r = ((color >> 16) & 0x000000FF) / 255.0f;
	colorValue.g = ((color >> 8) & 0x000000FF) / 255.0f;
	colorValue.b = (color & 0x000000FF) / 255.0f;

	line[0].color = colorValue;
	line[1].color = colorValue;

	D3DXMATRIX matVP = *pView * *pProjection;

	//ワールドビュー射影変換行列を渡す
	m_pEffect->SetMatrix("gMatVP", &matVP);

	//テクニックの設定
	m_pEffect->SetTechnique("Line3DTec");

	m_pDevice->SetVertexDeclaration(m_pVertexDeclaration);

	//パス０で描画
	UINT numPass;
	m_pEffect->Begin(&numPass, 0);
	m_pEffect->BeginPass(0);

	m_pDevice->DrawPrimitiveUP(D3DPT_LINELIST, 1, line, sizeof(Line3DData));

	m_pEffect->EndPass();
	m_pEffect->End();
}
