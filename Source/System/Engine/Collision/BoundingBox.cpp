//*****************************************************************************
//
// バウンディングボックスクラス
//
// BoundingBox.cpp
//
// K_Yamaoka
//
// 2013/03/04
//
// 2014/10/17 GetWorldTransformMatrix→GetWorldTransform　名称変更
//            SetWorldTransformMatrix→SetWorldTransform　名称変更
//
// 2014/11/28 エラーメッセージを追加
//
// 2020/09/03 シェーダー対応に変更
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "BoundingBox.h"

#include <cassert>

const int BoundingBox::VERTEX_NUM = 8; //バウンディングボックス頂点数

const int BoundingBox::INDEX_NUM = 16; //バウンディングボックスインデックス数

const int BoundingBox::POLYGON_NUM = 12;

//=============================================================================
// コンストラクタ
//=============================================================================
BoundingBox::BoundingBox()
	: m_pVB(nullptr)
	, m_pIB(nullptr)
	, m_pEffect(nullptr)
	, m_pVertexDeclaration(nullptr)
{
	D3DXMatrixIdentity(&m_matWorld);
}

//=============================================================================
// デストラクタ
//=============================================================================
BoundingBox::~BoundingBox()
{
	Delete();

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
// バウンディングボックスの作成
// 引　数：Engine*　　　　　　３Ｄエンジンクラスへのポインタ
// 　　　　const D3DXVECTOR3* 座標の最大値
// 　　　　const D3DXVECTOR3* 座標の最小値
// 　　　　const bool         描画フラグ（デバッグ用）デフォルトはfalse
//=============================================================================
void BoundingBox::CreateBB(Engine* pEngine, const D3DXVECTOR3* pVecMax, const D3DXVECTOR3* pVecMin, const bool showFlag)
{
	//バウンディングボックスデータの設定

	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	m_vertexData.vecMax = *pVecMax;
	m_vertexData.vecMin = *pVecMin;

	//半径のｾｯﾄ
	m_obbData.vecRadius.x = (pVecMax->x - pVecMin->x) / 2.0f;
	m_obbData.vecRadius.y = (pVecMax->y - pVecMin->y) / 2.0f;
	m_obbData.vecRadius.z = (pVecMax->z - pVecMin->z) / 2.0f;

	//中心からのずれ調整用
	m_obbData.vecAdjust.x = pVecMax->x - m_obbData.vecRadius.x;
	m_obbData.vecAdjust.y = pVecMax->y - m_obbData.vecRadius.y;
	m_obbData.vecAdjust.z = pVecMax->z - m_obbData.vecRadius.z;

	//描画する必要がある場合、描画用バウンディングボックスを作成する
	if (showFlag) {

		Delete();

		//ボックスは８つの頂点をラインストリップで結び表現する

		//頂点バッファの作成
		if (FAILED(pDevice->CreateVertexBuffer(sizeof(D3DXVECTOR3) * VERTEX_NUM, 0, D3DFVF_XYZ, D3DPOOL_DEFAULT, &m_pVB, nullptr))) {
			throw DxSystemException(DxSystemException::OM_BB_CREATE_VB_ERROR);
		}

		//インデックスバッファの作成
		if (FAILED(pDevice->CreateIndexBuffer(sizeof(WORD) * INDEX_NUM, 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_pIB, nullptr))) {
			throw DxSystemException(DxSystemException::OM_BB_CREATE_IB_ERROR);
		}

		//頂点情報格納用配列

		D3DXVECTOR3 vertices[VERTEX_NUM] = {
			D3DXVECTOR3(pVecMin->x, pVecMax->y, pVecMin->z),
			D3DXVECTOR3(pVecMax->x, pVecMax->y, pVecMin->z),
			D3DXVECTOR3(pVecMin->x, pVecMin->y, pVecMin->z),
			D3DXVECTOR3(pVecMax->x, pVecMin->y, pVecMin->z),
			D3DXVECTOR3(pVecMin->x, pVecMax->y, pVecMax->z),
			D3DXVECTOR3(pVecMax->x, pVecMax->y, pVecMax->z),
			D3DXVECTOR3(pVecMin->x, pVecMin->y, pVecMax->z),
			D3DXVECTOR3(pVecMax->x, pVecMin->y, pVecMax->z),
		};

		//頂点バッファのロックとデータ転送

		void* pVertices;

		if (FAILED(m_pVB->Lock(0, sizeof(vertices), (void**)&pVertices, 0))) {
			throw DxSystemException(DxSystemException::OM_BB_LOCK_VB_ERROR);
		}

		memcpy(pVertices, vertices, sizeof(vertices));

		m_pVB->Unlock();

		//インデックスデータ

		WORD indices[INDEX_NUM] = { 0, 1, 3, 2, 0, 4, 5, 7, 6, 4, 6, 2, 3, 7, 5, 1 };

		//インデックスバッファのロックとデータ転送

		void* pIndex;

		if (FAILED(m_pIB->Lock(0, sizeof(indices), (void**)&pIndex, 0))) {
			throw DxSystemException(DxSystemException::OM_BB_LOCK_IB_ERROR);
		}

		memcpy(pIndex, indices, sizeof(indices));

		m_pIB->Unlock();

		//シェーダー設定
		
		//シェーダーの読み込み
		if (!m_pEffect) {

#ifdef _DEBUG
			HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_COLLISION_EFFECT), nullptr, nullptr, D3DXSHADER_DEBUG, nullptr, &m_pEffect, nullptr);
#else
			HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_COLLISION_EFFECT), nullptr, nullptr, 0, nullptr, &m_pEffect, nullptr);
#endif
			if (FAILED(hr)) {
				throw DxSystemException(DxSystemException::OM_BB_LOAD_RESOURCE_ERROR);
			}
		}

		//頂点宣言
		D3DVERTEXELEMENT9 VertexElement[] = {
			{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
			D3DDECL_END()
		};

		//頂点宣言の作成
		if (!m_pVertexDeclaration) {
			HRESULT hr = pDevice->CreateVertexDeclaration(VertexElement, &m_pVertexDeclaration);
			if (FAILED(hr)) {
				throw DxSystemException(DxSystemException::OM_BB_DECLARE_ERROR);
			}
		}
	}
}

//=============================================================================
// バウンディングボックスの描画
// 引　数：Engine*　３Ｄエンジンクラスへのポインタ
// 　　　　Camera*           カメラクラスのポインタ
// 　　　　Projection*       プロジェクションクラスのポインタ
// 　　　　D3DCOLORVALUE*    色　デフォルトは白
//=============================================================================
void BoundingBox::Draw(Engine* pEngine, Camera* pCamera, Projection* pProj, D3DCOLORVALUE* color)
{
	if (m_pVB && m_pIB) {

		LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

		pDevice->SetVertexDeclaration(m_pVertexDeclaration);

		D3DXMATRIX matView = pCamera->GetViewMatrix();
		D3DXMATRIX matProj = pProj->GetProjectionMatrix();

		D3DXMATRIX matWorld = m_matWorld * matView * matProj;

		//ワールドビュー射影変換行列を渡す
		m_pEffect->SetMatrix("gMatWVP", &matWorld);

		//マテリアルのディフューズとアンビエントを渡す
		if (color) {
			m_pEffect->SetValue("gMaterialDiffuse", color, sizeof(D3DCOLORVALUE));
		}
		else {
			D3DCOLORVALUE white = { 1.0f, 1.0f, 1.0f, 1.0f };
			m_pEffect->SetValue("gMaterialDiffuse", &white, sizeof(D3DCOLORVALUE));
		}

		//テクニックの設定
		m_pEffect->SetTechnique("CollisionTec");

		DWORD saveFVF;

		pDevice->GetFVF(&saveFVF);

		pDevice->SetFVF(D3DFVF_XYZ);

		pDevice->SetStreamSource(0, m_pVB, 0, sizeof(D3DXVECTOR3));

		pDevice->SetIndices(m_pIB);

		pDevice->SetTexture(0, nullptr);

		pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

		UINT numPass;
		m_pEffect->Begin(&numPass, 0);
		m_pEffect->BeginPass(0);

		pDevice->DrawIndexedPrimitive(D3DPT_LINESTRIP, 0, 0, VERTEX_NUM, 0, INDEX_NUM - 1);

		m_pEffect->EndPass();
		m_pEffect->End();

		pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

		pDevice->SetFVF(saveFVF);
	}
}

//=============================================================================
// ワールド座標変換行列の設定
// 引　数：const LPD3DXMATRIX ワールド座標変換行列へのポインタ
//=============================================================================
void BoundingBox::SetWorldTransform(const LPD3DXMATRIX pMatWorld)
{
	m_matWorld = *pMatWorld;
}

//=============================================================================
//　ﾜｰﾙﾄﾞ座標変換行列の取得
//　戻り値：ﾜｰﾙﾄﾞ座標変換行列
//=============================================================================
D3DXMATRIX BoundingBox::GetWorldTransform() const
{
	return m_matWorld;
}

//=============================================================================
// バウンディングボックス　頂点データの取得
// 戻り値：BB_VertexData 頂点データ構造体
//=============================================================================
BB_VertexData BoundingBox::GetVertexData() const
{
	return m_vertexData;
}

//=============================================================================
// バウンディングボックス　ＯＢＢ用データの取得
// 戻り値：BB_ObbData* ＯＢＢ用データへのポインタ（更新の必要があるため）
//=============================================================================
BB_ObbData* BoundingBox::GetObbData()
{
	return &m_obbData;
}

//=============================================================================
// レイとの当たり判定用のポリゴン取得
// 戻り値：BB_Polygon ポリゴンの頂点情報構造体
// 引　数：const int ポリゴンのインデックス（０～１１）
//=============================================================================
BB_Polygon BoundingBox::GetPolygon(const int index)
{
	assert(index >= 0 && index < POLYGON_NUM);

	const D3DXVECTOR3 POLYGON_DATA[12] = {
		D3DXVECTOR3(m_vertexData.vecMin.x, m_vertexData.vecMax.y, m_vertexData.vecMin.z),
		D3DXVECTOR3(m_vertexData.vecMax.x, m_vertexData.vecMax.y, m_vertexData.vecMin.z),
		D3DXVECTOR3(m_vertexData.vecMin.x, m_vertexData.vecMin.y, m_vertexData.vecMin.z),
		D3DXVECTOR3(m_vertexData.vecMax.x, m_vertexData.vecMin.y, m_vertexData.vecMin.z),
		D3DXVECTOR3(m_vertexData.vecMax.x, m_vertexData.vecMax.y, m_vertexData.vecMax.z),
		D3DXVECTOR3(m_vertexData.vecMin.x, m_vertexData.vecMax.y, m_vertexData.vecMax.z),
		D3DXVECTOR3(m_vertexData.vecMax.x, m_vertexData.vecMin.y, m_vertexData.vecMax.z),
		D3DXVECTOR3(m_vertexData.vecMin.x, m_vertexData.vecMin.y, m_vertexData.vecMax.z),
	};

	const int INDEX_DATA[36] = {
		0, 1, 2, 1, 3, 2,
		1, 4, 3, 4, 6, 3,
		4, 5, 6, 5, 7, 6,
		5, 0, 7, 0, 2, 7,
		5, 4, 0, 4, 1, 0,
		2, 3, 7, 3, 6, 7,
	};

	m_polygon.vertex[0] = POLYGON_DATA[INDEX_DATA[index * 3 + 0]];
	m_polygon.vertex[1] = POLYGON_DATA[INDEX_DATA[index * 3 + 1]];
	m_polygon.vertex[2] = POLYGON_DATA[INDEX_DATA[index * 3 + 2]];

	return m_polygon;
}




//*****************************************************************************
// private関数
//*****************************************************************************


//-----------------------------------------------------------------------------
// 頂点バッファ、インデックスバッファの削除
//-----------------------------------------------------------------------------
void BoundingBox::Delete()
{
	if (m_pIB) {
		m_pIB->Release();
		m_pIB = nullptr;
	}

	if (m_pVB) {
		m_pVB->Release();
		m_pVB = nullptr;
	}
}
	