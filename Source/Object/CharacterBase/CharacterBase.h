#pragma once

#include "..\\..\\GameBase.h"

#include "..\\..\\Scene\\Scene\\Scene.h"
#include "..\\Object\\Map\\Map.h"

class Map;
class CharacterBase
{
public:
	void SetFirstPersonCamera(Engine* pEngine, Camera & camera);
	void SetThirdPersonFromBehind(Engine* pEngine, Camera& camera, Map& map);

	const D3DXVECTOR3& GetPosition()const;
	const D3DXVECTOR3& GetDirection()const;
	const D3DXVECTOR3& GetDepth()const;
	const D3DXVECTOR2& GetPosition2D()const;
	
	const float& GetFov()const;
	const float& GetRadius()const;
	const float& GetArrowAngle()const;
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
	void Draw(Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight);
	void Input(Engine * pEngine);

	//m_speedを変えてから呼び出す
	void Move(Map& map);
	void SetMouseCursor(Engine* pEngine, Camera& camera);

	virtual void LoadParameter() = 0;
	virtual void UpdateStamina();
	virtual void DrawStaminaGauge();

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
	float f_baseHAngle;
	float f_baseVAngle;
	float f_stickAngleH;

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
};
