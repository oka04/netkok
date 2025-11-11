#define _USING_V110_SDK71_ 1

#include "GoalEffect.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;

GoalEffect::GoalEffect()
	: m_pEffect(nullptr)
	, m_pVertexDeclaration(nullptr)
{

}

GoalEffect::~GoalEffect()
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

void GoalEffect::Initialize(Engine* pEngine, const D3DXVECTOR3 position, const DWORD existTime)
{
	m_pEngine = pEngine;
	m_position = position;
	m_generateTime = timeGetTime();
	m_existTime = existTime;
	m_bExist = true;

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

void GoalEffect::Update()
{
	if (m_imGenerate.GetTiming()) 
	{
		for (int i = 0; i < n_generateCount; i++) 
		{
			D3DCOLORVALUE color = { 1.0f, 1.0f, 1.0f, f_minAlpha };

			//パーティクルを生成する位置のランダムなズレ
			float randomX = (rand() / (float)RAND_MAX) * (f_positionRandomRange * 2.0f) - f_positionRandomRange;
			float randomZ = (rand() / (float)RAND_MAX) * (f_positionRandomRange * 2.0f) - f_positionRandomRange;
			D3DXVECTOR3 positionOffset(randomX, 0.0f, randomZ);

			//上昇する方向のベクトル（XZ平面にランダムなズレを加える）
			D3DXVECTOR3 direction(randomX * f_directionRandomScale, f_upwardSpeed, randomZ * f_directionRandomScale);
			D3DXVec3Normalize(&direction, &direction);

			Add(positionOffset, color, n_particleExistTime, n_particleFadeTime, 0.0f, false, direction);
		}
	}

	for (auto& particle : m_lstParticle) 
	{
		particle.m_position += particle.m_direction * f_moveSpeed;

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

void GoalEffect::Draw(Camera* pCamera, Projection* pProj)
{
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

void GoalEffect::LoadParameter()
{
	std::ifstream file(JSON_GOAL_EFFECT_PARAMETER);
	if (!file.is_open()) 
	{
		throw DxSystemException(DxSystemException::OM_FILE_OPEN_ERROR);
	}

	nlohmann::json config;
	file >> config;
	file.close();

	n_generateCount = config["generateCount"];
	f_positionRandomRange = config["positionRandomRange"];
	f_directionRandomScale = config["directionRandomScale"];
	f_upwardSpeed = config["upwardSpeed"];
	f_moveSpeed = config["moveSpeed"];
	f_fadeInSpeed = config["fadeInSpeed"];
	f_minAlpha = config["minAlpha"];
	f_maxAlpha = config["maxAlpha"];
	n_particleExistTime = config["particleExistTime"];
	n_particleFadeTime = config["particleFadeTime"];
}