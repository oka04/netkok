#pragma once

#include "..\\..\\GameBase.h"

#include "..\\..\\Scene\\Scene\\Scene.h"

class CharacterBase
{
public:
	void Initialize(Engine *pEngine, std::string filename);
	void UpdateMatrix();
	void Draw(Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight);

	const D3DXVECTOR3& GetPosition()const;
	const D3DXVECTOR3& GetDirection()const;

protected:
	Model m_model;
	D3DXVECTOR3 m_position;
	D3DXVECTOR3 m_direction;

	float m_angle;

	D3DXMATRIX m_matWorld;
	D3DXMATRIX m_matRotate;
	D3DXMATRIX m_matTrans;
};
