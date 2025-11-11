#pragma once

#include "..\\..\\GameBase.h"

#include "..\\..\\Scene\\Scene\\Scene.h"
#include "..\\..\\Object\\CharacterBase\\CharacterBase.h"
#include "PatrollerFileData.h"
#include "..\\Map\\Map.h"
#include <vector>
#include <fstream>
#include "..\\json.hpp" 

class Map;
class PatrollerManager;

class Patroller : public CharacterBase
{
public:
	void Initialize(Engine* pEngine, std::vector<D3DXVECTOR3*> waypoints, PatrollerFileData* fileData, AkGameObjectID id);
	void ReleaseSE();
	bool Update(Engine* pEngine, Map& map, const D3DXVECTOR3& playerPos, const std::vector<std::unique_ptr<Patroller>>& patrollers, D3DXVECTOR3* m_outPatrollerPosition, const float deltaTime);
	void Draw(Engine* pEngine, Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight);
	void DebugViewLine(Engine* pEngine, Camera* pCamera, Projection* pProj);
	void DebugPrint(Engine * pEngine);
	const int GetAlertCount()const;
	const bool IsAlert()const;
	const bool IsViewPlayer()const;
	SpotLight* GetLight();
private:
	enum class PATROLLER_STATE
	{
		PATROL,     //巡回
		CHASE,      //追跡
		SEARCH,     //索敵
		RETURN,     //巡回ルートへ戻る
	};

	void UpdateSound(const D3DXVECTOR3& playerPos, Map& map);
	void UpdatePatrol(Engine* pEngine, Map& map, const float deltaTime);
	void UpdateChase(Engine* pEngine, Map& map, const D3DXVECTOR3& playerPos, const float deltaTime);
	void UpdateSearch(Engine* pEngine, Map& map, const D3DXVECTOR3& playerPos, const float deltaTime);
	void UpdateReturn(Engine* pEngine, Map& map, const float deltaTime);
	void UpdateMatrix(Engine* pEngine);
	
	void MoveTowards(Map& map, const D3DXVECTOR3& targetPos, const float deltaTime, bool bRotateMove);
	void ResolveCollision(const std::vector<std::unique_ptr<Patroller>>& patrollers);
	bool IsSeePlayer(const D3DXVECTOR3& playerPos, Map& map);
	bool IsGameOver(const D3DXVECTOR3& playerPos);

	static const D3DXVECTOR3 DEPTH_DIRECTION;
	static const D3DXVECTOR3 UP_DIRECTION;

	AkGameObjectID m_id;
	AkPlayingID m_seFootId;

	int m_lookAroundCount;
	int m_returnWaypointIndex;
	int m_pathIndex;
	int m_waypointIndex;
	int m_alertCount;
	float m_initialAngle;
	float m_hAngle;
	float m_vAngle;
	float m_fov;
	float m_lightDistance;
	bool m_bLookingLeft;
	bool m_bViewPlayer;
	D3DXVECTOR3 m_lastPlayerPosition;
	D3DXVECTOR3 m_depth;
	D3DXVECTOR3 m_hori;
	D3DXVECTOR3 m_vector;

	std::vector<D3DXVECTOR3*> m_waypoints;
	std::vector<D3DXVECTOR3> m_pathToReturn;
	PATROLLER_STATE m_state;
	PatrollerFileData* f_fileData;
	SpotLight m_spotLight;
};