//*****************************************************************************
//
// パーティクルの基底クラス
//
// ParticleBase.cpp
//
// K_Yamaoka
//
// 2020/09/10
//
// 2020/11/12 Add関数に方向を追加（デフォルト付）
//
// 2020/11/13 Add関数にフェードアウトする時間を追加
//            SetSizeAndTexture関数でEngineクラスのポインタを保管
//
// 2020/11/17 ToFinite関数を追加
//
// 2020/11/19 ChangePosition､ChangeDirection関数を追加
//
// 2020/12/08 Add関数に角度と横置きフラグを追加
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "ParticleBase.h"

//存在時間（永遠）
const DWORD ParticleBase::INIFINITY = 0xFFFFFFFF;

//=============================================================================
// コンストラクタ
//=============================================================================
ParticleBase::ParticleBase()
	: m_pTexture(nullptr)
	, m_bPrepareDelete(false)
	, m_bExist(true)
	, m_pEngine(nullptr)
{

}

//=============================================================================
// デストラクタ
//=============================================================================
ParticleBase::~ParticleBase()
{
	DeleteTexture();

	m_lstParticle.clear();
}

//=============================================================================
// サイズとテクスチャーの設定
// 引　数：Engine*     エンジンクラスのアドレス
// 　　　　const float パーティクルの幅
// 　　　　const float パーティクルの高さ
// 　　　　std::string テクスチャーファイル名
//=============================================================================
void ParticleBase::SetSizeAndTexture(Engine* pEngine, const float width, const float height, std::string strFilename)
{
	assert(pEngine);

	m_pEngine = pEngine;

	SetRectangle(width, height);

	LoadTexture(pEngine, strFilename);
}

//=============================================================================
// 更新
//=============================================================================
void ParticleBase::Update()
{
	ExistCheck();
}

//=============================================================================
// パーティクルの描画
// 引　数：Engine*      ３Ｄエンジンクラスのポインタ
// 　　　　ID3DXEffect* エフェクトのポインタ
// 　　　　D3DXMATRIX*  ビュープロジェクション変換行列のポインタ
// 　　　　D3DXMATRIX*  回転行列（ビルボード）のポインタ
//=============================================================================
void ParticleBase::Draw(LPDIRECT3DDEVICE9 pDevice, ID3DXEffect* pEffect, D3DXMATRIX* pMatVP, D3DXMATRIX* pMatBillboard)
{
	//テクスチャーを渡す
	pEffect->SetTexture("gTexture", m_pTexture);

	for (auto& particle : m_lstParticle) {

		//色情報を渡す
		pEffect->SetValue("gColor", &particle.m_color, sizeof(D3DCOLORVALUE));

		D3DXMATRIX matRotateZ;
		D3DXMatrixRotationZ(&matRotateZ, particle.m_angle);

		D3DXMATRIX matLocalTrans;
		D3DXMatrixTranslation(&matLocalTrans, &particle.m_position);

		D3DXMATRIX matWorldTrans;
		D3DXMatrixTranslation(&matWorldTrans, &m_position);

		D3DXMATRIX matWorld;

		if (!particle.m_bHorizontal) {
			matWorld = matRotateZ * *pMatBillboard * matLocalTrans * matWorldTrans * *pMatVP;
		}
		else {
			D3DXMATRIX matRotateX;
			D3DXMatrixRotationX(&matRotateX, D3DXToRadian(90.0f));
			matWorld = matRotateZ * matRotateX * matLocalTrans * matWorldTrans * *pMatVP;
		}

		//ワールドビュー射影変換行列を渡す
		pEffect->SetMatrix("gMatWVP", &matWorld);

		//パス０で描画
		UINT numPass;
		pEffect->Begin(&numPass, 0);
		pEffect->BeginPass(0);

		pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, m_vertices, sizeof(PARTICLE_VERTEX));

		pEffect->EndPass();
		pEffect->End();
	}
}

//=============================================================================
// パーティクルの追加
// 引　数：const D3DXVECTOR3   追加する位置
// 　　　　const D3DCOLORVALUE 色
// 　　　　const DWORD         存在する時間（ミリ秒）
// 　　　　const DWORD         フェードアウトする時間（ミリ秒）
// 　　　　const float         回転角度
// 　　　　const bool          横置きフラグ（デフォルトはfalse）
// 　　　　const D3DXVECTOR3　 方向（デフォルトはゼロベクトル）
//=============================================================================
void ParticleBase::Add(const D3DXVECTOR3 position, const D3DCOLORVALUE color, const DWORD existTime, const DWORD fadeOutTime, const float angle, const bool bHorizontal, const D3DXVECTOR3 direction)
{
	if (!m_bPrepareDelete && m_bExist) {
	
		Particle work;

		work.m_position = position;
		work.m_color = color;
		work.m_generateTime = timeGetTime();
		work.m_fadeOutTime = fadeOutTime;
		work.m_existTime = existTime;
		work.m_bFadeOut = false;
		work.m_bExist = true;
		work.m_angle = angle;
		work.m_bHorizontal = bHorizontal;
		work.m_direction = direction;

		m_lstParticle.push_back(work);
	}
}

//=============================================================================
// 削除する予定の時間を取得
//=============================================================================
DWORD ParticleBase::GetDeleteTime() const
{
	return m_generateTime + m_existTime;
}

//=============================================================================
// パーティクルの削除
//=============================================================================
void ParticleBase::DeleteParticles()
{
	if (!m_lstParticle.empty()) {
		m_bPrepareDelete = true;
	}
	else {
		m_bExist = false;
	}
}

//=============================================================================
// 存在チェック
//=============================================================================
bool ParticleBase::IsExist() const
{
	return m_bExist;
}

//=============================================================================
// 永遠に存在し続けるかどうかチェック
//=============================================================================
bool ParticleBase::IsInfinity() const
{
	return m_existTime == INIFINITY;
}

//=============================================================================
// 永遠→有限に変更
// 引　数：const DWORD 削除するまでの時間（ミリ秒）
//=============================================================================
void ParticleBase::ToFinite(const DWORD deleteTime)
{
	if (m_existTime == ParticleBase::INIFINITY) {
		m_generateTime = timeGetTime();
		m_existTime = deleteTime;
	}
}

//=============================================================================
// 位置を変更
// 引　数：const D3DXVECTOR3 新しい位置ベクトル
//=============================================================================
void ParticleBase::ChangePosition(const D3DXVECTOR3 position)
{
	m_position = position;
}

//=============================================================================
// 方向を変更
// 引　数：const D3DXVECTOR3 新しい方向ベクトル
//=============================================================================
void ParticleBase::ChangeDirection(const D3DXVECTOR3 direction)
{
	m_direction = direction;
}


//*****************************************************************************
// private関数
//*****************************************************************************

//-----------------------------------------------------------------------------
// 板ポリゴンの設定
// 引　数：const float パーティクルの幅
// 　　　　const float パーティクルの高さ
//-----------------------------------------------------------------------------
void ParticleBase::SetRectangle(const float width, const float height)
{
	m_vertices[0].vertex = D3DXVECTOR3(-width * 0.5f, height * 0.5f, 0);
	m_vertices[1].vertex = D3DXVECTOR3(width * 0.5f, height * 0.5f, 0);
	m_vertices[2].vertex = D3DXVECTOR3(-width * 0.5f, -height * 0.5f, 0);
	m_vertices[3].vertex = D3DXVECTOR3(width * 0.5f, -height * 0.5f, 0);

	m_vertices[0].u = 0; m_vertices[0].v = 0;
	m_vertices[1].u = 1; m_vertices[1].v = 0;
	m_vertices[2].u = 0; m_vertices[2].v = 1;
	m_vertices[3].u = 1; m_vertices[3].v = 1;
}

//-----------------------------------------------------------------------------
// テクスチャの読み込み
// 引　数：Engine*     ３Ｄエンジンクラスへのポインタ
// 　　　　std::string ファイル名
//-----------------------------------------------------------------------------
void ParticleBase::LoadTexture(Engine* pEngine, std::string strFilename)
{
	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	DeleteTexture();

	HRESULT hResult = D3DXCreateTextureFromFileEx(pDevice, (LPCSTR)strFilename.c_str(), 0, 0, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0x00000000, nullptr, nullptr, &m_pTexture);

	if (FAILED(hResult)) {
		throw DxSystemException(DxSystemException::OM_PARTICLE_LOAD_TEXTURE_ERROR);
	}
}

//-----------------------------------------------------------------------------
// テクスチャの削除
//-----------------------------------------------------------------------------
void ParticleBase::DeleteTexture()
{
	if (m_pTexture) {
		m_pTexture->Release();
		m_pTexture = nullptr;
	}
}


//-----------------------------------------------------------------------------
// 存在チェック
// 備　考：存在時間を経過したパーティクルはリストから削除される
//-----------------------------------------------------------------------------
void ParticleBase::ExistCheck()
{
	DWORD nowTime = timeGetTime();

	for (auto& particle : m_lstParticle) {

		if (!particle.m_bFadeOut) {
			if (nowTime >= particle.m_generateTime + particle.m_existTime) {
				particle.m_fadeValue = particle.m_color.a / 60.0f / (particle.m_fadeOutTime / 1000.0f);
				particle.m_bFadeOut = true;
			}
		}
		else {
			particle.m_color.a -= particle.m_fadeValue;
			if (particle.m_color.a < 0.0f) {
				particle.m_color.a = 0.0f;
				particle.m_bExist = false;
			}
		}
	}

	std::list<Particle>::iterator it;

	for (it = m_lstParticle.begin(); it != m_lstParticle.end(); ) {
		if (!it->m_bExist) {
			it = m_lstParticle.erase(it);
		}
		else {
			it++;
		}
	}
}
