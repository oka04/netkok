//*****************************************************************************
//
// ビルボードクラス
//
// Billboard.cpp
//
// K_Yamaoka
//
// 2013/03/04
//
// 2014/11/28 エラーメッセージを追加
//
// 2020/08/24 シェーダー対応に変更
//
// 2020/08/26 マテリアルを削除
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "Billboard.h"

//=============================================================================
// コンストラクタ
//=============================================================================
Billboard::Billboard() : m_pEffect(nullptr), m_pVertexDeclaration(nullptr)
{

}

//=============================================================================
// デストラクタ
//=============================================================================
Billboard::~Billboard()
{
	DeleteTexture();
	DeleteMesh();

	m_billboardContainer.clear();

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
// ビルボードの追加
// 引　数：Engine*            ３Ｄエンジンクラスへのポインタ
// 　　　　const D3DXVECTOR3* ビルボードの位置へのポインタ
// 　　　　const float        幅
// 　　　　const float        高さ
// 　　　　std::string        テクスチャファイル名
// 　　　　const DWORD        カラーキー（デフォルトは紫）
//=============================================================================
void Billboard::Add(Engine* pEngine, const D3DXVECTOR3* pVecPosition, const float width, const float height, std::string strFilename, const DWORD colorKey)
{
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	//シェーダーの読み込み
	if (!m_pEffect) {

#ifdef _DEBUG
		HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_BILLBOARD_EFFECT), nullptr, nullptr, D3DXSHADER_DEBUG, nullptr, &m_pEffect, nullptr);
#else
		HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_BILLBOARD_EFFECT), nullptr, nullptr, 0, nullptr, &m_pEffect, nullptr);
#endif
		if (FAILED(hr)) {
			throw DxSystemException(DxSystemException::OM_BILLBOARD_LOAD_RESOURCE_ERROR);
		}
	}

	BillboardData workBillboardData;

	workBillboardData.vecPosition = *pVecPosition;

	//モデルデータの重複チェック

	bool bExistModel = false;

	if (!m_modelContainer.empty()) {
		for (DWORD i = 0; i < m_modelContainer.size(); i++) {
			if (m_modelContainer[i].width == width && m_modelContainer[i].height == height) {
				workBillboardData.modelNo = i;
				bExistModel = true;
			}
		}
	}

	//重複データなしの場合、新しくモデルデータを作成

	if (!bExistModel) {

		workBillboardData.modelNo = m_modelContainer.size();

		LPD3DXMESH pMesh = nullptr;

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
				throw DxSystemException(DxSystemException::OM_BILLBOARD_DECLARE_ERROR);
			}
		}

		//メッシュの作成
		HRESULT hResult = D3DXCreateMesh(2, 6, D3DXMESH_MANAGED, VertexElement, pDevice, &pMesh);

		if (FAILED(hResult)) {
			throw DxSystemException(DxSystemException::OM_BILLBOARD_CREATE_MESH_ERROR);
		}

		//頂点データ
		D3DXVECTOR3 vertices[6] = {
			D3DXVECTOR3(-width / 2,  height, 0),
			D3DXVECTOR3(width / 2,  height, 0),
			D3DXVECTOR3(-width / 2,  0     , 0),
			D3DXVECTOR3(width / 2,  height, 0),
			D3DXVECTOR3(width / 2,  0     , 0),
			D3DXVECTOR3(-width / 2,  0     , 0),
		};

		//法線データ
		D3DXVECTOR3 normals = D3DXVECTOR3(0, 0, -1);

		//テクスチャ座標
		Coord coords[36] = {
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
		hResult = pMesh->LockVertexBuffer(0, &pVertex);

		if (FAILED(hResult)) {
			throw DxSystemException(DxSystemException::OM_BILLBOARD_LOCK_VB_ERROR);
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
		pMesh->UnlockVertexBuffer();

		//インデックスバッファへのアクセス

		void* pIndex; //インデックス情報の先頭番地

		//インデックスバッファのロック
		hResult = pMesh->LockIndexBuffer(0, &pIndex);

		if (FAILED(hResult)) {
			throw DxSystemException(DxSystemException::OM_BILLBOARD_LOCK_IB_ERROR);
		}

		//インデックスの設定
		for (int i = 0; i < 6; i++) {
			((WORD*)(pIndex))[i] = (WORD)i;
		}

		//バッファのアンロック
		pMesh->UnlockIndexBuffer();

		//モデルデータの登録
		BillboardModelData workModelData;

		workModelData.width = width;
		workModelData.height = height;
		workModelData.pMesh = pMesh;

		m_modelContainer.push_back(workModelData);
	}

	//テクスチャデータの重複チェック

	bool bExistTexture = false;

	if (!m_textureContainer.empty()) {
		for (DWORD i = 0; i < m_textureContainer.size(); i++) {
			if (m_textureContainer[i].strFilename == strFilename) {
				workBillboardData.textureNo = i;
				bExistTexture = true;
			}
		}
	}

	//重複データなしの場合、新しくテクスチャデータを作成

	if (!bExistTexture) {

		workBillboardData.textureNo = m_textureContainer.size();

		LPDIRECT3DTEXTURE9 pTexture = nullptr;

		HRESULT hResult = D3DXCreateTextureFromFileEx(pDevice, (LPCSTR)strFilename.c_str(), 0, 0, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, colorKey, nullptr, nullptr, &pTexture);

		if (FAILED(hResult)) {
			throw DxSystemException(DxSystemException::OM_BILLBOARD_LOAD_TEXTURE_ERROR);
		}

		//テクスチャデータの登録
		BillboardTextureData workTextureData;

		workTextureData.strFilename = strFilename;
		workTextureData.pTexture = pTexture;

		m_textureContainer.push_back(workTextureData);
	}

	//ビルボードデータの登録

	m_billboardContainer.push_back(workBillboardData);
}

//=============================================================================
// ビルボードの描画
// 引　数：Engine*           ３Ｄエンジンクラスのポインタ
// 　　　　Camera*           カメラクラスのポインタ
// 　　　　Projection*       プロジェクションクラスのポインタ
// 　　　　AmbientLight*     アンビエントライトクラスのポインタ
// 　　　　DirectionalLight* ディレクショナルライトのポインタ
//=============================================================================
void Billboard::Draw(Engine* pEngine, Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight)
{
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	//登録されているすべてのビルボードを描画

	D3DXVECTOR3 vecDistance;

	float distance1, distance2;

	BillboardData work;

	bool bChange;

	DWORD size = m_billboardContainer.size();

	//Ｚソート

	for (DWORD i = 0; i < size - 1; i++) {
		bChange = false;
		for (DWORD j = 0; j < size - i - 1; j++) {
			vecDistance = m_billboardContainer[j].vecPosition - pCamera->m_vecEye;
			distance1 = D3DXVec3Length(&vecDistance);
			vecDistance = m_billboardContainer[j + 1].vecPosition - pCamera->m_vecEye;
			distance2 = D3DXVec3Length(&vecDistance);
			if (distance1 < distance2) {
				work = m_billboardContainer[j];
				m_billboardContainer[j] = m_billboardContainer[j + 1];
				m_billboardContainer[j + 1] = work;
				bChange = true;
			}
		}
		if (!bChange) {
			break;
		}
	}

	pDevice->SetVertexDeclaration(m_pVertexDeclaration);

	//アンビエントライトの色を渡す
	D3DCOLORVALUE ambientColor = pAmbient->GetColorValue();
	m_pEffect->SetValue("gAmbientColor", &ambientColor, sizeof(D3DCOLORVALUE));

	//ディレクショナルライトの色と方向を渡す
	D3DLIGHT9 light = pLight->GetLight();
	m_pEffect->SetValue("gLightColor", &light.Diffuse, sizeof(D3DCOLORVALUE));
	m_pEffect->SetValue("gLightDir", &light.Direction, sizeof(D3DVECTOR));
	
	//回転行列を渡す
	D3DXMATRIX matRotate = pCamera->GetYBillboardMatrix();
	m_pEffect->SetMatrix("gMatRotate", &matRotate);

	D3DXMATRIX matView = pCamera->GetViewMatrix();
	D3DXMATRIX matProj = pProj->GetProjectionMatrix();

	for (DWORD i = 0; i < size; i++) {

		D3DXMATRIX matTrans;
		D3DXMatrixTranslation(&matTrans, m_billboardContainer[i].vecPosition.x, m_billboardContainer[i].vecPosition.y, m_billboardContainer[i].vecPosition.z);
		
		D3DXMATRIX matWorld = matRotate * matTrans * matView * matProj;

		//ワールドビュー射影変換行列を渡す
		m_pEffect->SetMatrix("gMatWVP", &matWorld);

		//テクスチャーを渡す
		m_pEffect->SetTexture("gTexture", m_textureContainer[m_billboardContainer[i].textureNo].pTexture);

		//テクニックの設定
		m_pEffect->SetTechnique("BillboardTec");
		
		//パス０で描画
		UINT numPass;
		m_pEffect->Begin(&numPass, 0);
		m_pEffect->BeginPass(0);

		m_modelContainer[m_billboardContainer[i].modelNo].pMesh->DrawSubset(0);

		m_pEffect->EndPass();
		m_pEffect->End();
	}
}




//*****************************************************************************
// private関数
//*****************************************************************************


//-----------------------------------------------------------------------------
// メッシュの削除
//-----------------------------------------------------------------------------
void Billboard::DeleteMesh()
{
	if (!m_modelContainer.empty()) {

		for (DWORD i = 0; i < m_modelContainer.size(); i++) {
			if (m_modelContainer[i].pMesh) {
				m_modelContainer[i].pMesh->Release();
				m_modelContainer[i].pMesh = nullptr;
			}
		}

		m_modelContainer.clear();
	}
}

//-----------------------------------------------------------------------------
// テクスチャの削除
//-----------------------------------------------------------------------------
void Billboard::DeleteTexture()
{
	if (!m_textureContainer.empty()) {

		for (DWORD i = 0; i < m_textureContainer.size(); i++) {
			if (m_textureContainer[i].pTexture) {
				m_textureContainer[i].pTexture->Release();
				m_textureContainer[i].pTexture = nullptr;
			}
		}

		m_textureContainer.clear();
	}
}


