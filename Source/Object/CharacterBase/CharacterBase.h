// CharacterBase.h - Update/Draw/SetThirdPersonFromBehindをpublicに変更
#pragma once

#include <winsock2.h> 
#include <ws2tcpip.h>

#include "..\\..\\GameBase.h"
#include "..\\..\\Scene\\Scene\\Scene.h"
#include "..\\Object\\Map\\Map.h"
#include "..\\..\\Object\\Network\\NetworkSync.h"
#include "..\\..\\Object\\Network\\NetworkLogger.h"

class Map;
class CharacterBase
{
public:
	virtual void Update(Engine* pEngine, Map& map, Camera& camera, DirectionalLight& light, float deltaTime) = 0;
	void Draw(Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight);

	virtual void DrawDepth(Engine* pEngine, const D3DXMATRIX* pMatLightVP) = 0;
	void SetFirstPersonCamera(Engine* pEngine, Camera & camera);
	void SetThirdPersonFromBehind(Engine* pEngine, Camera& camera, Map& map);

	const D3DXVECTOR3& GetPosition()const;
	const D3DXVECTOR3& GetDirection()const;
	const D3DXVECTOR3& GetDepth()const;
	const D3DXVECTOR2 GetPosition2D()const;

	virtual D3DXVECTOR3 GetCenterPosition() const
	{
		D3DXVECTOR3 centerPos = m_position;
		centerPos.y += f_height / 2.0f;
		return centerPos;
	}

	const float& GetFov()const;
	const float& GetRadius()const;
	const float& GetArrowAngle()const;

	virtual void DrawEffects(Engine* pEngine, Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight) = 0;
	virtual NetPlayerState GetNetState() const;
	virtual void UpdateFromNetwork(const NetPlayerState& state, DirectionalLight& light, float deltaTime);
	void PredictMovement(float deltaTime);

	void SetClientId(uint32_t id) { m_clientId = id; }
	uint32_t GetClientId() const { return m_clientId; }

	void SetCharacterName(const std::string& name) { m_characterName = name; }
	const std::string& GetCharacterName() const { return m_characterName; }

	void SetIsLocal(bool local) { m_bIsLocal = local; }
	bool IsLocal() const { return m_bIsLocal; }

	void SetPosition(const D3DXVECTOR3& pos) { m_position = pos; }
	const D3DXVECTOR3& GetEyePosition() const { return m_eyePosition; }

	virtual uint32_t GetMeltTargetId() const { return 0; }
	unsigned char GetKeyFlag() const { return m_keyFlag; }

protected:
	static const D3DXVECTOR3 DEPTH_DIRECTION;
	static const D3DXVECTOR3 UP_DIRECTION;

	enum KEY_FLAG
	{
		W_KEY = 1 << 0,
		A_KEY = 1 << 1,
		S_KEY = 1 << 2,
		D_KEY = 1 << 3,
		CROUCH_KEY = 1 << 4,
		DASH_KEY = 1 << 5,
		ATTACK_KEY = 1 << 6,  
	};

	enum GAUGE_COLOR
	{
		GREEN,
		YELLOW,
		RED,
		GRAY,
		FATIGUE,
	};

	void Initialize(Engine *pEngine, std::string filename, Projection* projection, Camera& camera, DirectionalLight &light);
	void UpdateMatrix(DirectionalLight &light);
	void Input(Engine * pEngine);

	//m_speedを変えてから呼び出す
	void Move(Map& map);
	void SetMouseCursor(Engine* pEngine, Camera& camera);

	virtual void LoadParameter() = 0;
	virtual void UpdateStamina(float deltaTime) {}
	virtual void DrawStaminaGauge(Engine* pEngine) {}

	// ★★★ ネットワーク同期用の内部メソッド ★★★
	void AddPositionToHistory(const D3DXVECTOR3& pos);
	D3DXVECTOR3 GetAveragedPosition() const;

	//ファイルからの読み込み用
	//必ず実装させるクラスですべて読み込むこと

	float f_crouchSpeed;
	float f_walkSpeed;
	float f_dashSpeed;
	float f_maxAngleV;
	float f_minAngleV;
	float f_defaultSenseV;
	float f_defaultSenseH;
	float f_radius;
	float f_headSize;
	float f_height;
	float f_baseHAngle;
	float f_baseVAngle;
	float f_eyePsoitionY;
	D3DXVECTOR3 f_standEyePosition;
	D3DXVECTOR3 f_crouchEyePosition;
	//ここまで

	Model m_model;
	D3DXVECTOR3 m_position;
	D3DXVECTOR3 m_direction;
	D3DXVECTOR3 m_cameraFront;
	D3DXVECTOR3 m_eyePosition;

	D3DXVECTOR3 m_depth;
	D3DXVECTOR3 m_hori;

	D3DXMATRIX m_matWorld;
	D3DXMATRIX m_matRotate;
	D3DXMATRIX m_matTrans;

	unsigned char m_keyFlag;

	float m_angle;
	float m_speed;
	float m_hAngle;
	float m_vAngle;
	float m_fov;
	float m_deltaTime;

	bool m_bFirstPerson;

	// ★★★ ネットワーク同期用メンバー変数 ★★★
	uint32_t m_clientId;
	std::string m_characterName;
	bool m_bIsLocal;

	// ★★★ 補間用変数 ★★★
	D3DXVECTOR3 m_targetPosition;
	float m_targetHAngle;
	float m_targetVAngle;
	float m_interpolationSpeed;
	float m_adaptiveInterpolationSpeed;

	// ★★★ 予測移動用変数 ★★★
	D3DXVECTOR3 m_velocity;
	D3DXVECTOR3 m_predictedPosition;
	D3DXVECTOR3 m_smoothedVelocity;
	float m_velocitySmoothingFactor;

	// ★★★ ジッター対策用変数 ★★★
	static const int MAX_POSITION_HISTORY = 5;
	D3DXVECTOR3 m_positionHistory[MAX_POSITION_HISTORY];
	int m_positionHistoryIndex;
	int m_positionHistoryCount;

	// ★★★ タイムスタンプ管理 ★★★
	DWORD m_lastUpdateTime;
	float m_timeSinceLastUpdate;
};