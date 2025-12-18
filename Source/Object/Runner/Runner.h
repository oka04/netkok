// Runner.h - 氷状態管理機能を追加 + 解凍ゲージ表示
#pragma once

#include <winsock2.h> 
#include <ws2tcpip.h>

#include "..\\GameBase.h"
#include "..\\Scene\\Scene\\Scene.h"
#include "..\\Object\\CharacterBase\\CharacterBase.h"
#include "..\\Object\\Map\\Map.h"
#include "..\\Object\\IceBlock\\IceBlock.h"
#include <fstream>
#include "..\\json.hpp"

class Runner : public CharacterBase
{
public:
	Runner();
	~Runner();

	void Initialize(Engine* pEngine, Map& map, Projection* projection, Camera& camera, DirectionalLight& light);
	void InitializeAtPosition(Engine* pEngine, const D3DXVECTOR3& startPos, Projection* projection, Camera& camera, DirectionalLight& light);
	void Release(Engine* pEngine);

	void Update(Engine* pEngine, Map& map, Camera& camera, DirectionalLight& light, float deltaTime);

	void Draw(Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight);
	D3DXVECTOR3 GetCenterPosition() const override
	{
		D3DXVECTOR3 centerPos = m_position;
		centerPos.y += f_height / 2.0f;
		return centerPos;
	}
	virtual void DrawEffects(Engine* pEngine, Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight) override;
	virtual void DrawDepth(Engine* pEngine, const D3DXMATRIX* pMatLightVP) override;
	void DrawStaminaGauge(Engine* pEngine) override;
	void DebugPrint(Engine* pEngine);

	virtual NetPlayerState GetNetState() const override;

	virtual void UpdateFromNetwork(const NetPlayerState& state, DirectionalLight& light, float deltaTime) override;

	// ★★★ 氷状態関連 ★★★
	bool IsFrozen() const { return m_bFrozen; }
	void SetFrozen(bool frozen);
	float GetFrozenAmount() const { return m_frozenAmount; }
	void SetFrozenAmount(float amount);  // ★★★ 追加：外部から氷の溶け具合を設定 ★★★
	void UpdateFrozenState(float deltaTime);
	void TryMeltNearbyFrozenPlayer(Engine* pEngine, const std::vector<std::pair<uint32_t, CharacterBase*>>& players, float deltaTime);

	// ★★★ 解凍ゲージ表示 ★★★
	void DrawMeltGauge(Engine* pEngine, Camera* pCamera, Projection* pProj, float viewerDistance);

private:
	void LoadParameter() override;
	void ChangeSpeed();
	void UpdateStamina(float deltaTime)override;

	// ★★★ ゲージ描画用ヘルパー ★★★
	void DrawGaugeRect(LPDIRECT3DDEVICE9 pDevice, int x, int y, int width, int height, D3DCOLOR color);

	enum GAUGE_COLOR
	{
		GREEN,
		YELLOW,
		RED,
		GRAY,
		FATIGUE,
	};

	int f_gaugeAlpha;
	float f_maxStamina;
	float f_dashStaminaCost;
	float f_walkRecoveryRate;
	float f_stopRecoveryRate;
	float f_fatigueRecoveryRate;
	float f_recoveryDelayTime;
	float f_fatigueSpeed;
	float f_fatigueRecoveryThreshold;
	std::vector<float> f_gaugeColorThresholds;

	IntVector2 f_gaugeSourSize;
	IntVector2 f_gaugeDestSize;
	IntVector2 f_gaugePosition;

	float m_stamina;
	float m_staminaRecoveryTimer;
	bool m_bFatigued;

	// ★★★ 氷状態管理 ★★★
	bool m_bFrozen;               // 凍結状態フラグ
	float m_frozenAmount;         // 氷の溶け具合 (0.0 = 完全凍結, 1.0 = 完全解凍)
	IceBlock* m_pIceBlock;        // 氷ブロック表示用
	float f_meltRange;            // 解凍可能な距離
	float f_meltSpeed;            // 解凍速度
	uint32_t m_targetMeltPlayer;  // 現在解凍中のプレイヤーID（0なら解凍していない）
};