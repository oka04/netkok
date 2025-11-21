// Player.h にネットワーク用のメソッドとデータ構造を追加
#pragma once

#include "..\\..\\GameBase.h"
#include "..\\..\\Scene\\Scene\\Scene.h"
#include "..\\..\\Object\\CharacterBase\\CharacterBase.h"
#include "..\\..\\Object\\Map\\Map.h"
#include <fstream>
#include "..\\json.hpp" 

// ネットワーク送信用のプレイヤーデータ構造体
struct PlayerNetworkData
{
	uint32_t clientId;
	float posX, posY, posZ;
	float hAngle;
	float vAngle;
	float depthX, depthY, depthZ;
	uint8_t keyFlag;
	float stamina;
	bool bFirstPerson;
};

class Player : public CharacterBase
{
public:
	void Initialize(Engine* pEngine, Map& map, Projection* projection, Camera& camera, DirectionalLight &light);
	void Release(Engine* pEngine);

	// ローカルプレイヤー用の通常更新
	void Update(Engine* pEngine, Map& map, Camera& camera, DirectionalLight &light, float deltaTime);

	// リモートプレイヤー用の更新（受信データで更新）
	void UpdateFromNetwork(const PlayerNetworkData& data, DirectionalLight &light);

	void Draw(Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight);
	void DrawStaminaGauge(Engine* pEngine);
	void DebugPrint(Engine * pEngine);

	// ネットワーク用のGetter/Setter
	PlayerNetworkData GetNetworkData() const;
	void SetClientId(uint32_t id) { m_clientId = id; }
	uint32_t GetClientId() const { return m_clientId; }
	void SetPlayerName(const std::string& name) { m_playerName = name; }
	const std::string& GetPlayerName() const { return m_playerName; }

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

	//ファイルからの読み込み（f_は変更不可）
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
	D3DXVECTOR3 f_standEyePosition;
	D3DXVECTOR3 f_crouchEyePosition;
	//ここまで変更不可

	float m_stamina;
	float m_staminaRecoveryTimer;
	bool m_bFatigued;

	// ネットワーク用
	uint32_t m_clientId;
	std::string m_playerName;
};