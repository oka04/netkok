#pragma once

#include "..\\..\\GameBase.h"

#include "..\\..\\Scene\\Scene\\Scene.h"
#include "..\\..\\Object\\CharacterBase\\CharacterBase.h"
#include "..\\..\\Object\\Map\\Map.h"


#include <fstream>
#include "..\\json.hpp" 

class Player : public CharacterBase 
{
public:
	void Initialize(Engine* pEngine, Map& map, Projection* projection, Camera& camera, DirectionalLight &light);
	void Release(Engine* pEngine);
	void Update(Engine* pEngine, Map& map, Camera& camera, DirectionalLight &light, float deltaTime);
	void Draw(Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight);
	void DrawStaminaGauge(Engine* pEngine);
	void DebugPrint(Engine * pEngine);

	void SetFirstPersonCamera(Engine* pEngine, Camera& camera);
	void SetThirdPersonFromBehind(Engine* pEngine, Camera& camera, Map& map);

	bool RotateToTarget(DirectionalLight &light, const D3DXVECTOR3& targetPos, float deltaTime);

	const float GetFov()const;
	const float GetRadius()const;
	const float GetArrowAngle()const;
	D3DXVECTOR2 GetPosition2D()const;
	D3DXVECTOR3 GetDepth()const;
private:
	void LoadParameter();
	void StickWallMove(Map& map, float deltaTime);
	void NormalMove(Map& map, float deltaTime);
	void SetMouseCursor(Engine* pEngine, Camera& camera);
	void Input(Engine* pEngine, Map& map);
	void UpdateMatrix(DirectionalLight &light);
	void UpdateStamina(float deltaTime);

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
		STICK_KEY = 1 << 6,
	};

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
	float f_crouchSpeed;
	float f_walkSpeed;
	float f_dashSpeed;
	float f_maxAngleV;
	float f_minAngleV;
	float f_defaultSenseV;
	float f_defaultSenseH; 
	float f_radius;
	float f_headSize;
	float f_baseHAngle;
	float f_baseVAngle;
	float f_stickAngleH;
	float f_rotateSpeed;
	float f_minRotateSpeed;
	float f_rotateFinishAngle;
			
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

	unsigned short m_keyFlag;

	//視野角
	float m_fov;
	float m_speed;
	//角度
	float m_hAngle; //水平方向
	float m_vAngle; //垂直方向

	float m_stamina;
	float m_staminaRecoveryTimer;
	bool m_bFatigued;

	bool m_bFirstPerson;

	D3DXVECTOR3 m_stickNormal;
	D3DXVECTOR3 m_cameraFront; //カメラの方向
	D3DXVECTOR3 m_eyePosition;

	//方向
	D3DXVECTOR3 m_depth; //前後
	D3DXVECTOR3 m_hori;//左右
};