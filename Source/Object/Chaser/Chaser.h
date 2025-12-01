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

	SpotLight * GetLights();

	virtual NetPlayerState GetNetState() const override;

	virtual void UpdateFromNetwork(const NetPlayerState& state, DirectionalLight& light, float deltaTime) override;
	void UpdateLight(Engine* pEngine);

private:
	void LoadParameter() override;
	SpotLight m_spotLight;
	float m_lightFov;
	float m_lightRange;
};