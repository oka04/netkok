//*****************************************************************************
//
// プリミティブ（基本形状）クラス
//
// Primitive.cpp
//
// K_Yamaoka
//
// 2013/03/04
//
// 2014/11/28 エラーメッセージを追加
//
// 2020/08/25 シェーダー対応に変更
//
// 2020/09/04 マテリアルのデフォルト値を変更
//
// 2021/05/13 Draw関数のSphere描画を修正
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "Primitive.h"

#include <cassert>

const int Primitive::MAX_SPOT_LIGHTS = 4;

//=============================================================================
// コンストラクタ
//=============================================================================
Primitive::Primitive()
	: m_type(NONE)
	, m_pMesh(nullptr)
	, m_pTexture(nullptr)
	, m_pEffect(nullptr)
	, m_pVertexDeclaration(nullptr)
{
	//マテリアルの初期化
	ZeroMemory(&m_material, sizeof(D3DMATERIAL9));

	m_material.Diffuse.r = 1.0f;
	m_material.Diffuse.g = 1.0f;
	m_material.Diffuse.b = 1.0f;
	m_material.Diffuse.a = 1.0f;

	m_material.Ambient.r = 1.0f;
	m_material.Ambient.g = 1.0f;
	m_material.Ambient.b = 1.0f;
	m_material.Ambient.a = 1.0f;

	//ワールド座標変換行列の初期化
	D3DXMatrixIdentity(&m_matWorld);
}

//=============================================================================
// デストラクタ
//=============================================================================
Primitive::~Primitive()
{
	DeleteTexture();
	DeleteMesh();

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
// 三角ポリゴン（１つ）の作成
// 引　数：Engine*           ３Ｄエンジンクラスへのポインタ
// 　　　　const D3DXVECTOR3 ポリゴンを構成する３頂点の座標（配列）
//=============================================================================
void Primitive::CreateTriangle(Engine* pEngine, const D3DXVECTOR3 vecPosition[3])
{
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	DeleteMesh();

	//シェーダーの読み込み
	if (!m_pEffect) {

#ifdef _DEBUG
		HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_PRIMITIVE_POS_ONLY_EFFECT), nullptr, nullptr, D3DXSHADER_DEBUG, nullptr, &m_pEffect, nullptr);
#else
		HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_PRIMITIVE_POS_ONLY_EFFECT), nullptr, nullptr, 0, nullptr, &m_pEffect, nullptr);
#endif
		if (FAILED(hr)) {
			throw DxSystemException(DxSystemException::OM_PRIMITIVE_LOAD_RESOURCE_ERROR);
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
			throw DxSystemException(DxSystemException::OM_PRIMITIVE_DECLARE_ERROR);
		}
	}

	//メッシュの作成
	HRESULT hResult = D3DXCreateMesh(1, 3, D3DXMESH_MANAGED, VertexElement, pDevice, &m_pMesh);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PRIMITIVE_CREATE_MESH_ERROR);
	}

	//頂点バッファへのアクセス

	void* pVertex; //頂点情報の先頭番地

	//頂点バッファのロック
	hResult = m_pMesh->LockVertexBuffer(0, &pVertex);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PRIMITIVE_LOCK_VB_ERROR);
	}

	//頂点座標の設定

	for (int i = 0; i < 3; i++) {
		((D3DXVECTOR3*)pVertex)[i] = vecPosition[i];
	}

	//バッファのアンロック
	m_pMesh->UnlockVertexBuffer();

	//インデックスバッファへのアクセス

	void* pIndex; //インデックス情報の先頭番地

	//インデックスバッファのロック
	hResult = m_pMesh->LockIndexBuffer(0, &pIndex);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PRIMITIVE_LOCK_IB_ERROR);
	}

	//インデックスの設定
	for (int i = 0; i < 3; i++) {
		((WORD*)(pIndex))[i] = (WORD)i;
	}

	//バッファのアンロック
	m_pMesh->UnlockIndexBuffer();

	//型設定
	m_type = TRIANGLE_XYZ;
}

//=============================================================================
// 三角ポリゴン（１つ）の作成：法線付き
// 引　数：Engine*            ３Ｄエンジンクラスへのポインタ
// 　　　　const D3DXVECTOR3  ポリゴンを構成する３頂点の座標（配列）
// 　　　　const D3DXVECTOR3* 法線ベクトルへのポインタ（単位ベクトル）
//=============================================================================
void Primitive::CreateTriangle(Engine* pEngine, const D3DXVECTOR3 vecPosition[3], const D3DXVECTOR3* pVecNormal)
{
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	DeleteMesh();

	//シェーダーの読み込み
	if (!m_pEffect) {

#ifdef _DEBUG
		HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_PRIMITIVE_NORMAL_EFFECT), nullptr, nullptr, D3DXSHADER_DEBUG, nullptr, &m_pEffect, nullptr);
#else
		HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_PRIMITIVE_NORMAL_EFFECT), nullptr, nullptr, 0, nullptr, &m_pEffect, nullptr);
#endif
		if (FAILED(hr)) {
			throw DxSystemException(DxSystemException::OM_PRIMITIVE_LOAD_RESOURCE_ERROR);
		}
	}

	//頂点宣言
	D3DVERTEXELEMENT9 VertexElement[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		D3DDECL_END()
	};

	//頂点宣言の作成
	if (!m_pVertexDeclaration) {
		HRESULT hr = pDevice->CreateVertexDeclaration(VertexElement, &m_pVertexDeclaration);
		if (FAILED(hr)) {
			throw DxSystemException(DxSystemException::OM_PRIMITIVE_DECLARE_ERROR);
		}
	}

	//メッシュの作成
	HRESULT hResult = D3DXCreateMesh(1, 3, D3DXMESH_MANAGED, VertexElement, pDevice, &m_pMesh);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PRIMITIVE_CREATE_MESH_ERROR);
	}

	//頂点バッファへのアクセス

	void* pVertex; //頂点情報の先頭番地

	//頂点バッファのロック
	hResult = m_pMesh->LockVertexBuffer(0, &pVertex);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PRIMITIVE_LOCK_VB_ERROR);
	}

	//頂点座標の設定

	*((D3DXVECTOR3*)pVertex + 0) = vecPosition[0];
	*((D3DXVECTOR3*)pVertex + 1) = *pVecNormal;

	*((D3DXVECTOR3*)pVertex + 2) = vecPosition[1];
	*((D3DXVECTOR3*)pVertex + 3) = *pVecNormal;

	*((D3DXVECTOR3*)pVertex + 4) = vecPosition[2];
	*((D3DXVECTOR3*)pVertex + 5) = *pVecNormal;

	//バッファのアンロック
	m_pMesh->UnlockVertexBuffer();

	//インデックスバッファへのアクセス

	void* pIndex; //インデックス情報の先頭番地

	//インデックスバッファのロック
	hResult = m_pMesh->LockIndexBuffer(0, &pIndex);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PRIMITIVE_LOCK_IB_ERROR);
	}

	//インデックスの設定
	for (int i = 0; i < 3; i++) {
		((WORD*)(pIndex))[i] = (WORD)i;
	}

	//バッファのアンロック
	m_pMesh->UnlockIndexBuffer();

	//型設定
	m_type = TRIANGLE_XYZ_NORMAL;
}

//=============================================================================
// 三角ポリゴン（１つ）の作成：法線、テクスチャ座標付き
// 引　数：Engine*            ３Ｄエンジンクラスへのポインタ
// 　　　　const D3DXVECTOR3  ポリゴンを構成する３頂点の座標（配列）
// 　　　　const D3DXVECTOR3* 法線ベクトルへのポインタ（単位ベクトル）
// 　　　　const Coord        テクスチャ座標構造体配列
//=============================================================================
void Primitive::CreateTriangle(Engine* pEngine, const D3DXVECTOR3 vecPosition[3], const D3DXVECTOR3* pVecNormal, const Coord coords[3])
{
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	DeleteMesh();

	//シェーダーの読み込み
	if (!m_pEffect) {

#ifdef _DEBUG
		HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_PRIMITIVE_TEXTURE_EFFECT), nullptr, nullptr, D3DXSHADER_DEBUG, nullptr, &m_pEffect, nullptr);
#else
		HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_PRIMITIVE_TEXTURE_EFFECT), nullptr, nullptr, 0, nullptr, &m_pEffect, nullptr);
#endif
		if (FAILED(hr)) {
			throw DxSystemException(DxSystemException::OM_PRIMITIVE_LOAD_RESOURCE_ERROR);
		}
	}

	//頂点宣言
	D3DVERTEXELEMENT9 VertexElement[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	//頂点宣言の作成
	if (!m_pVertexDeclaration) {
		HRESULT hr = pDevice->CreateVertexDeclaration(VertexElement, &m_pVertexDeclaration);
		if (FAILED(hr)) {
			throw DxSystemException(DxSystemException::OM_PRIMITIVE_DECLARE_ERROR);
		}
	}

	//メッシュの作成
	HRESULT hResult = D3DXCreateMesh(1, 3, D3DXMESH_MANAGED, VertexElement, pDevice, &m_pMesh);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PRIMITIVE_CREATE_MESH_ERROR);
	}

	//頂点バッファへのアクセス

	void* pVertex; //頂点情報の先頭番地

	//頂点バッファのロック
	hResult = m_pMesh->LockVertexBuffer(0, &pVertex);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PRIMITIVE_LOCK_VB_ERROR);
	}

	//頂点座標の設定

	DWORD numVertexBytes = sizeof(D3DXVECTOR3) * 2 + sizeof(float) * 2;

	for (int i = 0; i < 3; i++) {
		*(((D3DXVECTOR3*)((BYTE*)pVertex + numVertexBytes * i)) + 0) = vecPosition[i];
		*(((D3DXVECTOR3*)((BYTE*)pVertex + numVertexBytes * i)) + 1) = *pVecNormal;
		*(((float*)((BYTE*)pVertex + numVertexBytes * i)) + 6) = coords[i].u;
		*(((float*)((BYTE*)pVertex + numVertexBytes * i)) + 7) = coords[i].v;
	}

	//バッファのアンロック
	m_pMesh->UnlockVertexBuffer();

	//インデックスバッファへのアクセス

	void* pIndex; //インデックス情報の先頭番地

	//インデックスバッファのロック
	hResult = m_pMesh->LockIndexBuffer(0, &pIndex);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PRIMITIVE_LOCK_IB_ERROR);
	}

	//インデックスの設定
	for (int i = 0; i < 3; i++) {
		((WORD*)(pIndex))[i] = (WORD)i;
	}

	//バッファのアンロック
	m_pMesh->UnlockIndexBuffer();

	//型設定
	m_type = TRIANGLE_XYZ_NORMAL_UV;
}

//=============================================================================
// 板ポリゴンの作成：法線、テクスチャ座標付き
// 引　数：Engine*     ３Ｄエンジンクラスへのポインタ
// 　　　　const float 幅
// 　　　　const float 高さ
//=============================================================================
void Primitive::CreateRectangle(Engine* pEngine, const float width, const float height)
{
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	DeleteMesh();

	//シェーダーの読み込み
	if (!m_pEffect) {

#ifdef _DEBUG
		HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_PRIMITIVE_TEXTURE_EFFECT), nullptr, nullptr, D3DXSHADER_DEBUG, nullptr, &m_pEffect, nullptr);
#else
		HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_PRIMITIVE_TEXTURE_EFFECT), nullptr, nullptr, 0, nullptr, &m_pEffect, nullptr);
#endif
		if (FAILED(hr)) {
			throw DxSystemException(DxSystemException::OM_PRIMITIVE_LOAD_RESOURCE_ERROR);
		}
	}

	//頂点宣言
	D3DVERTEXELEMENT9 VertexElement[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	//頂点宣言の作成
	if (!m_pVertexDeclaration) {
		HRESULT hr = pDevice->CreateVertexDeclaration(VertexElement, &m_pVertexDeclaration);
		if (FAILED(hr)) {
			throw DxSystemException(DxSystemException::OM_PRIMITIVE_DECLARE_ERROR);
		}
	}

	//メッシュの作成
	HRESULT hResult = D3DXCreateMesh(2, 6, D3DXMESH_MANAGED, VertexElement, pDevice, &m_pMesh);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PRIMITIVE_CREATE_MESH_ERROR);
	}

	//頂点データ
	D3DXVECTOR3 vertices[6] = {
		D3DXVECTOR3(-width / 2,  height / 2, 0),
		D3DXVECTOR3(width / 2,  height / 2, 0),
		D3DXVECTOR3(-width / 2, -height / 2, 0),
		D3DXVECTOR3(width / 2,  height / 2, 0),
		D3DXVECTOR3(width / 2, -height / 2, 0),
		D3DXVECTOR3(-width / 2, -height / 2, 0),
	};

	//法線データ
	D3DXVECTOR3 normals = D3DXVECTOR3(0, 0, -1);

	//テクスチャ座標
	Coord coords[6] = {
		Coord(0.0f, 0.0f),
		Coord(1.0f, 0.0f),
		Coord(0.0f, 1.0f),
		Coord(1.0f, 0.0f),
		Coord(1.0f, 1.0f),
		Coord(0.0f, 1.0f),
	};

	//頂点バッファへのアクセス

	void* pVertex; //頂点情報の先頭番地

	//頂点バッファのロック
	hResult = m_pMesh->LockVertexBuffer(0, &pVertex);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PRIMITIVE_LOCK_VB_ERROR);
	}

	//頂点座標の設定

	DWORD numVertexBytes = sizeof(D3DXVECTOR3) * 2 + sizeof(float) * 2;

	for (int i = 0; i < 6; i++) {
		*(((D3DXVECTOR3*)((BYTE*)pVertex + numVertexBytes * i)) + 0) = vertices[i];
		*(((D3DXVECTOR3*)((BYTE*)pVertex + numVertexBytes * i)) + 1) = normals;
		*(((float*)((BYTE*)pVertex + numVertexBytes * i)) + 6) = coords[i].u;
		*(((float*)((BYTE*)pVertex + numVertexBytes * i)) + 7) = coords[i].v;
	}

	//バッファのアンロック
	m_pMesh->UnlockVertexBuffer();

	//インデックスバッファへのアクセス

	void* pIndex; //インデックス情報の先頭番地

	//インデックスバッファのロック
	hResult = m_pMesh->LockIndexBuffer(0, &pIndex);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PRIMITIVE_LOCK_IB_ERROR);
	}

	//インデックスの設定
	for (int i = 0; i < 6; i++) {
		((WORD*)(pIndex))[i] = (WORD)i;
	}

	//バッファのアンロック
	m_pMesh->UnlockIndexBuffer();

	//型設定
	m_type = RECTANGLE;
}

//=============================================================================
// ボックスの作成：法線、テクスチャ座標付き
// 引　数：Engine*     ３Ｄエンジンクラスへのポインタ
// 　　　　const float 幅
// 　　　　const float 高さ
// 　　　　const float 奥行
//=============================================================================
void Primitive::CreateBox(Engine* pEngine, const float width, const float height, const float depth)
{
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	DeleteMesh();

	//シェーダーの読み込み
	if (!m_pEffect) {

#ifdef _DEBUG
		HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_PRIMITIVE_TEXTURE_EFFECT), nullptr, nullptr, D3DXSHADER_DEBUG, nullptr, &m_pEffect, nullptr);
#else
		HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_PRIMITIVE_TEXTURE_EFFECT), nullptr, nullptr, 0, nullptr, &m_pEffect, nullptr);
#endif
		if (FAILED(hr)) {
			throw DxSystemException(DxSystemException::OM_PRIMITIVE_LOAD_RESOURCE_ERROR);
		}
	}

	//頂点宣言
	D3DVERTEXELEMENT9 VertexElement[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	//頂点宣言の作成
	if (!m_pVertexDeclaration) {
		HRESULT hr = pDevice->CreateVertexDeclaration(VertexElement, &m_pVertexDeclaration);
		if (FAILED(hr)) {
			throw DxSystemException(DxSystemException::OM_PRIMITIVE_DECLARE_ERROR);
		}
	}

	//メッシュの作成
	HRESULT hResult = D3DXCreateMesh(12, 36, D3DXMESH_MANAGED, VertexElement, pDevice, &m_pMesh);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PRIMITIVE_CREATE_MESH_ERROR);
	}

	//頂点データ
	D3DXVECTOR3 vertices[36] = {
		D3DXVECTOR3(-width / 2,  height / 2, -depth / 2),
		D3DXVECTOR3(width / 2,  height / 2, -depth / 2),
		D3DXVECTOR3(-width / 2, -height / 2, -depth / 2),
		D3DXVECTOR3(width / 2,  height / 2, -depth / 2),
		D3DXVECTOR3(width / 2, -height / 2, -depth / 2),
		D3DXVECTOR3(-width / 2, -height / 2, -depth / 2),
		D3DXVECTOR3(width / 2,  height / 2, -depth / 2),
		D3DXVECTOR3(width / 2,  height / 2,  depth / 2),
		D3DXVECTOR3(width / 2, -height / 2, -depth / 2),
		D3DXVECTOR3(width / 2,  height / 2,  depth / 2),
		D3DXVECTOR3(width / 2, -height / 2,  depth / 2),
		D3DXVECTOR3(width / 2, -height / 2, -depth / 2),
		D3DXVECTOR3(width / 2,  height / 2,  depth / 2),
		D3DXVECTOR3(-width / 2,  height / 2,  depth / 2),
		D3DXVECTOR3(width / 2, -height / 2,  depth / 2),
		D3DXVECTOR3(-width / 2,  height / 2,  depth / 2),
		D3DXVECTOR3(-width / 2, -height / 2,  depth / 2),
		D3DXVECTOR3(width / 2, -height / 2,  depth / 2),
		D3DXVECTOR3(-width / 2,  height / 2,  depth / 2),
		D3DXVECTOR3(-width / 2,  height / 2, -depth / 2),
		D3DXVECTOR3(-width / 2, -height / 2,  depth / 2),
		D3DXVECTOR3(-width / 2,  height / 2, -depth / 2),
		D3DXVECTOR3(-width / 2, -height / 2, -depth / 2),
		D3DXVECTOR3(-width / 2, -height / 2,  depth / 2),
		D3DXVECTOR3(-width / 2,  height / 2,  depth / 2),
		D3DXVECTOR3(width / 2,  height / 2,  depth / 2),
		D3DXVECTOR3(-width / 2,  height / 2, -depth / 2),
		D3DXVECTOR3(width / 2,  height / 2,  depth / 2),
		D3DXVECTOR3(width / 2,  height / 2, -depth / 2),
		D3DXVECTOR3(-width / 2,  height / 2, -depth / 2),
		D3DXVECTOR3(width / 2, -height / 2, -depth / 2),
		D3DXVECTOR3(width / 2, -height / 2,  depth / 2),
		D3DXVECTOR3(-width / 2, -height / 2,  depth / 2),
		D3DXVECTOR3(-width / 2, -height / 2, -depth / 2),
		D3DXVECTOR3(width / 2, -height / 2, -depth / 2),
		D3DXVECTOR3(-width / 2, -height / 2,  depth / 2),
	};

	//法線データ
	D3DXVECTOR3 normals[36] = {
		D3DXVECTOR3(0,  0, -1),
		D3DXVECTOR3(0,  0, -1),
		D3DXVECTOR3(0,  0, -1),
		D3DXVECTOR3(0,  0, -1),
		D3DXVECTOR3(0,  0, -1),
		D3DXVECTOR3(0,  0, -1),
		D3DXVECTOR3(1,  0,  0),
		D3DXVECTOR3(1,  0,  0),
		D3DXVECTOR3(1,  0,  0),
		D3DXVECTOR3(1,  0,  0),
		D3DXVECTOR3(1,  0,  0),
		D3DXVECTOR3(1,  0,  0),
		D3DXVECTOR3(0,  0,  1),
		D3DXVECTOR3(0,  0,  1),
		D3DXVECTOR3(0,  0,  1),
		D3DXVECTOR3(0,  0,  1),
		D3DXVECTOR3(0,  0,  1),
		D3DXVECTOR3(0,  0,  1),
		D3DXVECTOR3(-1,  0,  0),
		D3DXVECTOR3(-1,  0,  0),
		D3DXVECTOR3(-1,  0,  0),
		D3DXVECTOR3(-1,  0,  0),
		D3DXVECTOR3(-1,  0,  0),
		D3DXVECTOR3(-1,  0,  0),
		D3DXVECTOR3(0,  1,  0),
		D3DXVECTOR3(0,  1,  0),
		D3DXVECTOR3(0,  1,  0),
		D3DXVECTOR3(0,  1,  0),
		D3DXVECTOR3(0,  1,  0),
		D3DXVECTOR3(0,  1,  0),
		D3DXVECTOR3(0, -1,  0),
		D3DXVECTOR3(0, -1,  0),
		D3DXVECTOR3(0, -1,  0),
		D3DXVECTOR3(0, -1,  0),
		D3DXVECTOR3(0, -1,  0),
		D3DXVECTOR3(0, -1,  0),
	};

	//テクスチャ座標
	Coord coords[36] = {
		Coord(0.0f, 0.0f),
		Coord(1.0f, 0.0f),
		Coord(0.0f, 1.0f),
		Coord(1.0f, 0.0f),
		Coord(1.0f, 1.0f),
		Coord(0.0f, 1.0f),
		Coord(0.0f, 0.0f),
		Coord(1.0f, 0.0f),
		Coord(0.0f, 1.0f),
		Coord(1.0f, 0.0f),
		Coord(1.0f, 1.0f),
		Coord(0.0f, 1.0f),
		Coord(0.0f, 0.0f),
		Coord(1.0f, 0.0f),
		Coord(0.0f, 1.0f),
		Coord(1.0f, 0.0f),
		Coord(1.0f, 1.0f),
		Coord(0.0f, 1.0f),
		Coord(0.0f, 0.0f),
		Coord(1.0f, 0.0f),
		Coord(0.0f, 1.0f),
		Coord(1.0f, 0.0f),
		Coord(1.0f, 1.0f),
		Coord(0.0f, 1.0f),
		Coord(0.0f, 0.0f),
		Coord(1.0f, 0.0f),
		Coord(0.0f, 1.0f),
		Coord(1.0f, 0.0f),
		Coord(1.0f, 1.0f),
		Coord(0.0f, 1.0f),
		Coord(0.0f, 0.0f),
		Coord(1.0f, 0.0f),
		Coord(0.0f, 1.0f),
		Coord(1.0f, 0.0f),
		Coord(1.0f, 1.0f),
		Coord(0.0f, 1.0f),
	};


	//頂点バッファへのアクセス

	void* pVertex; //頂点情報の先頭番地

	//頂点バッファのロック
	hResult = m_pMesh->LockVertexBuffer(0, &pVertex);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PRIMITIVE_LOCK_VB_ERROR);
	}

	//頂点座標の設定

	DWORD numVertexBytes = sizeof(D3DXVECTOR3) * 2 + sizeof(float) * 2;

	for (int i = 0; i < 36; i++) {
		*(((D3DXVECTOR3*)((BYTE*)pVertex + numVertexBytes * i)) + 0) = vertices[i];
		*(((D3DXVECTOR3*)((BYTE*)pVertex + numVertexBytes * i)) + 1) = normals[i];
		*(((float*)((BYTE*)pVertex + numVertexBytes * i)) + 6) = coords[i].u;
		*(((float*)((BYTE*)pVertex + numVertexBytes * i)) + 7) = coords[i].v;
	}

	//バッファのアンロック
	m_pMesh->UnlockVertexBuffer();

	//インデックスバッファへのアクセス

	void* pIndex; //インデックス情報の先頭番地

	//インデックスバッファのロック
	hResult = m_pMesh->LockIndexBuffer(0, &pIndex);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PRIMITIVE_LOCK_IB_ERROR);
	}

	//インデックスの設定
	for (int i = 0; i < 36; i++) {
		((WORD*)(pIndex))[i] = (WORD)i;
	}

	//バッファのアンロック
	m_pMesh->UnlockIndexBuffer();

	m_type = BOX;
}

//=============================================================================
// 球（スフィア）の作成：法線、テクスチャ座標付き
// 引　数：Engine*     ３Ｄエンジンクラスへのポインタ
// 　　　　const float 半径
// 　　　　const float セグメント数
//=============================================================================
void Primitive::CreateSphere(Engine* pEngine, const float radius, const unsigned int segment)
{
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	DeleteMesh();

	LPD3DXMESH pMesh = nullptr;

	//球体の作成
	HRESULT hResult = D3DXCreateSphere(pDevice, radius, segment, segment, &pMesh, nullptr);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PRIMITIVE_CREATE_SPHERE_ERROR);
	}

	//法線計算
	hResult = D3DXComputeNormals(pMesh, nullptr);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PRIMITIVE_COMPUTE_NORMAL_ERROR);
	}

	//クローンの作成
	hResult = pMesh->CloneMeshFVF(D3DXMESH_MANAGED, D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1, pDevice, &m_pMesh);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PRIMITIVE_CLONE_FVF_ERROR);
	}

	pMesh->Release();
	pMesh = nullptr;

	//ＵＶ座標計算

	DWORD numVertexBytes = m_pMesh->GetNumBytesPerVertex();
	DWORD numVertices = m_pMesh->GetNumVertices();

	void* pVertex = nullptr;

	//頂点バッファのロック
	hResult = m_pMesh->LockVertexBuffer(0, &pVertex);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PRIMITIVE_LOCK_VB_ERROR);
	}

	float x, y, z, phi, u, v;

	for (DWORD i = 0; i < numVertices; i++) {

		x = ((D3DXVECTOR3*)((BYTE*)pVertex + numVertexBytes * i))->x / radius;
		y = ((D3DXVECTOR3*)((BYTE*)pVertex + numVertexBytes * i))->y / radius;
		z = ((D3DXVECTOR3*)((BYTE*)pVertex + numVertexBytes * i))->z / radius;

		phi = acos(y);

		v = (float)(phi / D3DX_PI);

		if (y == 1.0f || y == -1.0f) {
			u = 0.5f;
		}
		else {
			u = (float)(acos(max(-1.0, min(1.0, z / sin(phi)))) / (2.0 * D3DX_PI));
		}

		if (x > 0) {
			u = 1.0f - u;
		}

		*(((float*)((BYTE*)pVertex + numVertexBytes * i)) + 6) = u;
		*(((float*)((BYTE*)pVertex + numVertexBytes * i)) + 7) = v;
	}

	//バッファのアンロック
	m_pMesh->UnlockVertexBuffer();

	//シェーダーの読み込み
	if (!m_pEffect) {

#ifdef _DEBUG
		HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_PRIMITIVE_TEXTURE_EFFECT), nullptr, nullptr, D3DXSHADER_DEBUG, nullptr, &m_pEffect, nullptr);
#else
		HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_PRIMITIVE_TEXTURE_EFFECT), nullptr, nullptr, 0, nullptr, &m_pEffect, nullptr);
#endif
		if (FAILED(hr)) {
			throw DxSystemException(DxSystemException::OM_PRIMITIVE_LOAD_RESOURCE_ERROR);
		}
	}

	//頂点宣言
	D3DVERTEXELEMENT9 VertexElement[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
		{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	//頂点宣言の作成
	if (!m_pVertexDeclaration) {
		HRESULT hr = pDevice->CreateVertexDeclaration(VertexElement, &m_pVertexDeclaration);
		if (FAILED(hr)) {
			throw DxSystemException(DxSystemException::OM_PRIMITIVE_DECLARE_ERROR);
		}
	}

	m_type = SPHERE;
}

//=============================================================================
// テクスチャの読み込み
// 引　数：Engine*     ３Ｄエンジンクラスへのポインタ
// 　　　　std::string ファイル名
// 　　　　const DWORD カラーキー
//=============================================================================
void Primitive::LoadTexture(Engine* pEngine, std::string strFilename, const DWORD colorKey)
{
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	DeleteTexture();

	HRESULT hResult = D3DXCreateTextureFromFileEx(pDevice, (LPCSTR)strFilename.c_str(), 0, 0, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, colorKey, nullptr, nullptr, &m_pTexture);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PRIMITIVE_LOAD_TEXTURE_ERROR);
	}
}

//=============================================================================
// ワールド座標変換行列の取得
// 戻り値：ワールド座標変換行列
//=============================================================================
D3DXMATRIX Primitive::GetWorldTransformMatrix() const
{
	return m_matWorld;
}

//=============================================================================
// ワールド座標変換行列の設定
// 引　数：const LPD3DXMATRIX ワールド座標変換行列へのポインタ
//=============================================================================
void Primitive::SetWorldTransform(const LPD3DXMATRIX pMatWorld)
{
	m_matWorld = *pMatWorld;
}

//=============================================================================
// プリミティブの描画
// 引　数：Engine*           ３Ｄエンジンクラスのポインタ
// 　　　　Camera*           カメラクラスのポインタ
// 　　　　Projection*       プロジェクションクラスのポインタ
// 　　　　AmbientLight*     アンビエントライトクラスのポインタ
// 　　　　DirectionalLight* ディレクショナルライトのポインタ
//=============================================================================
void Primitive::Draw(Engine* pEngine, Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight, std::vector<SpotLight>* pSpotLights)
{
	if (!pAmbient) {
		if (m_type != TRIANGLE_XYZ) {
			return;
		}
	}

	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	pDevice->SetVertexDeclaration(m_pVertexDeclaration);

	//アンビエントライトの色を渡す
	if (pAmbient) {
		D3DCOLORVALUE ambientColor = pAmbient->GetColorValue();
		m_pEffect->SetValue("gAmbientColor", &ambientColor, sizeof(D3DCOLORVALUE));
	}

	D3DXMATRIX matView = pCamera->GetViewMatrix();
	D3DXMATRIX matProj = pProj->GetProjectionMatrix();

	D3DXMATRIX matWorld = m_matWorld * matView * matProj;

	//ワールド座標変換行列を渡す
	m_pEffect->SetMatrix("gMatW", &m_matWorld);

	//ワールドビュー射影変換行列を渡す
	m_pEffect->SetMatrix("gMatWVP", &matWorld);

	//マテリアルのディフューズとアンビエントを渡す
	m_pEffect->SetValue("gMaterialDiffuse", &m_material.Diffuse, sizeof(D3DCOLORVALUE));
	m_pEffect->SetValue("gMaterialAmbient", &m_material.Ambient, sizeof(D3DCOLORVALUE));

	UINT numPass;
	m_pEffect->Begin(&numPass, 0);

	//スポットライトに必要な設定の初期化
	D3DXVECTOR3 positions[MAX_SPOT_LIGHTS] = {};
	D3DXVECTOR3 directions[MAX_SPOT_LIGHTS] = {};
	D3DCOLORVALUE colors[MAX_SPOT_LIGHTS] = {};
	float ranges[MAX_SPOT_LIGHTS] = {};
	float outerAngles[MAX_SPOT_LIGHTS] = {}; 
	float innerAngles[MAX_SPOT_LIGHTS] = {}; 
	float falloffs[MAX_SPOT_LIGHTS] = {};    
	float attn0s[MAX_SPOT_LIGHTS] = {};      
	float attn1s[MAX_SPOT_LIGHTS] = {};      
	float attn2s[MAX_SPOT_LIGHTS] = {};      

	//スポットライトがある場合はここで受け取る
	int spotCount = 0;
	if (pSpotLights) {
		spotCount = min((int)pSpotLights->size(), MAX_SPOT_LIGHTS);
		for (int i = 0; i < spotCount; ++i) {
			const D3DLIGHT9& light = (*pSpotLights)[i].GetLight();
			positions[i] = light.Position;
			directions[i] = light.Direction;
			colors[i] = light.Diffuse;
			outerAngles[i] = cosf(light.Phi);  
			innerAngles[i] = cosf(light.Theta);
			falloffs[i] = light.Falloff;
			attn0s[i] = light.Attenuation0;
			attn1s[i] = light.Attenuation1;
			attn2s[i] = light.Attenuation2;
			ranges[i] = light.Range;
		}
	}

	// シェーダーに値を渡す
	//size*今回渡す個数ではなくsize*渡す最大数にしないとバグる
	m_pEffect->SetValue("gSpotLightCount", &spotCount, sizeof(INT));
	m_pEffect->SetValue("gSpotLightPos", positions, sizeof(D3DVECTOR) * MAX_SPOT_LIGHTS);
	m_pEffect->SetValue("gSpotLightDir", directions, sizeof(D3DVECTOR) * MAX_SPOT_LIGHTS);
	m_pEffect->SetValue("gSpotLightColor", colors, sizeof(D3DCOLORVALUE) * MAX_SPOT_LIGHTS);
	m_pEffect->SetValue("gSpotLightOuterAngle", outerAngles, sizeof(FLOAT) * MAX_SPOT_LIGHTS);
	m_pEffect->SetValue("gSpotLightInnerAngle", innerAngles, sizeof(FLOAT) * MAX_SPOT_LIGHTS);
	m_pEffect->SetValue("gSpotLightFalloff", falloffs, sizeof(FLOAT) * MAX_SPOT_LIGHTS);      
	m_pEffect->SetValue("gSpotLightAttn0", attn0s, sizeof(FLOAT) * MAX_SPOT_LIGHTS);          
	m_pEffect->SetValue("gSpotLightAttn1", attn1s, sizeof(FLOAT) * MAX_SPOT_LIGHTS);          
	m_pEffect->SetValue("gSpotLightAttn2", attn2s, sizeof(FLOAT) * MAX_SPOT_LIGHTS);          
	m_pEffect->SetValue("gSpotLightRange", ranges, sizeof(FLOAT) * MAX_SPOT_LIGHTS); 


	switch (m_type) {
	case TRIANGLE_XYZ:
		pDevice->SetRenderState(D3DRS_WRAP0, 0);
		m_pEffect->SetTechnique("PrimitivePosOnlyTec");
		m_pEffect->BeginPass(0);
		break;
	case TRIANGLE_XYZ_NORMAL:
	{
		pDevice->SetRenderState(D3DRS_WRAP0, 0);
		D3DLIGHT9 light = pLight->GetLight();
		m_pEffect->SetValue("gLightColor", &light.Diffuse, sizeof(D3DCOLORVALUE));
		m_pEffect->SetValue("gLightDir", &light.Direction, sizeof(D3DVECTOR));
		m_pEffect->SetTechnique("PrimitiveNormalTec");
		m_pEffect->BeginPass(0);
		break;
	}
	case TRIANGLE_XYZ_NORMAL_UV:
	case RECTANGLE:
	case BOX:
	{
		pDevice->SetRenderState(D3DRS_WRAP0, 0);
		D3DLIGHT9 light = pLight->GetLight();
		m_pEffect->SetValue("gLightColor", &light.Diffuse, sizeof(D3DCOLORVALUE));
		m_pEffect->SetValue("gLightDir", &light.Direction, sizeof(D3DVECTOR));
		m_pEffect->SetTexture("gTexture", m_pTexture);
		m_pEffect->SetTechnique("PrimitiveTextureTec");
		if (m_pTexture) {
			m_pEffect->BeginPass(0);
		}
		else {
			m_pEffect->BeginPass(1);
		}
		break;
	}
	case SPHERE:
	{
		if (m_pTexture) {
			pDevice->SetRenderState(D3DRS_WRAP0, D3DWRAPCOORD_0);
		}
		else {
			pDevice->SetRenderState(D3DRS_WRAP0, 0);
		}
		D3DLIGHT9 light = pLight->GetLight();
		m_pEffect->SetValue("gLightColor", &light.Diffuse, sizeof(D3DCOLORVALUE));
		m_pEffect->SetValue("gLightDir", &light.Direction, sizeof(D3DVECTOR));
		m_pEffect->SetTexture("gTexture", m_pTexture);
		m_pEffect->SetTechnique("PrimitiveTextureTec");
		if (m_pTexture) {
			m_pEffect->BeginPass(0);
		}
		else {
			m_pEffect->BeginPass(1);
		}
		break;
	}
	default:
		assert(0);
		break;
	}

	m_pMesh->DrawSubset(0);

	m_pEffect->EndPass();
	m_pEffect->End();
}
//=============================================================================
//　バウンディングスフィア半径の取得
//　戻り値：バウンディングスフィアの半径
//=============================================================================
float Primitive::GetBoundingShpereRadius()
{
	DWORD numVertices = m_pMesh->GetNumVertices();

	UINT stride = D3DXGetFVFVertexSize(m_pMesh->GetFVF());

	D3DXVECTOR3 center;

	float radius;

	void* pVertex;

	m_pMesh->LockVertexBuffer(0, &pVertex);

	D3DXComputeBoundingSphere((D3DXVECTOR3*)pVertex, numVertices, stride, &center, &radius);

	m_pMesh->UnlockVertexBuffer();

	return radius;
}

//=============================================================================
//　バウンディングボックス最大値の取得
//　戻り値：バウンディングボックス最大値
//=============================================================================
D3DXVECTOR3 Primitive::GetBoundingBoxMax()
{
	DWORD numVertices = m_pMesh->GetNumVertices();

	UINT stride = D3DXGetFVFVertexSize(m_pMesh->GetFVF());

	D3DXVECTOR3 min, max;

	void* pVertex;

	m_pMesh->LockVertexBuffer(0, &pVertex);

	D3DXComputeBoundingBox((D3DXVECTOR3*)pVertex, numVertices, stride, &min, &max);

	m_pMesh->UnlockVertexBuffer();

	return max;
}
	
//=============================================================================
//　バウンディングボックス最小値の取得
//　戻り値：バウンディングボックス最小値
//=============================================================================
D3DXVECTOR3 Primitive::GetBoundingBoxMin()
{
	DWORD numVertices = m_pMesh->GetNumVertices();

	UINT stride = D3DXGetFVFVertexSize(m_pMesh->GetFVF());

	D3DXVECTOR3 min, max;

	void* pVertex;

	m_pMesh->LockVertexBuffer(0, &pVertex);

	D3DXComputeBoundingBox((D3DXVECTOR3*)pVertex, numVertices, stride, &min, &max);

	m_pMesh->UnlockVertexBuffer();

	return min;
}

//=============================================================================
// マテリアルの設定
// 引　数：const D3DMATERIAL9 マテリアル
//=============================================================================
void Primitive::SetMaterial(const D3DMATERIAL9 material)
{
	m_material = material;
}

void Primitive::DrawForDepthPass(Engine * pEngine)
{
	if (!m_pMesh) return; // メッシュがロードされていなければ何もしない

						  // ワールド行列を設定
	pEngine->GetDevice()->SetTransform(D3DTS_WORLD, &m_matWorld);

	// D3DXMESH は自身の描画メソッドを持っているのでそれを利用
	// 深度パスではマテリアルやテクスチャ、ライトは不要
	// D3DXCreateBoxなどで生成されたプリミティブは通常1つのサブセットを持つ
	for (DWORD i = 0; i < 1; ++i) { // サブセット数を1と仮定して修正
		m_pMesh->DrawSubset(i);
	}
}



//*****************************************************************************
// private関数
//*****************************************************************************


//-----------------------------------------------------------------------------
// メッシュの削除
//-----------------------------------------------------------------------------
void Primitive::DeleteMesh()
{
	m_type = NONE;

	DeleteTexture();

	if (m_pMesh) {
		m_pMesh->Release();
		m_pMesh = nullptr;
	}
}

//-----------------------------------------------------------------------------
// テクスチャの削除
//-----------------------------------------------------------------------------
void Primitive::DeleteTexture()
{
	if (m_pTexture) {
		m_pTexture->Release();
		m_pTexture = nullptr;
	}
}

