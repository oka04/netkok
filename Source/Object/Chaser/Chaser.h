#pragma once

#include <winsock2.h> 
#include <ws2tcpip.h>

#include "..\\..\\GameBase.h"
#include "..\\..\\Scene\\Scene\\Scene.h"
#include "..\\..\\Object\\CharacterBase\\CharacterBase.h"
#include "..\\..\\Object\\Map\\Map.h"
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

	// ★★★ 深度パス用の描画を追加 ★★★
	virtual void DrawDepth(Engine* pEngine) override;

	// ★★★ シャドウマップ関連のメソッドを追加 ★★★
	bool IsShadowMapEnabled() const { return m_bShadowMapEnabled; }
	LPDIRECT3DTEXTURE9 GetShadowTexture() const { return m_pShadowTexture; }
	LPDIRECT3DSURFACE9 GetShadowDepthSurface() const { return m_pShadowDepthSurface; }
	D3DXMATRIX GetLightViewMatrix() const { return m_matLightView; }
	D3DXMATRIX GetLightProjectionMatrix() const { return m_matLightProj; }
	D3DXMATRIX GetLightViewProjectionMatrix() const { return m_matLightView * m_matLightProj; }

private:
	void LoadParameter() override;

	// ★★★ シャドウマップ関連のメソッド ★★★
	void CreateShadowMap(Engine* pEngine);
	void ReleaseShadowMap();
	void UpdateLightMatrices();

	SpotLight m_spotLight;
	float m_lightFov;
	float m_lightRange;

	// ★★★ シャドウマップ関連のメンバー変数 ★★★
	bool m_bShadowMapEnabled;
	LPDIRECT3DTEXTURE9 m_pShadowTexture;
	LPDIRECT3DSURFACE9 m_pShadowSurface;
	LPDIRECT3DSURFACE9 m_pShadowDepthSurface;
	D3DXMATRIX m_matLightView;
	D3DXMATRIX m_matLightProj;
	static const int SHADOW_MAP_SIZE = 512;
};