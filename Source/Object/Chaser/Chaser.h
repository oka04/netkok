// Chaser.h - スポットライト用シャドウマップ対応
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

	// ★★★ シャドウマップ用の新機能 ★★★

	// シャドウマップの初期化
	void InitializeShadowMap(Engine* pEngine, int shadowMapSize = 512);

	// シャドウマップの解放
	void ReleaseShadowMap();

	// シャドウマップ用のビュー・プロジェクション行列を取得
	D3DXMATRIX GetLightViewMatrix() const;
	D3DXMATRIX GetLightProjectionMatrix() const;
	D3DXMATRIX GetLightViewProjectionMatrix() const;

	// シャドウマップテクスチャの取得
	LPDIRECT3DTEXTURE9 GetShadowTexture() const { return m_pShadowTexture; }

	// シャドウマップが有効か
	bool IsShadowMapEnabled() const { return m_pShadowTexture != nullptr; }

private:
	void LoadParameter() override;

	SpotLight m_spotLight;
	float m_lightFov;
	float m_lightRange;

	// ★★★ シャドウマップ用のメンバー変数 ★★★
	LPDIRECT3DTEXTURE9 m_pShadowTexture;
	LPDIRECT3DSURFACE9 m_pShadowSurface;
	LPDIRECT3DSURFACE9 m_pShadowDepthBuffer;
	int m_shadowMapSize;

	// ライトのビュー・プロジェクション行列
	D3DXMATRIX m_matLightView;
	D3DXMATRIX m_matLightProj;
};