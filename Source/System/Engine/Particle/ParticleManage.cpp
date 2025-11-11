//*****************************************************************************
//
// パーティクル管理クラス
//
// ParticleManage.cpp
//
// K_Yamaoka
//
// 2020/09/10
//
// 2020/11/17 Draw関数のビルボード用行列の取得を修正
//            DeleteInfiniyParticle関数を追加
//
// 2020/11/19 ChangePosition,ChangeDirection関数を追加
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "ParticleManage.h"

//=============================================================================
// コンストラクタ
//=============================================================================
ParticleManage::ParticleManage()
	: m_pEffect(nullptr)
	, m_pVertexDeclaration(nullptr)
{

}

//=============================================================================
// デストラクタ
//=============================================================================
ParticleManage::~ParticleManage()
{
	for (auto& pParticles : m_lstParticleBase) {
		delete pParticles;
		pParticles = nullptr;
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
// 引　数：Engine* エンジンクラスのアドレス
// 備　考：シーンのStart関数で呼び出す
//=============================================================================
void ParticleManage::Initialize(Engine* pEngine)
{
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	//シェーダーの読み込み
	if (!m_pEffect) {

#ifdef _DEBUG
		HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_PARTICLE_EFFECT), nullptr, nullptr, D3DXSHADER_DEBUG, nullptr, &m_pEffect, nullptr);
#else
		HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_PARTICLE_EFFECT), nullptr, nullptr, 0, nullptr, &m_pEffect, nullptr);
#endif
		if (FAILED(hr)) {
			throw DxSystemException(DxSystemException::OM_PARTICLE_LOAD_RESOURCE_ERROR);
		}
	}

	//頂点宣言
	D3DVERTEXELEMENT9 VertexElement[] = {
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
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
// 更新
// 備　考：シーンのUpdate関数で呼び出す
//=============================================================================
void ParticleManage::Update()
{
	for (auto& pParticles : m_lstParticleBase) {
		pParticles->Update();
	}

	ExistCheck();
}

//=============================================================================
// 描画
// 引　数：Engine*     エンジンクラスのアドレス
// 　　　　Camera*     カメラクラスのアドレス
// 　　　　Projection* プロジェクションクラスのアドレス
// 備　考：シーンのDraw関数で呼び出す
//=============================================================================
void ParticleManage::Draw(Engine* pEngine, Camera* pCamera, Projection* pProj)
{
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

	pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	pDevice->SetRenderState(D3DRS_WRAP0, 0);

	pDevice->SetVertexDeclaration(m_pVertexDeclaration);

	//ビルボード用回転行列を取得
	D3DXMATRIX matRotate = pCamera->GetBillboardMatrix();

	D3DXMATRIX matView = pCamera->GetViewMatrix();
	D3DXMATRIX matProj = pProj->GetProjectionMatrix();
	D3DXMATRIX matVP = matView * matProj;

	//テクニックの設定
	m_pEffect->SetTechnique("ParticleTec");
	
	//描画
	for (auto& pParticles : m_lstParticleBase) {
		pParticles->Draw(pDevice, m_pEffect, &matVP, &matRotate);
	}
	
	pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);

	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}

//=============================================================================
// パーティクル生成
// 引　数：Engine*           エンジンクラスのアドレス
// 　　　　ParticleBase*     パーティクルクラスのアドレス
// 　　　　const D3DXVECTOR3 位置
// 　　　　const DWORD       存在時間
//=============================================================================
void ParticleManage::Generate(Engine* pEngine, ParticleBase* pParticle, const D3DXVECTOR3 position, const DWORD existTime)
{
	pParticle->Initialize(pEngine, position, existTime);

	m_lstParticleBase.push_back(pParticle);

	pParticle = nullptr;
}

//=============================================================================
// 永遠に存在するパーティクルを消す
// 引　数：const ParticleBase* パーティクルベースクラスのアドレス
// 　　　　const DWORD         消すまでの時間（ミリ秒）
//=============================================================================
void ParticleManage::DeleteInfiniyParticle(const ParticleBase* pTargetAddress, const DWORD deleteTime)
{
	for (ParticleBase* pAddress : m_lstParticleBase) {
		if (pAddress == pTargetAddress) {
			pAddress->ToFinite(deleteTime);
			pAddress->DeleteParticles();
		}
	}
}

//=============================================================================
// 位置を変更する
// 引　数：const ParticleBase* パーティクルベースクラスのアドレス
// 　　　　const D3DXVECTOR3   新しい位置
//=============================================================================
void ParticleManage::ChangePosition(ParticleBase* pTargetAddress, const D3DXVECTOR3 position)
{
	for (ParticleBase* pAddress : m_lstParticleBase) {
		if (pAddress == pTargetAddress) {
			pAddress->ChangePosition(position);
		}
	}
}

//=============================================================================
// 方向を変更する
// 引　数：const ParticleBase* パーティクルベースクラスのアドレス
// 　　　　const D3DXVECTOR3   新しい方向
//=============================================================================
void ParticleManage::ChangeDirection(ParticleBase* pTargetAddress, const D3DXVECTOR3 direction)
{
	for (ParticleBase* pAddress : m_lstParticleBase) {
		if (pAddress == pTargetAddress) {
			pAddress->ChangeDirection(direction);
		}
	}
}

//-----------------------------------------------------------------------------
// 存在チェック
// 備　考：存在時間を経過したパーティクルはリストから削除される
//-----------------------------------------------------------------------------
void ParticleManage::ExistCheck()
{
	DWORD nowTime = timeGetTime();

	for (auto& pParticles : m_lstParticleBase) {
		if (!pParticles->IsInfinity()) {
			if (nowTime >= pParticles->GetDeleteTime()) {
				pParticles->DeleteParticles();
			}
		}
	}

	std::list<ParticleBase*>::iterator it;

	for (it = m_lstParticleBase.begin(); it != m_lstParticleBase.end(); ) {
		if (!(*it)->IsExist()) {
			delete *it;
			*it = nullptr;
			it = m_lstParticleBase.erase(it);
		}
		else {
			it++;
		}
	}
}