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
	, f_iceColorR(0.3f)
	, f_iceColorG(0.5f)
	, f_iceColorB(0.9f)
	, m_bActive(false)
	, m_activateTime(0)
	, m_duration(5000)
	, m_maxDistance(15.0f)
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

void IceBreath::Initialize(Engine* pEngine, const D3DXVECTOR3 position, const DWORD existTime)
{
	m_pEngine = pEngine;
	m_position = position;
	m_generateTime = timeGetTime();
	m_existTime = existTime;
	m_bExist = true;
	m_bActive = false;
	m_duration = existTime;

	LoadParameter();

	ParticleBase::SetSizeAndTexture(m_pEngine, 0.3f, 0.3f, TEXTURE_EFFECT);

	m_imGenerate.SetInterval(110);

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

void IceBreath::Activate(const D3DXVECTOR3& position, const D3DXVECTOR3& direction)
{
	m_position = position;
	m_breathDirection = direction;

	D3DXVec3Normalize(&m_breathDirection, &m_breathDirection);

	m_bActive = true;
	m_activateTime = timeGetTime();
	m_generateTime = m_activateTime;
	m_bExist = true;

	m_lstParticle.clear();

	NET_LOG_F("[IceBreath::Activate] Pos=(%.1f,%.1f,%.1f) Dir=(%.2f,%.2f,%.2f) MaxDist=%.2f",
		m_position.x, m_position.y, m_position.z,
		m_breathDirection.x, m_breathDirection.y, m_breathDirection.z,
		m_maxDistance);
}

void IceBreath::Deactivate()
{
	m_bActive = false;
	m_lstParticle.clear();

}

void IceBreath::Update()
{
	if (!m_bActive)
	{
		return;
	}

	DWORD now = timeGetTime();
	if (now - m_activateTime >= m_duration)
	{
		NET_LOG("[IceBreath::Update] 持続時間終了");
		Deactivate();
		return;
	}

	if (m_imGenerate.GetTiming())
	{
		for (int i = 0; i < n_generateCount; i++)
		{
			float initialAlpha = f_maxAlpha * 0.5f;
			D3DCOLORVALUE color = { f_iceColorR, f_iceColorG, f_iceColorB, initialAlpha };

			float randomX = (rand() / (float)RAND_MAX) * (f_positionRandomRange * 2.0f) - f_positionRandomRange;
			float randomY = (rand() / (float)RAND_MAX) * (f_positionRandomRange * 2.0f) - f_positionRandomRange;
			float randomZ = (rand() / (float)RAND_MAX) * (f_positionRandomRange * 2.0f) - f_positionRandomRange;
			D3DXVECTOR3 positionOffset(randomX, randomY, randomZ);

			D3DXVECTOR3 baseDirection = m_breathDirection * f_forwardSpeed;

			baseDirection.y -= 0.05f;  //下方向に少し傾ける

			D3DXVECTOR3 rightVec;
			D3DXVECTOR3 upVec(0.0f, 1.0f, 0.0f);

			D3DXVec3Cross(&rightVec, &upVec, &m_breathDirection);
			D3DXVec3Normalize(&rightVec, &rightVec);

			D3DXVec3Cross(&upVec, &m_breathDirection, &rightVec);
			D3DXVec3Normalize(&upVec, &upVec);

			float coneRandomAngle = (rand() / (float)RAND_MAX) * f_coneAngle - (f_coneAngle * 0.5f);
			float coneRandomAngle2 = (rand() / (float)RAND_MAX) * f_coneAngle - (f_coneAngle * 0.5f);

			D3DXVECTOR3 direction = baseDirection
				+ rightVec * coneRandomAngle * f_directionRandomScale
				+ upVec * coneRandomAngle2 * f_directionRandomScale;

			D3DXVec3Normalize(&direction, &direction);

			Add(positionOffset, color, n_particleExistTime, n_particleFadeTime, 0.0f, false, direction);
		}

		static DWORD lastLog = 0;
		if (now - lastLog > 1000)
		{
			NET_LOG_F("[IceBreath::Update] パーティクル生成: 総数=%d", (int)m_lstParticle.size());
			lastLog = now;
		}
	}

	for (auto& particle : m_lstParticle)
	{
		particle.m_position += particle.m_direction * f_moveSpeed;

		particle.m_position.y -= f_gravity;

		if (particle.m_color.a < f_maxAlpha)
		{
			particle.m_color.a += f_fadeInSpeed;
			if (particle.m_color.a > f_maxAlpha)
			{
				particle.m_color.a = f_maxAlpha;
			}
		}

		D3DXVECTOR3 diff = particle.m_position - m_position;
		float distance = D3DXVec3Length(&diff);

		if (distance > m_maxDistance * 1.5f)
		{
			particle.m_color.a -= 0.015f;
			if (particle.m_color.a < 0.0f)
			{
				particle.m_color.a = 0.0f;
			}
		}
	}

	ParticleBase::Update();
}

void IceBreath::Draw(Camera* pCamera, Projection* pProj)
{
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
	std::ifstream file(JSON_ICE_BREATH_PARAMETER);
	if (!file.is_open())
	{
		throw DxSystemException(DxSystemException::OM_FILE_OPEN_ERROR);
	}

	nlohmann::json config;
	file >> config;
	file.close();

	f_iceColorR = config.value("iceColorR", 0.3f); 
	f_iceColorG = config.value("iceColorG", 0.5f); 
	f_iceColorB = config.value("iceColorB", 0.9f); 

	n_generateCount = config.value("generateCount", 10);
	f_positionRandomRange = config.value("positionRandomRange", 0.3f);
	f_directionRandomScale = config.value("directionRandomScale", 0.2f);
	f_forwardSpeed = config.value("forwardSpeed", 0.5f);
	f_moveSpeed = config.value("moveSpeed", 0.08f);
	f_fadeInSpeed = config.value("fadeInSpeed", 0.03f);
	f_minAlpha = config.value("minAlpha", 0.6f);
	f_maxAlpha = config.value("maxAlpha", 0.9f); 
	n_particleExistTime = config.value("particleExistTime", 5000);
	n_particleFadeTime = config.value("particleFadeTime", 1000);
	f_coneAngle = config.value("coneAngle", 30.0f);
	f_gravity = config.value("gravity", 0.002f);
}