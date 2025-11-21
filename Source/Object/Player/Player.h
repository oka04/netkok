#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include "..\\..\\GameBase.h"
#include "..\\..\\Scene\\Scene\\Scene.h"
#include "..\\..\\Object\\CharacterBase\\CharacterBase.h"
#include "..\\..\\Object\\Map\\Map.h"
#include "..\\..\\Object\\Network\\NetworkSync.h"
#include <fstream>
#include "..\\json.hpp"

class Player : public CharacterBase
{
public:
	Player();
	~Player();

	void Initialize(Engine* pEngine, Map& map, Projection* projection, Camera& camera, DirectionalLight& light);
	void InitializeAtPosition(Engine* pEngine, const D3DXVECTOR3& startPos, Projection* projection, Camera& camera, DirectionalLight& light);
	void Release(Engine* pEngine);

	void Update(Engine* pEngine, Map& map, Camera& camera, DirectionalLight& light, float deltaTime);
	void UpdateFromNetwork(const NetPlayerState& state, DirectionalLight& light, float deltaTime);

	void Draw(Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight);
	void DrawStaminaGauge(Engine* pEngine);
	void DebugPrint(Engine* pEngine);

	NetPlayerState GetNetState() const;
	void SetClientId(uint32_t id) { m_clientId = id; }
	uint32_t GetClientId() const { return m_clientId; }
	void SetPlayerName(const std::string& name) { m_playerName = name; }
	const std::string& GetPlayerName() const { return m_playerName; }
	void SetIsLocal(bool local) { m_bIsLocal = local; }
	bool IsLocal() const { return m_bIsLocal; }

	void SetPosition(const D3DXVECTOR3& pos) { m_position = pos; }
	const D3DXVECTOR3& GetEyePosition() const { return m_eyePosition; }

private:
	void LoadParameter();
	void ChangeSpeed();
	void UpdateStamina(float deltaTime);

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

	uint32_t m_clientId;
	std::string m_playerName;
	bool m_bIsLocal;

	D3DXVECTOR3 m_targetPosition;
	float m_targetHAngle;
	float m_targetVAngle;
	float m_interpolationSpeed;
};