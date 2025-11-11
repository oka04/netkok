#define _USING_V110_SDK71_ 1

#include "CharacterBase.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;

//初期化
void CharacterBase::Initialize(Engine *pEngine, std::string filename)
{
	m_model.SetModel(pEngine->GetModel(filename));
}

//更新
void CharacterBase::UpdateMatrix()
{
	D3DXMatrixRotationY(&m_matRotate, m_angle);

	D3DXMatrixTranslation(&m_matTrans, &m_position);

	m_matWorld = m_matRotate * m_matTrans;
	
	m_model.SetWorldTransform(&m_matWorld);
}

//描画
void CharacterBase::Draw(Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight)
{
	m_model.Draw(pCamera, pProj, pAmbient, pLight);
}

const D3DXVECTOR3& CharacterBase::GetPosition()const
{
	return m_position;
}

const D3DXVECTOR3& CharacterBase::GetDirection() const
{
	return m_direction;
}

