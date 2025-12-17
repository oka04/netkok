// Chaser.h - プレイヤー固有の実装 + ブレス攻撃機能
#pragma once

#include <winsock2.h> 
#include <ws2tcpip.h>

#include "..\\..\\GameBase.h"
#include "..\\..\\Scene\\Scene\\Scene.h"
#include "..\\..\\Object\\CharacterBase\\CharacterBase.h"
#include "..\\..\\Object\\Map\\Map.h"
#include "..\\..\\Object\\IceBreath\\IceBreath.h"
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
	void DebugPrint(Engine* pEngine);

	SpotLight* GetLights();

	virtual NetPlayerState GetNetState() const override;
	virtual void UpdateFromNetwork(const NetPlayerState& state, DirectionalLight& light, float deltaTime) override;
	void UpdateLight(Engine* pEngine);

	virtual void DrawEffects(Camera* pCamera, Projection* pProj) override;
	virtual void DrawDepth(Engine* pEngine, const D3DXMATRIX* pMatLightVP) override;

	D3DXVECTOR3 GetCenterPosition() const override
	{
		D3DXVECTOR3 centerPos = m_position;
		centerPos.y += f_height / 2.0f; 
		return centerPos;
	}
	// ★★★ ブレス中かどうかを取得 ★★★
	bool IsBreathing() const { return m_bBreathActive; }

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
	bool m_bBreathButtonPressed;
	DWORD m_lastBreathTime;
	float f_breathCooldown;
	DWORD m_breathDuration;
};