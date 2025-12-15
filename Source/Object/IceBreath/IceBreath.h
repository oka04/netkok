// IceBreath.h - 氷の息吹エフェクト
#pragma once

#include "..\\..\\GameBase.h"

#include "..\\..\\System\\Engine\\Particle\\ParticleBase.h"
#include "..\\..\\Scene\\Scene\\Scene.h"
#include <fstream>
#include "..\\json.hpp"

class IceBreath : public ParticleBase
{
public:
	IceBreath();
	~IceBreath();

	virtual void Initialize(Engine* pEngine, const D3DXVECTOR3 position, const DWORD existTime) override;

	void Activate(const D3DXVECTOR3& position, const D3DXVECTOR3& direction);
	void Deactivate();
	bool IsActive() const { return m_bActive; }

	void SetPosition(const D3DXVECTOR3& position) { m_position = position; }
	void SetDirection(const D3DXVECTOR3& direction) { m_breathDirection = direction; }

	// ★★★ ライトの範囲に合わせる ★★★
	void SetMaxDistance(float distance) { m_maxDistance = distance; }

	void Update();
	void Draw(Camera* pCamera, Projection* pProj);

	void LoadParameter();

private:
	IntervalManage m_imGenerate;
	ID3DXEffect* m_pEffect;
	LPDIRECT3DVERTEXDECLARATION9 m_pVertexDeclaration;

	D3DXVECTOR3 m_breathDirection;

	bool m_bActive;
	DWORD m_activateTime;
	DWORD m_duration;

	// ★★★ ライトの範囲制限用 ★★★
	float m_maxDistance;

	float f_iceColorR;
	float f_iceColorG;
	float f_iceColorB;

	int n_generateCount;
	float f_positionRandomRange;
	float f_directionRandomScale;
	float f_forwardSpeed;
	float f_moveSpeed;
	float f_fadeInSpeed;
	float f_minAlpha;
	float f_maxAlpha;
	int n_particleExistTime;
	int n_particleFadeTime;
	float f_coneAngle;
	float f_gravity;
};