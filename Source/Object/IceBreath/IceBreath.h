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

	// ★★★ ParticleBaseの純粋仮想関数をオーバーライド ★★★
	virtual void Initialize(Engine* pEngine, const D3DXVECTOR3 position, const DWORD existTime) override;

	// ★★★ エフェクト再利用のための新メソッド ★★★
	void Activate(const D3DXVECTOR3& position, const D3DXVECTOR3& direction);
	void Deactivate();
	bool IsActive() const { return m_bActive; }

	void Update();
	void Draw(Camera* pCamera, Projection* pProj);

	void LoadParameter();

private:
	IntervalManage m_imGenerate;
	ID3DXEffect* m_pEffect;
	LPDIRECT3DVERTEXDECLARATION9 m_pVertexDeclaration;

	// キャラクターの向き（ブレスを吐く方向）
	D3DXVECTOR3 m_breathDirection;

	// ★★★ エフェクトの状態管理 ★★★
	bool m_bActive;              // エフェクトが発動中か
	DWORD m_activateTime;        // 発動開始時刻
	DWORD m_duration;            // 持続時間（ミリ秒）

								 // 氷の色パラメータ
	float f_iceColorR;
	float f_iceColorG;
	float f_iceColorB;

	int n_generateCount;
	float f_positionRandomRange;
	float f_directionRandomScale;
	float f_forwardSpeed;        // 前方への速度
	float f_moveSpeed;
	float f_fadeInSpeed;
	float f_minAlpha;
	float f_maxAlpha;
	int n_particleExistTime;
	int n_particleFadeTime;
	float f_coneAngle;           // ブレスの広がり角度
	float f_gravity;             // 重力効果
};