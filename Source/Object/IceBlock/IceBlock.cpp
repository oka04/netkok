// IceBlock.cpp - 溶ける氷ブロッククラス実装
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
	, m_pEffect(nullptr)
	, m_pVertexDeclaration(nullptr)
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

	// ボックスプリミティブを作成
	m_primitive.CreateBox(pEngine, width, height, depth);

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
	m_primitive.SetMaterial(material);

	// シェーダー作成
	CreateShaderEffect(pEngine);

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

	// 溶け具合に応じてスケールを調整
	D3DXVECTOR3 currentScale = m_scale;

	// 下から溶ける効果
	float meltScale = 1.0f - m_meltAmount;
	currentScale.y *= meltScale;

	// 若干全体的にも縮小
	float shrinkFactor = 1.0f - (m_meltAmount * 0.3f);
	currentScale.x *= shrinkFactor;
	currentScale.z *= shrinkFactor;

	// 溶けた分だけ下に移動
	D3DXVECTOR3 adjustedPosition = m_position;
	adjustedPosition.y -= (m_originalSize.y * m_scale.y * m_meltAmount * 0.5f);

	// ワールド行列の更新
	D3DXMATRIX matScale, matRotX, matRotY, matRotZ, matTrans;
	D3DXMatrixScaling(&matScale, currentScale.x, currentScale.y, currentScale.z);
	D3DXMatrixRotationX(&matRotX, m_rotation.x);
	D3DXMatrixRotationY(&matRotY, m_rotation.y);
	D3DXMatrixRotationZ(&matRotZ, m_rotation.z);
	D3DXMatrixTranslation(&matTrans, adjustedPosition.x, adjustedPosition.y, adjustedPosition.z);

	D3DXMATRIX matWorld = matScale * matRotX * matRotY * matRotZ * matTrans;
	m_primitive.SetWorldTransform(&matWorld);

	// 溶け具合に応じてアルファ値を調整
	float alpha = m_iceColor.w * (1.0f - m_meltAmount * 0.5f);

	D3DMATERIAL9 material;
	ZeroMemory(&material, sizeof(D3DMATERIAL9));
	material.Diffuse.r = m_iceColor.x;
	material.Diffuse.g = m_iceColor.y;
	material.Diffuse.b = m_iceColor.z;
	material.Diffuse.a = alpha;
	material.Ambient = material.Diffuse;
	material.Specular.r = 1.0f;
	material.Specular.g = 1.0f;
	material.Specular.b = 1.0f;
	material.Specular.a = 1.0f;
	material.Power = 32.0f;
	m_primitive.SetMaterial(material);

	// 描画
	m_primitive.Draw(pEngine, pCamera, pProj, pAmbient, pLight);

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

void IceBlock::CreateShaderEffect(Engine* pEngine)
{
	// Primitiveクラスが既にシェーダーを持っているので、ここでは特別な処理は不要
	// 必要に応じてカスタムシェーダーを読み込むこともできます
}

void IceBlock::ReleaseResources()
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