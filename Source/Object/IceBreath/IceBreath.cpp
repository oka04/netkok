#define _USING_V110_SDK71_ 1

#include "IceBreath.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;

IceBreath::IceBreath()
	: m_pEffect(nullptr)
	, m_pVertexDeclaration(nullptr)
	, m_breathDirection(0.0f, 0.0f, -1.0f)
	, f_iceColorR(0.5f)
	, f_iceColorG(0.7f)
	, f_iceColorB(1.0f)
	, m_bActive(false)
	, m_activateTime(0)
	, m_duration(3000)
{

}

IceBreath::~IceBreath()
{
	if (m_pVertexDeclaration)
	{
		m_pVertexDeclaration->Release();
		m_pVertexDeclaration = nullptr;
	}

	if (m_pEffect)
	{
		m_pEffect->Release();
		m_pEffect = nullptr;
	}
}

// ★★★ ParticleBaseの純粋仮想関数の実装（基本的な初期化のみ） ★★★
void IceBreath::Initialize(Engine* pEngine, const D3DXVECTOR3 position, const DWORD existTime)
{
	m_pEngine = pEngine;
	m_position = position;
	m_generateTime = timeGetTime();
	m_existTime = existTime;
	m_bExist = true;
	m_bActive = false;  // 最初は非アクティブ
	m_duration = existTime;

	LoadParameter();

	ParticleBase::SetSizeAndTexture(m_pEngine, 0.5f, 0.5f, TEXTURE_EFFECT);

	m_imGenerate.SetInterval(100);

	LPDIRECT3DDEVICE9 pDevice = m_pEngine->GetDevice();

	if (!m_pEffect)
	{

#ifdef _DEBUG
		HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_PARTICLE_EFFECT), nullptr, nullptr, D3DXSHADER_DEBUG, nullptr, &m_pEffect, nullptr);
#else
		HRESULT hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_PARTICLE_EFFECT), nullptr, nullptr, 0, nullptr, &m_pEffect, nullptr);
#endif
		if (FAILED(hr)) {
			throw DxSystemException(DxSystemException::OM_PARTICLE_LOAD_RESOURCE_ERROR);
		}
	}

	D3DVERTEXELEMENT9 VertexElement[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};

	if (!m_pVertexDeclaration)
	{
		HRESULT hr = pDevice->CreateVertexDeclaration(VertexElement, &m_pVertexDeclaration);
		if (FAILED(hr))
		{
			throw DxSystemException(DxSystemException::OM_PARTICLE_DECLARE_ERROR);
		}
	}
}

// ★★★ エフェクトを再起動（表示開始） ★★★
void IceBreath::Activate(const D3DXVECTOR3& position, const D3DXVECTOR3& direction)
{
	m_position = position;
	m_breathDirection = direction;

	// 向きを正規化
	D3DXVec3Normalize(&m_breathDirection, &m_breathDirection);

	m_bActive = true;
	m_activateTime = timeGetTime();
	m_generateTime = m_activateTime;
	m_bExist = true;

	// 既存のパーティクルをクリア
	m_lstParticle.clear();
}

// ★★★ エフェクトを停止（非表示） ★★★
void IceBreath::Deactivate()
{
	m_bActive = false;
	m_lstParticle.clear();

}

void IceBreath::Update()
{
	// ★★★ 非アクティブなら何もしない ★★★
	if (!m_bActive)
	{
		return;
	}

	// ★★★ 持続時間チェック ★★★
	DWORD now = timeGetTime();
	if (now - m_activateTime >= m_duration)
	{
		Deactivate();
		return;
	}

	// パーティクル生成
	if (m_imGenerate.GetTiming())
	{
		for (int i = 0; i < n_generateCount; i++)
		{
			// ★★★ 青っぽい氷の色を設定 ★★★
			D3DCOLORVALUE color = { f_iceColorR, f_iceColorG, f_iceColorB, f_minAlpha };

			// パーティクル生成位置のランダムなズレ（口元付近）
			float randomX = (rand() / (float)RAND_MAX) * (f_positionRandomRange * 2.0f) - f_positionRandomRange;
			float randomY = (rand() / (float)RAND_MAX) * (f_positionRandomRange * 2.0f) - f_positionRandomRange;
			float randomZ = (rand() / (float)RAND_MAX) * (f_positionRandomRange * 2.0f) - f_positionRandomRange;
			D3DXVECTOR3 positionOffset(randomX, randomY, randomZ);

			// ブレスの方向ベクトルを計算（コーン状に広がる）
			D3DXVECTOR3 baseDirection = m_breathDirection * f_forwardSpeed;

			// 横方向と上方向のランダムな広がりを追加（コーン状）
			D3DXVECTOR3 rightVec;
			D3DXVECTOR3 upVec(0.0f, 1.0f, 0.0f);

			// 右方向ベクトルを計算
			D3DXVec3Cross(&rightVec, &upVec, &m_breathDirection);
			D3DXVec3Normalize(&rightVec, &rightVec);

			// 上方向ベクトルを再計算（正確な上方向）
			D3DXVec3Cross(&upVec, &m_breathDirection, &rightVec);
			D3DXVec3Normalize(&upVec, &upVec);

			// コーン状に広がる角度をランダムに生成
			float coneRandomAngle = (rand() / (float)RAND_MAX) * f_coneAngle - (f_coneAngle * 0.5f);
			float coneRandomAngle2 = (rand() / (float)RAND_MAX) * f_coneAngle - (f_coneAngle * 0.5f);

			// 方向に広がりを追加
			D3DXVECTOR3 direction = baseDirection
				+ rightVec * coneRandomAngle * f_directionRandomScale
				+ upVec * coneRandomAngle2 * f_directionRandomScale;

			D3DXVec3Normalize(&direction, &direction);

			Add(positionOffset, color, n_particleExistTime, n_particleFadeTime, 0.0f, false, direction);
		}
	}

	// パーティクルの更新（移動と重力効果）
	for (auto& particle : m_lstParticle)
	{
		// 前方への移動
		particle.m_position += particle.m_direction * f_moveSpeed;

		// 重力効果（徐々に下に落ちる）
		particle.m_position.y -= f_gravity;

		// フェードイン効果
		if (particle.m_color.a < f_maxAlpha)
		{
			particle.m_color.a += f_fadeInSpeed;
			if (particle.m_color.a > f_maxAlpha)
			{
				particle.m_color.a = f_maxAlpha;
			}
		}
	}

	ParticleBase::Update();
}

void IceBreath::Draw(Camera* pCamera, Projection* pProj)
{
	// ★★★ 非アクティブなら描画しない ★★★
	if (!m_bActive)
	{
		return;
	}

	LPDIRECT3DDEVICE9 pDevice = m_pEngine->GetDevice();

	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	pDevice->SetRenderState(D3DRS_WRAP0, 0);

	pDevice->SetVertexDeclaration(m_pVertexDeclaration);

	D3DXMATRIX matRotate = pCamera->GetBillboardMatrix();

	D3DXMATRIX matView = pCamera->GetViewMatrix();
	D3DXMATRIX matProj = pProj->GetProjectionMatrix();
	D3DXMATRIX matVP = matView * matProj;

	m_pEffect->SetTechnique("ParticleTec");

	ParticleBase::Draw(pDevice, m_pEffect, &matVP, &matRotate);

	pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}

void IceBreath::LoadParameter()
{
	// ★★★ JSONファイル名を氷のブレス専用のものに変更 ★★★
	// KeyString.hに "JSON_ICE_BREATH_PARAMETER" の定義が必要
	std::ifstream file(JSON_ICE_BREATH_PARAMETER);
	if (!file.is_open())
	{
		throw DxSystemException(DxSystemException::OM_FILE_OPEN_ERROR);
	}

	nlohmann::json config;
	file >> config;
	file.close();

	// ★★★ 氷の色をJSONから読み込み ★★★
	f_iceColorR = config["iceColorR"];
	f_iceColorG = config["iceColorG"];
	f_iceColorB = config["iceColorB"];

	n_generateCount = config["generateCount"];
	f_positionRandomRange = config["positionRandomRange"];
	f_directionRandomScale = config["directionRandomScale"];
	f_forwardSpeed = config["forwardSpeed"];
	f_moveSpeed = config["moveSpeed"];
	f_fadeInSpeed = config["fadeInSpeed"];
	f_minAlpha = config["minAlpha"];
	f_maxAlpha = config["maxAlpha"];
	n_particleExistTime = config["particleExistTime"];
	n_particleFadeTime = config["particleFadeTime"];
	f_coneAngle = config["coneAngle"];
	f_gravity = config["gravity"];
}