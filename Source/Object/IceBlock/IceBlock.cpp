// IceBlock.cpp - 溶ける氷ブロッククラス実装（球体版・線形縮小・明るい色）
#define _USING_V110_SDK71_ 1

#include "IceBlock.h"
#include "../../Effect/resource.h"

IceBlock::IceBlock()
	: m_position(0.0f, 0.0f, 0.0f)
	, m_rotation(0.0f, 0.0f, 0.0f)
	, m_scale(1.0f, 1.0f, 1.0f)
	, m_originalSize(1.0f, 1.0f, 1.0f)
	, m_meltAmount(0.0f)
	, m_meltDuration(0.0f)
	, m_elapsedTime(0.0f)
	, m_iceColor(0.8f, 0.95f, 1.0f, 0.75f)  // ★★★ 明るい青白色に変更 ★★★
	, m_autoMelt(false)
{
	D3DXMatrixIdentity(&m_matWorld);
}

IceBlock::~IceBlock()
{
	ReleaseResources();
}

void IceBlock::Initialize(Engine* pEngine, float width, float height, float depth,
	const D3DXVECTOR3& position, float meltDuration)
{
	m_position = position;
	m_originalSize = D3DXVECTOR3(width, height, depth);
	m_meltDuration = meltDuration;
	m_autoMelt = (meltDuration > 0.0f);
	m_elapsedTime = 0.0f;
	m_meltAmount = 0.0f;

	// 球体プリミティブを作成
	float radius = (width + height + depth) / 6.0f;
	m_primitiveSphere.CreateSphere(pEngine, radius, 24);

	// マテリアル設定（明るめに）
	D3DMATERIAL9 material;
	ZeroMemory(&material, sizeof(D3DMATERIAL9));
	material.Diffuse.r = m_iceColor.x;
	material.Diffuse.g = m_iceColor.y;
	material.Diffuse.b = m_iceColor.z;
	material.Diffuse.a = m_iceColor.w;
	material.Ambient = material.Diffuse;
	material.Ambient.r *= 1.2f;  // ★★★ アンビエントを明るく ★★★
	material.Ambient.g *= 1.2f;
	material.Ambient.b *= 1.2f;
	material.Specular.r = 1.0f;
	material.Specular.g = 1.0f;
	material.Specular.b = 1.0f;
	material.Specular.a = 1.0f;
	material.Power = 32.0f;
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

	// 半透明描画設定
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	// ★★★ 線形にサイズを縮小（0% = 100%サイズ, 100% = 30%サイズ） ★★★
	const float MIN_SCALE = 0.3f;
	float shrinkAmount = 1.0f - (1.0f - MIN_SCALE) * m_meltAmount;

	D3DXVECTOR3 currentScale = m_scale;
	currentScale.x *= shrinkAmount;
	currentScale.y *= shrinkAmount;
	currentScale.z *= shrinkAmount;

	// Y位置はプレイヤーの中心に固定（動かさない）
	D3DXVECTOR3 adjustedPosition = m_position;

	// ★★★ アルファ値の調整（徐々に透明に） ★★★
	float currentAlpha = m_iceColor.w * (1.0f - m_meltAmount * 0.4f);

	// ワールド行列の構築
	D3DXMATRIX matScale, matRotX, matRotY, matRotZ, matTrans;
	D3DXMatrixScaling(&matScale, currentScale.x, currentScale.y, currentScale.z);
	D3DXMatrixRotationX(&matRotX, m_rotation.x);
	D3DXMatrixRotationY(&matRotY, m_rotation.y);
	D3DXMatrixRotationZ(&matRotZ, m_rotation.z);
	D3DXMatrixTranslation(&matTrans, adjustedPosition.x, adjustedPosition.y, adjustedPosition.z);

	D3DXMATRIX matWorld = matScale * matRotX * matRotY * matRotZ * matTrans;

	// マテリアル更新（明るめに）
	D3DMATERIAL9 material;
	ZeroMemory(&material, sizeof(D3DMATERIAL9));
	material.Diffuse.r = m_iceColor.x;
	material.Diffuse.g = m_iceColor.y;
	material.Diffuse.b = m_iceColor.z;
	material.Diffuse.a = currentAlpha;
	material.Ambient.r = m_iceColor.x * 1.3f;  // ★★★ アンビエントを明るく ★★★
	material.Ambient.g = m_iceColor.y * 1.3f;
	material.Ambient.b = m_iceColor.z * 1.3f;
	material.Ambient.a = currentAlpha;
	material.Specular.r = 1.0f;
	material.Specular.g = 1.0f;
	material.Specular.b = 1.0f;
	material.Specular.a = 1.0f;
	material.Power = 32.0f;
	m_primitiveSphere.SetMaterial(material);

	// 球体の描画
	m_primitiveSphere.SetWorldTransform(&matWorld);
	m_primitiveSphere.Draw(pEngine, pCamera, pProj, pAmbient, pLight);

	// レンダーステートを戻す
	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
}

// ★★★ 壁貫通描画（常に最大サイズ、Z-test無効、明るい色） ★★★
void IceBlock::DrawThroughWalls(Engine* pEngine, Camera* pCamera, Projection* pProj,
	AmbientLight* pAmbient, DirectionalLight* pLight, float alpha)
{
	if (m_meltAmount >= 1.0f)
		return;

	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	// レンダーステートを保存
	DWORD oldZEnable, oldZWriteEnable, oldAlphaBlend, oldSrcBlend, oldDestBlend, oldCullMode;
	pDevice->GetRenderState(D3DRS_ZENABLE, &oldZEnable);
	pDevice->GetRenderState(D3DRS_ZWRITEENABLE, &oldZWriteEnable);
	pDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &oldAlphaBlend);
	pDevice->GetRenderState(D3DRS_SRCBLEND, &oldSrcBlend);
	pDevice->GetRenderState(D3DRS_DESTBLEND, &oldDestBlend);
	pDevice->GetRenderState(D3DRS_CULLMODE, &oldCullMode);

	// 壁貫通設定（Z-test無効、Z-write無効）
	pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	// 常に最大サイズで描画（溶け具合は無視）
	D3DXVECTOR3 maxScale = m_scale;

	D3DXVECTOR3 adjustedPosition = m_position;

	// ★★★ 透明度を調整（壁越しは薄く表示、さらに明るく） ★★★
	float currentAlpha = m_iceColor.w * alpha * 1.5f;  // ★★★ 1.5倍明るく ★★★

													   // ワールド行列の構築（最大サイズ）
	D3DXMATRIX matScale, matRotX, matRotY, matRotZ, matTrans;
	D3DXMatrixScaling(&matScale, maxScale.x, maxScale.y, maxScale.z);
	D3DXMatrixRotationX(&matRotX, m_rotation.x);
	D3DXMatrixRotationY(&matRotY, m_rotation.y);
	D3DXMatrixRotationZ(&matRotZ, m_rotation.z);
	D3DXMatrixTranslation(&matTrans, adjustedPosition.x, adjustedPosition.y, adjustedPosition.z);

	D3DXMATRIX matWorld = matScale * matRotX * matRotY * matRotZ * matTrans;

	// マテリアル更新（明るめに）
	D3DMATERIAL9 material;
	ZeroMemory(&material, sizeof(D3DMATERIAL9));
	material.Diffuse.r = m_iceColor.x * 1.3f;  // ★★★ ディフューズを明るく ★★★
	material.Diffuse.g = m_iceColor.y * 1.3f;
	material.Diffuse.b = m_iceColor.z * 1.3f;
	material.Diffuse.a = currentAlpha;
	material.Ambient.r = m_iceColor.x * 1.5f;  // ★★★ アンビエントをさらに明るく ★★★
	material.Ambient.g = m_iceColor.y * 1.5f;
	material.Ambient.b = m_iceColor.z * 1.5f;
	material.Ambient.a = currentAlpha;
	material.Specular.r = 1.0f;
	material.Specular.g = 1.0f;
	material.Specular.b = 1.0f;
	material.Specular.a = 1.0f;
	material.Power = 32.0f;
	m_primitiveSphere.SetMaterial(material);

	// 球体の描画
	m_primitiveSphere.SetWorldTransform(&matWorld);
	m_primitiveSphere.Draw(pEngine, pCamera, pProj, pAmbient, pLight);

	// レンダーステートを元に戻す
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
	// Primitiveのデストラクタが自動的にリソースを解放
}