#pragma once

#include "..\\..\\GameBase.h"

#include "..\\..\\System\\Engine\\Particle\\ParticleBase.h"
#include "..\\..\\Scene\\Scene\\Scene.h"
#include <fstream>
#include "..\\json.hpp"

class GoalEffect : public ParticleBase
{
public:
	GoalEffect();
	~GoalEffect();

	void Initialize(Engine* pEngine, const D3DXVECTOR3 position, const DWORD existTime);
	void Update();
	void Draw(Camera* pCamera, Projection* pProj);

	void LoadParameter();

private:
	IntervalManage m_imGenerate;
	ID3DXEffect* m_pEffect;
	LPDIRECT3DVERTEXDECLARATION9 m_pVertexDeclaration;

	int n_generateCount;
	float f_positionRandomRange;
	float f_directionRandomScale;
	float f_upwardSpeed;
	float f_moveSpeed;
	float f_fadeInSpeed;
	float f_minAlpha;
	float f_maxAlpha;
	int n_particleExistTime;
	int n_particleFadeTime;
};