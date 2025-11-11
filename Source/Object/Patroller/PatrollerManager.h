#pragma once

#include "..\\..\\GameBase.h"

#include "..\\Patroller\\Patroller.h"
#include "..\\Map\\Map.h"
#include "PatrollerFileData.h"

#include <vector>
#include <memory>

class Map;
class Patroller;

class PatrollerManager
{
public:
	void Initialize(Engine* pEngine, int sceneNumber);
	void Release(Engine* pEngine);
	void ReleaseSE();
	void SetPatroller(Engine* pEngine, std::vector<D3DXVECTOR3*> waypoints);
	bool Update(Engine * pEngine, Map & map, const D3DXVECTOR3 & playerPos, D3DXVECTOR3* m_outPatrollerPosition, float deltaTime);
	void Draw(Engine* pEngine, Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight);
	void DebugViewLine(Engine * pEngine, Camera* pCamera, Projection* pProj);
	void DebugPrint(Engine * pEngine);
	void DrawHeartBeat(Engine * pEngine, const D3DXVECTOR3& playerPos);
	const int GetAlertCount() const;
	const bool IsViewPlayer()const;
	std::vector<SpotLight>* GetLights(const D3DXVECTOR3& playerPos, const D3DXVECTOR3& playerDir, const float fov);
	std::vector<D3DXVECTOR2> GetMiniMapPositions(const D3DXVECTOR2& playerPosition, float mapSize);
private:
	void LoadParameter();
	std::vector<std::unique_ptr<Patroller>> m_patrollers;
	std::vector<SpotLight> m_lights;
	PatrollerFileData f_fileData;
	 
	int m_sceneNumber;
	float m_heartbeatAlpha;
	float m_beatTime;
	bool m_bAlert;
};