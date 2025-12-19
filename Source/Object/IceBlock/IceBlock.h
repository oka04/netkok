// IceBlock.h - 溶ける氷ブロッククラス（球体版）
#pragma once

#include "..\\GameBase.h"
#include "..\\System\\Engine\\Mesh\\Primitive.h"
#include <d3dx9.h>
#include "..\\Scene\\Scene\\Scene.h"
#include <fstream>
#include "..\\json.hpp"

class IceBlock
{
public:
	IceBlock();
	~IceBlock();

	void Initialize(Engine* pEngine, float width, float height, float depth,
		const D3DXVECTOR3& position, float meltDuration = 0.0f);

	void Update(float deltaTime);

	void Draw(Engine* pEngine, Camera* pCamera, Projection* pProj,
		AmbientLight* pAmbient, DirectionalLight* pLight);

	void SetMeltAmount(float amount);
	float GetMeltAmount() const { return m_meltAmount; }
	bool IsFullyMelted() const { return m_meltAmount >= 1.0f; }

	void SetPosition(const D3DXVECTOR3& position);
	void DrawThroughWalls(Engine* pEngine, Camera* pCamera, Projection* pProj,
		AmbientLight* pAmbient, DirectionalLight* pLight, float alpha = 0.5f);
	const D3DXVECTOR3& GetPosition() const { return m_position; }

	void SetRotation(float x, float y, float z);
	void SetScale(float x, float y, float z);
	void SetColor(const D3DXVECTOR4& color) { m_iceColor = color; }

private:
	void UpdateWorldMatrix();
	void ReleaseResources();
	void LoadParameter(); 
	Primitive m_primitiveSphere;

	D3DXVECTOR3 m_position;
	D3DXVECTOR3 m_rotation;
	D3DXVECTOR3 m_scale;
	D3DXVECTOR3 m_originalSize;

	D3DXMATRIX m_matWorld;

	float m_meltAmount;
	float m_meltDuration;
	float m_elapsedTime;

	D3DXVECTOR4 m_iceColor;

	bool m_autoMelt;

	// ★★★ パラメータファイルから読み込む値 ★★★
	float f_iceColorR;
	float f_iceColorG;
	float f_iceColorB;
	float f_iceAlpha;
	float f_ambientMultiplier;
	float f_diffuseMultiplier;
	float f_specularPower;
	float f_minScale;
	float f_throughWallAlphaMultiplier;
	float f_throughWallDiffuseMultiplier;
	float f_throughWallAmbientMultiplier;
	float f_alphaFadeRate;
};