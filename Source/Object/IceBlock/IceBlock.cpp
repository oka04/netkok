// IceBlock.cpp - 溶ける氷ブロッククラス実装（球体版）
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
	, m_iceColor(0.7f, 0.9f, 1.0f, 0.6f)  // 青白い半透明
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

	// ★★★ 球体プリミティブを作成（元のサイズに基づく） ★★★
	// 初期サイズは元のブロックサイズと同じくらいの直径
	float radius = (width + height + depth) / 6.0f;  // 平均の半径
	m_primitiveSphere.CreateSphere(pEngine, radius, 24);  // 滑らかな球体

														  // マテリアル設定
	D3DMATERIAL9 material;
	ZeroMemory(&material, sizeof(D3DMATERIAL9));
	material.Diffuse.r = m_iceColor.x;
	material.Diffuse.g = m_iceColor.y;
	material.Diffuse.b = m_iceColor.z;
	material.Diffuse.a = m_iceColor.w;
	material.Ambient = material.Diffuse;
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
		return;  // 完全に溶けたら描画しない

	LPDIRECT3DDEVICE9 pDevice = pEngine->GetDevice();

	// 半透明描画設定
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

	// カリング設定（両面描画）
	pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	// ★★★ 溶け具合に応じたスケール計算 ★★★
	D3DXVECTOR3 currentScale = m_scale;

	// 全体的な縮小率（1.0 → 0.0に向かって縮む）
	// イージング関数を使って自然な縮み方に
	float shrinkAmount = 1.0f - m_meltAmount;  // 1.0 → 0.0
	float easeOut = shrinkAmount * shrinkAmount;  // 二次関数で徐々に縮む

	currentScale.x *= easeOut;
	currentScale.y *= easeOut;
	currentScale.z *= easeOut;

	// ★★★ Y位置はプレイヤーの中心に固定（動かさない） ★★★
	D3DXVECTOR3 adjustedPosition = m_position;

	// プレイヤーの中心位置をそのまま使用
	// Y位置の調整なし（プレイヤーの中心で溶けていく）

	// ★★★ アルファ値の調整（徐々に透明に） ★★★
	float currentAlpha = m_iceColor.w * (1.0f - m_meltAmount * 0.5f);  // 完全には透明にならない

																	   // ワールド行列の構築
	D3DXMATRIX matScale, matRotX, matRotY, matRotZ, matTrans;
	D3DXMatrixScaling(&matScale, currentScale.x, currentScale.y, currentScale.z);
	D3DXMatrixRotationX(&matRotX, m_rotation.x);
	D3DXMatrixRotationY(&matRotY, m_rotation.y);
	D3DXMatrixRotationZ(&matRotZ, m_rotation.z);
	D3DXMatrixTranslation(&matTrans, adjustedPosition.x, adjustedPosition.y, adjustedPosition.z);

	D3DXMATRIX matWorld = matScale * matRotX * matRotY * matRotZ * matTrans;

	// マテリアル更新
	D3DMATERIAL9 material;
	ZeroMemory(&material, sizeof(D3DMATERIAL9));
	material.Diffuse.r = m_iceColor.x;
	material.Diffuse.g = m_iceColor.y;
	material.Diffuse.b = m_iceColor.z;
	material.Diffuse.a = currentAlpha;
	material.Ambient = material.Diffuse;
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