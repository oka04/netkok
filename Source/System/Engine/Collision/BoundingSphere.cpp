//*****************************************************************************
//
// バウンディングスフィアクラス
//
// BoundingSphere.cpp
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

#include "BoundingSphere.h"

//１つの輪の頂点数
const int BoundingSphere::CIRCLE_VERTEX_NUM = 16;

//バウンディングスフィア頂点数
const int BoundingSphere::FULL_VERTEX_NUM = CIRCLE_VERTEX_NUM * 4;

//バウンディングスフィアインデックス数
const int BoundingSphere::INDEX_NUM = CIRCLE_VERTEX_NUM * 4 + 1;

//=============================================================================
// コンストラクタ
//=============================================================================
BoundingSphere::BoundingSphere() 
	: m_pVB(nullptr)
	, m_pIB(nullptr)
	, m_radius(0)
	, m_pEffect(nullptr)
	, m_pVertexDeclaration(nullptr)
{
	D3DXMatrixIdentity(&m_matWorld);
}

//=============================================================================
// デストラクタ
//=============================================================================
BoundingSphere::~BoundingSphere()
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
// バウンディングスフィアの作成
// 引　数：Engine*     ３Ｄエンジンクラスへのポインタ
// 　　　　const float 半径
// 　　　　const bool  描画フラグ（デバッグ用）デフォルトはfalse
//=============================================================================
void BoundingSphere::CreateBS(Engine* pEngine, const float radius, const bool showFlag)
{
	//バウンディングスフィアデータの設定

	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	//半径の保管
	m_radius = radius;

	//描画する必要がある場合、描画用バウンディングスフィアを作成する
	if (showFlag) {

		Delete();

		//球体は４つの輪で表現する
		//１つの輪のデータを作成し、残り３つは１つ目の輪の角度を変えて設定する

		//頂点バッファの作成
		if (FAILED(pDevice->CreateVertexBuffer(sizeof(D3DXVECTOR3) * FULL_VERTEX_NUM, 0, D3DFVF_XYZ, D3DPOOL_DEFAULT, &m_pVB, nullptr))) {
			throw DxSystemException(DxSystemException::OM_BS_CREATE_VB_ERROR);
		}

		//インデックスバッファの作成
		if (FAILED(pDevice->CreateIndexBuffer(sizeof(WORD) * INDEX_NUM, 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_pIB, nullptr))) {
			throw DxSystemException(DxSystemException::OM_BS_CREATE_IB_ERROR);
		}

		//頂点情報格納用配列

		D3DXVECTOR3 vertices[FULL_VERTEX_NUM];

		//１つ目の輪の頂点を計算

		const double CIRCLE_ANGLE = D3DXToRadian(360.0 / CIRCLE_VERTEX_NUM);

		for (int i = 0; i < CIRCLE_VERTEX_NUM; i++) {
			vertices[i].x = 0;
			vertices[i].y = (float)(cos(CIRCLE_ANGLE * i) * radius);
			vertices[i].z = (float)(sin(CIRCLE_ANGLE * i) * radius);
		}

		//１つ目の輪の角度を変えて、残り３つの座標を計算

		float rotateAngle;

		D3DXMATRIX matRotate;

		rotateAngle = D3DXToRadian(-45.0f);

		D3DXMatrixRotationY(&matRotate, rotateAngle);

		for (int i = 0; i < CIRCLE_VERTEX_NUM; i++) {
			D3DXVec3TransformCoord(&vertices[CIRCLE_VERTEX_NUM + i], &vertices[i], &matRotate);
		}

		rotateAngle = D3DXToRadian(45.0f);

		D3DXMatrixRotationY(&matRotate, rotateAngle);

		for (int i = 0; i < CIRCLE_VERTEX_NUM; i++) {
			D3DXVec3TransformCoord(&vertices[CIRCLE_VERTEX_NUM * 2 + i], &vertices[i], &matRotate);
		}

		rotateAngle = D3DXToRadian(90.0f);

		D3DXMatrixRotationY(&matRotate, rotateAngle);

		for (int i = 0; i < CIRCLE_VERTEX_NUM; i++) {
			D3DXVec3TransformCoord(&vertices[CIRCLE_VERTEX_NUM * 3 + i], &vertices[i], &matRotate);
		}

		//頂点バッファのロックとデータ転送

		void* pVertices;

		if (FAILED(m_pVB->Lock(0, sizeof(vertices), (void**)&pVertices, 0))) {
			throw DxSystemException(DxSystemException::OM_BS_LOCK_VB_ERROR);
		}

		memcpy(pVertices, vertices, sizeof(vertices));

		m_pVB->Unlock();

		//インデックスデータ

		WORD indices[INDEX_NUM] = {
			 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
			16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
			32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
			48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 0,
		};

		//インデックスバッファのロックとデータ転送

		void* pIndex;

		if (FAILED(m_pIB->Lock(0, sizeof(indices), (void**)&pIndex, 0))) {
			throw DxSystemException(DxSystemException::OM_BS_LOCK_IB_ERROR);
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
				throw DxSystemException(DxSystemException::OM_BS_LOAD_RESOURCE_ERROR);
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
				throw DxSystemException(DxSystemException::OM_BS_DECLARE_ERROR);
			}
		}
	}
}

//=============================================================================
// バウンディングスフィアの描画
// 引　数：Engine* ３Ｄエンジンクラスへのポインタ
// 　　　　Camera*           カメラクラスのポインタ
// 　　　　Projection*       プロジェクションクラスのポインタ
// 　　　　D3DCOLORVALUE*    色　デフォルトは白
//=============================================================================
void BoundingSphere::Draw(Engine* pEngine, Camera* pCamera, Projection* pProj, D3DCOLORVALUE* color)
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

		pDevice->DrawIndexedPrimitive(D3DPT_LINESTRIP, 0, 0, FULL_VERTEX_NUM, 0, INDEX_NUM - 1);

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
void BoundingSphere::SetWorldTransform(const LPD3DXMATRIX pMatWorld)
{
	m_matWorld = *pMatWorld;
}

//=============================================================================
// ワールド座標変換行列の取得
// 戻り値：const D3DXMATRIX& ワールド座標変換行列
//=============================================================================
D3DXMATRIX BoundingSphere::GetWorldTransform() const
{
	return m_matWorld;
}

//=============================================================================
// 半径の設定
// 引　数：float 半径
// 備　考：この関数を呼び出してもメッシュの大きさは変わらない
// 　　　　メッシュの大きさも変更したい場合は、再度CreateBSを呼び出す必要がある
//=============================================================================
void BoundingSphere::SetRadius(const float radius)
{
	m_radius = radius;
}

//=============================================================================
// 半径の取得
// 戻り値：float 半径
//=============================================================================
float BoundingSphere::GetRadius() const
{
	return m_radius;
}




//*****************************************************************************
// private関数
//*****************************************************************************


//-----------------------------------------------------------------------------
// 頂点バッファ、インデックスバッファの削除
//-----------------------------------------------------------------------------
void BoundingSphere::Delete()
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
	