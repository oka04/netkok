#pragma once

#include <winsock2.h> 
#include <ws2tcpip.h>

#include "..\\..\\GameBase.h"
#include "..\\..\\Scene\\Scene\\Scene.h"
#include "..\\..\\Object\\CharacterBase\\CharacterBase.h"
#include "..\\..\\Object\\Map\\Map.h"
#include "..\\..\\Object\\IceBreath\\IceBreath.h"  // ★★★ IceBreathをインクルード ★★★
#include <fstream>
#include "..\\json.hpp"

class Chaser : public CharacterBase
{
public:
	Chaser();
	~Chaser();

	void Initialize(Engine* pEngine, Map& map, Projection* projection, Camera& camera, DirectionalLight& light);
	void InitializeAtPosition(Engine* pEngine, const D3DXVECTOR3& startPos, Projection* projection, Camera& camera, DirectionalLight& light);
	void Release(Engine* pEngine);

	void Update(Engine* pEngine, Map& map, Camera& camera, DirectionalLight& light, float deltaTime);

	void Draw(Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight);
	void DrawEffects(Camera* pCamera, Projection* pProj);  // ★★★ エフェクト描画用 ★★★
	void DebugPrint(Engine* pEngine);

	SpotLight* GetLights();

	virtual NetPlayerState GetNetState() const override;
	virtual void UpdateFromNetwork(const NetPlayerState& state, DirectionalLight& light, float deltaTime) override;
	void UpdateLight(Engine* pEngine);

	virtual void DrawDepth(Engine* pEngine, const D3DXMATRIX* pMatLightVP) override;

	// シャドウマップ関連
	bool IsShadowMapEnabled() const { return m_bShadowMapEnabled; }
	LPDIRECT3DTEXTURE9 GetShadowTexture() const { return m_pShadowTexture; }
	LPDIRECT3DSURFACE9 GetShadowDepthSurface() const { return m_pShadowDepthSurface; }
	D3DXMATRIX GetLightViewMatrix() const { return m_matLightView; }
	D3DXMATRIX GetLightProjectionMatrix() const { return m_matLightProj; }
	D3DXMATRIX GetLightViewProjectionMatrix() const { return m_matLightView * m_matLightProj; }
	D3DXMATRIX GetScaleBiasMatrix() const { return m_matScaleBias; }

private:
	void LoadParameter() override;
	void CreateShadowMap(Engine* pEngine);
	void ReleaseShadowMap();
	void UpdateLightMatrices();
	void CreateScaleBiasMatrix();

	// ★★★ ブレス攻撃関連 ★★★
	void UpdateBreathAttack(Engine* pEngine);
	bool CanUseBreath() const;

	SpotLight m_spotLight;
	float m_lightFov;
	float m_lightRange;

	// シャドウマップ関連
	bool m_bShadowMapEnabled;
	LPDIRECT3DTEXTURE9 m_pShadowTexture;
	LPDIRECT3DSURFACE9 m_pShadowSurface;
	LPDIRECT3DSURFACE9 m_pShadowDepthSurface;
	D3DXMATRIX m_matLightView;
	D3DXMATRIX m_matLightProj;
	D3DXMATRIX m_matScaleBias;
	static const int SHADOW_MAP_SIZE = 1024;

	// ★★★ ブレス攻撃関連メンバー変数 ★★★
	IceBreath* m_pIceBreath;
	bool m_bBreathActive;
	bool m_bBreathButtonPressed;  // 前フレームでボタンが押されていたか
	DWORD m_lastBreathTime;       // 最後にブレスを使った時間
	float f_breathCooldown;       // ブレスのクールタイム（秒）
	DWORD m_breathDuration;       // ブレスの持続時間（ミリ秒）
};