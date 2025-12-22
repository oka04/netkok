// IceBlock.cpp - パラメータファイル対応版

#define _USING_V110_SDK71_ 1

#include "IceBlock.h"
using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;


IceBlock::IceBlock()
	: m_position(0.0f, 0.0f, 0.0f)
	, m_rotation(0.0f, 0.0f, 0.0f)
	, m_scale(1.0f, 1.0f, 1.0f)
	, m_originalSize(1.0f, 1.0f, 1.0f)
	, m_meltAmount(0.0f)
	, m_meltDuration(0.0f)
	, m_elapsedTime(0.0f)
	, m_iceColor(0.8f, 0.95f, 1.0f, 0.75f)
	, m_autoMelt(false)
	, f_iceColorR(0.8f)
	, f_iceColorG(0.95f)
	, f_iceColorB(1.0f)
	, f_iceAlpha(0.75f)
	, f_ambientMultiplier(1.3f)
	, f_diffuseMultiplier(1.0f)
	, f_specularPower(32.0f)
	, f_minScale(0.3f)
	, f_throughWallAlphaMultiplier(1.5f)
	, f_throughWallDiffuseMultiplier(1.3f)
	, f_throughWallAmbientMultiplier(1.5f)
	, f_alphaFadeRate(0.4f)
{
	D3DXMatrixIdentity(&m_matWorld);
}

IceBlock::~IceBlock()
{
	ReleaseResources();
}

void IceBlock::LoadParameter()
{
	std::ifstream file(JSON_ICE_BLOCK_PARAMETER);
	if (!file.is_open())
	{
		throw DxSystemException(DxSystemException::OM_FILE_OPEN_ERROR);
	}

	nlohmann::json config;
	file >> config;
	file.close();

	f_iceColorR = config.value("iceColorR", 0.8f);
	f_iceColorG = config.value("iceColorG", 0.95f);
	f_iceColorB = config.value("iceColorB", 1.0f);
	f_iceAlpha = config.value("iceAlpha", 0.75f);
	f_ambientMultiplier = config.value("ambientMultiplier", 1.3f);
	f_diffuseMultiplier = config.value("diffuseMultiplier", 1.0f);
	f_specularPower = config.value("specularPower", 32.0f);
	f_minScale = config.value("minScale", 0.3f);
	f_throughWallAlphaMultiplier = config.value("throughWallAlphaMultiplier", 1.5f);
	f_throughWallDiffuseMultiplier = config.value("throughWallDiffuseMultiplier", 1.3f);
	f_throughWallAmbientMultiplier = config.value("throughWallAmbientMultiplier", 1.5f);
	f_alphaFadeRate = config.value("alphaFadeRate", 0.4f);

	m_iceColor = D3DXVECTOR4(f_iceColorR, f_iceColorG, f_iceColorB, f_iceAlpha);
}

void IceBlock::Initialize(Engine* pEngine, float width, float height, float depth,
	const D3DXVECTOR3& position, float meltDuration)
{
	LoadParameter(); 

	m_position = position;
	m_originalSize = D3DXVECTOR3(width, height, depth);
	m_meltDuration = meltDuration;
	m_autoMelt = (meltDuration > 0.0f);
	m_elapsedTime = 0.0f;
	m_meltAmount = 0.0f;

	float radius = (width + height + depth) / 6.0f;
	m_primitiveSphere.CreateSphere(pEngine, radius, 24);

	D3DMATERIAL9 material;
	ZeroMemory(&material, sizeof(D3DMATERIAL9));
	material.Diffuse.r = m_iceColor.x * f_diffuseMultiplier;
	material.Diffuse.g = m_iceColor.y * f_diffuseMultiplier;
	material.Diffuse.b = m_iceColor.z * f_diffuseMultiplier;
	material.Diffuse.a = m_iceColor.w;
	material.Ambient = material.Diffuse;
	material.Ambient.r *= f_ambientMultiplier;
	material.Ambient.g *= f_ambientMultiplier;
	material.Ambient.b *= f_ambientMultiplier;
	material.Specular.r = 1.0f;
	material.Specular.g = 1.0f;
	material.Specular.b = 1.0f;
	material.Specular.a = 1.0f;
	material.Power = f_specularPower;
	m_primitiveSphere.SetMaterial(material);

	UpdateWorldMatrix();
}

void IceBlock::Update(float deltaTime)
{
	if (m_autoMelt && m_meltAmount < 1.0f)
	{
		m_elapsedTime += deltaTime;
		m_meltAmount = min(1.0f, m_elapsedTime / m_meltDuration);
	}
}

void IceBlock::Draw(Engine* pEngine, Camera* pCamera, Projection* pProj,
	AmbientLight* pAmbient, DirectionalLight* pLight)
{
	if (m_meltAmount >= 1.0f)
		return;

	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	float shrinkAmount = 1.0f - (1.0f - f_minScale) * m_meltAmount;

	D3DXVECTOR3 currentScale = m_scale;
	currentScale.x *= shrinkAmount;
	currentScale.y *= shrinkAmount;
	currentScale.z *= shrinkAmount;

	D3DXVECTOR3 adjustedPosition = m_position;

	float currentAlpha = m_iceColor.w * (1.0f - m_meltAmount * f_alphaFadeRate);

	D3DXMATRIX matScale, matRotX, matRotY, matRotZ, matTrans;
	D3DXMatrixScaling(&matScale, currentScale.x, currentScale.y, currentScale.z);
	D3DXMatrixRotationX(&matRotX, m_rotation.x);
	D3DXMatrixRotationY(&matRotY, m_rotation.y);
	D3DXMatrixRotationZ(&matRotZ, m_rotation.z);
	D3DXMatrixTranslation(&matTrans, adjustedPosition.x, adjustedPosition.y, adjustedPosition.z);

	D3DXMATRIX matWorld = matScale * matRotX * matRotY * matRotZ * matTrans;

	D3DMATERIAL9 material;
	ZeroMemory(&material, sizeof(D3DMATERIAL9));
	material.Diffuse.r = m_iceColor.x * f_diffuseMultiplier;
	material.Diffuse.g = m_iceColor.y * f_diffuseMultiplier;
	material.Diffuse.b = m_iceColor.z * f_diffuseMultiplier;
	material.Diffuse.a = currentAlpha;
	material.Ambient.r = m_iceColor.x * f_ambientMultiplier;
	material.Ambient.g = m_iceColor.y * f_ambientMultiplier;
	material.Ambient.b = m_iceColor.z * f_ambientMultiplier;
	material.Ambient.a = currentAlpha;
	material.Specular.r = 1.0f;
	material.Specular.g = 1.0f;
	material.Specular.b = 1.0f;
	material.Specular.a = 1.0f;
	material.Power = f_specularPower;
	m_primitiveSphere.SetMaterial(material);

	m_primitiveSphere.SetWorldTransform(&matWorld);
	m_primitiveSphere.Draw(pEngine, pCamera, pProj, pAmbient, pLight);

	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
}

void IceBlock::DrawThroughWalls(Engine* pEngine, Camera* pCamera, Projection* pProj,
	AmbientLight* pAmbient, DirectionalLight* pLight, float alpha)
{
	if (m_meltAmount >= 1.0f)
		return;

	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	DWORD oldZEnable, oldZWriteEnable, oldAlphaBlend, oldSrcBlend, oldDestBlend, oldCullMode;
	pDevice->GetRenderState(D3DRS_ZENABLE, &oldZEnable);
	pDevice->GetRenderState(D3DRS_ZWRITEENABLE, &oldZWriteEnable);
	pDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &oldAlphaBlend);
	pDevice->GetRenderState(D3DRS_SRCBLEND, &oldSrcBlend);
	pDevice->GetRenderState(D3DRS_DESTBLEND, &oldDestBlend);
	pDevice->GetRenderState(D3DRS_CULLMODE, &oldCullMode);

	pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	// ★★★ 溶け具合を反映したスケールを計算 ★★★
	float shrinkAmount = 1.0f - (1.0f - f_minScale) * m_meltAmount;

	D3DXVECTOR3 currentScale = m_scale;
	currentScale.x *= shrinkAmount;
	currentScale.y *= shrinkAmount;
	currentScale.z *= shrinkAmount;

	D3DXVECTOR3 adjustedPosition = m_position;

	// ★★★ 溶け具合を反映したアルファ値を計算 ★★★
	float currentAlpha = m_iceColor.w * alpha * f_throughWallAlphaMultiplier * (1.0f - m_meltAmount * f_alphaFadeRate);

	D3DXMATRIX matScale, matRotX, matRotY, matRotZ, matTrans;
	D3DXMatrixScaling(&matScale, currentScale.x, currentScale.y, currentScale.z);
	D3DXMatrixRotationX(&matRotX, m_rotation.x);
	D3DXMatrixRotationY(&matRotY, m_rotation.y);
	D3DXMatrixRotationZ(&matRotZ, m_rotation.z);
	D3DXMatrixTranslation(&matTrans, adjustedPosition.x, adjustedPosition.y, adjustedPosition.z);

	D3DXMATRIX matWorld = matScale * matRotX * matRotY * matRotZ * matTrans;

	D3DMATERIAL9 material;
	ZeroMemory(&material, sizeof(D3DMATERIAL9));
	material.Diffuse.r = m_iceColor.x * f_throughWallDiffuseMultiplier;
	material.Diffuse.g = m_iceColor.y * f_throughWallDiffuseMultiplier;
	material.Diffuse.b = m_iceColor.z * f_throughWallDiffuseMultiplier;
	material.Diffuse.a = currentAlpha;
	material.Ambient.r = m_iceColor.x * f_throughWallAmbientMultiplier;
	material.Ambient.g = m_iceColor.y * f_throughWallAmbientMultiplier;
	material.Ambient.b = m_iceColor.z * f_throughWallAmbientMultiplier;
	material.Ambient.a = currentAlpha;
	material.Specular.r = 1.0f;
	material.Specular.g = 1.0f;
	material.Specular.b = 1.0f;
	material.Specular.a = 1.0f;
	material.Power = f_specularPower;
	m_primitiveSphere.SetMaterial(material);

	m_primitiveSphere.SetWorldTransform(&matWorld);
	m_primitiveSphere.Draw(pEngine, pCamera, pProj, pAmbient, pLight);

	pDevice->SetRenderState(D3DRS_ZENABLE, oldZEnable);
	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, oldZWriteEnable);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, oldAlphaBlend);
	pDevice->SetRenderState(D3DRS_SRCBLEND, oldSrcBlend);
	pDevice->SetRenderState(D3DRS_DESTBLEND, oldDestBlend);
	pDevice->SetRenderState(D3DRS_CULLMODE, oldCullMode);
}

void IceBlock::DrawThroughWallsFullSize(Engine* pEngine, Camera* pCamera, Projection* pProj,
	AmbientLight* pAmbient, DirectionalLight* pLight, float alpha)
{
	if (m_meltAmount >= 1.0f)
		return;

	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	// レンダーステートの保存
	DWORD oldZEnable, oldZWriteEnable, oldAlphaBlend, oldSrcBlend, oldDestBlend, oldCullMode;
	pDevice->GetRenderState(D3DRS_ZENABLE, &oldZEnable);
	pDevice->GetRenderState(D3DRS_ZWRITEENABLE, &oldZWriteEnable);
	pDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &oldAlphaBlend);
	pDevice->GetRenderState(D3DRS_SRCBLEND, &oldSrcBlend);
	pDevice->GetRenderState(D3DRS_DESTBLEND, &oldDestBlend);
	pDevice->GetRenderState(D3DRS_CULLMODE, &oldCullMode);

	// 壁貫通設定
	pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	// ★★★ 常に最大サイズで描画（溶け具合を反映しない）★★★
	D3DXVECTOR3 maxScale = m_scale;
	D3DXVECTOR3 adjustedPosition = m_position;

	// ★★★ アルファ値は通常通り（溶け具合は反映しない）★★★
	float currentAlpha = m_iceColor.w * alpha * f_throughWallAlphaMultiplier;

	// ワールド行列の計算
	D3DXMATRIX matScale, matRotX, matRotY, matRotZ, matTrans;
	D3DXMatrixScaling(&matScale, maxScale.x, maxScale.y, maxScale.z);
	D3DXMatrixRotationX(&matRotX, m_rotation.x);
	D3DXMatrixRotationY(&matRotY, m_rotation.y);
	D3DXMatrixRotationZ(&matRotZ, m_rotation.z);
	D3DXMatrixTranslation(&matTrans, adjustedPosition.x, adjustedPosition.y, adjustedPosition.z);

	D3DXMATRIX matWorld = matScale * matRotX * matRotY * matRotZ * matTrans;

	// マテリアル設定
	D3DMATERIAL9 material;
	ZeroMemory(&material, sizeof(D3DMATERIAL9));
	material.Diffuse.r = m_iceColor.x * f_throughWallDiffuseMultiplier;
	material.Diffuse.g = m_iceColor.y * f_throughWallDiffuseMultiplier;
	material.Diffuse.b = m_iceColor.z * f_throughWallDiffuseMultiplier;
	material.Diffuse.a = currentAlpha;
	material.Ambient.r = m_iceColor.x * f_throughWallAmbientMultiplier;
	material.Ambient.g = m_iceColor.y * f_throughWallAmbientMultiplier;
	material.Ambient.b = m_iceColor.z * f_throughWallAmbientMultiplier;
	material.Ambient.a = currentAlpha;
	material.Specular.r = 1.0f;
	material.Specular.g = 1.0f;
	material.Specular.b = 1.0f;
	material.Specular.a = 1.0f;
	material.Power = f_specularPower;
	m_primitiveSphere.SetMaterial(material);

	// 描画
	m_primitiveSphere.SetWorldTransform(&matWorld);
	m_primitiveSphere.Draw(pEngine, pCamera, pProj, pAmbient, pLight);

	// レンダーステートの復元
	pDevice->SetRenderState(D3DRS_ZENABLE, oldZEnable);
	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, oldZWriteEnable);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, oldAlphaBlend);
	pDevice->SetRenderState(D3DRS_SRCBLEND, oldSrcBlend);
	pDevice->SetRenderState(D3DRS_DESTBLEND, oldDestBlend);
	pDevice->SetRenderState(D3DRS_CULLMODE, oldCullMode);
}


void IceBlock::SetMeltAmount(float amount)
{
	m_meltAmount = max(0.0f, min(1.0f, amount));
}

void IceBlock::SetPosition(const D3DXVECTOR3& position)
{
	m_position = position;
	UpdateWorldMatrix();
}

void IceBlock::SetRotation(float x, float y, float z)
{
	m_rotation = D3DXVECTOR3(x, y, z);
	UpdateWorldMatrix();
}

void IceBlock::SetScale(float x, float y, float z)
{
	m_scale = D3DXVECTOR3(x, y, z);
	UpdateWorldMatrix();
}

void IceBlock::UpdateWorldMatrix()
{
	D3DXMATRIX matScale, matRotX, matRotY, matRotZ, matTrans;
	D3DXMatrixScaling(&matScale, m_scale.x, m_scale.y, m_scale.z);
	D3DXMatrixRotationX(&matRotX, m_rotation.x);
	D3DXMatrixRotationY(&matRotY, m_rotation.y);
	D3DXMatrixRotationZ(&matRotZ, m_rotation.z);
	D3DXMatrixTranslation(&matTrans, m_position.x, m_position.y, m_position.z);

	m_matWorld = matScale * matRotX * matRotY * matRotZ * matTrans;
}

void IceBlock::ReleaseResources()
{
}