#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>
#include <fstream>
#include "..\\json.hpp"
#include <queue>
#include <map>

#include "..\\..\\GameBase.h"
#include "..\\..\\Scene\\Scene\\Scene.h"
#include "..\\Patroller\\PatrollerManager.h"
#include "..\\GoalEffect\\GoalEffect.h"

class PatrollerManager;
class Patroller;


class Map 
{
public:
	void Initialize(Engine * pEngine, PatrollerManager& patrollerManager, Camera * pCamera, Projection * pProj, AmbientLight * pAmbient, DirectionalLight * pLight, const int mapNumber);
	void Release(Engine * pEngine);
	void UpdateGoalEffect();
	void DrawMap(Engine* pEngine, Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight, std::vector<SpotLight>* lights);
	void DrawGoalEffect(Camera* pCamera, Projection* pProj);
	void DrawMiniMap(Engine * pEngine, const D3DXVECTOR2 & playerPosition, const float arrowAngle, PatrollerManager& patrollerManager);
	void DebugBoxLine(Engine * pEngine, Camera * pCamera, Projection * pProj);

	//移動できるかの確認(移動はこの関数内で行う）
	void MoveCheck(D3DXVECTOR3 &position, const D3DXVECTOR3 &vector, const float &radius);

	//壁の張り付きの確認
	bool StickWallCheck(D3DXVECTOR3& position, float radius, float checkDist, D3DXVECTOR3* outNormal);
	bool RayToWallIntersection(const D3DXVECTOR3& rayOrigin, const D3DXVECTOR3& rayEnd, D3DXVECTOR3* outIntersection);
	bool CheckGoal(const D3DXVECTOR3& playerPosition);

	const D3DXVECTOR3 AdjustCameraPosition(const D3DXVECTOR3& playerPosition, const D3DXVECTOR3& desiredCameraPosition);
	const D3DXVECTOR3& GetPlayerStartPosition();

	// A*アルゴリズムで目的地までの最短経路を調べる
	const std::vector<D3DXVECTOR3> FindPath(const D3DXVECTOR3& startPos, const D3DXVECTOR3& targetPos);
private:
	struct Node
	{
		int x, y;
		float gCost;
		float hCost;
		float fCost;
		int parent;
	};

	struct WallRect
	{
		D3DXVECTOR2 min;
		D3DXVECTOR2 max;
	};
	void LoadParameter();
	void LoadMap(Engine * pEngine, PatrollerManager& patrollerManager, Camera * pCamera, Projection * pProj, AmbientLight * pAmbient, DirectionalLight * pLight, const int mapNumber);
	void CreateWall(Engine * pEngine);
	// 壁に埋まっていたら押し戻す
	void Resolve(const WallRect& rect, D3DXVECTOR2& position, float radius);
	//進めるかチェックする
	void CheckWallsInNeighborhood(D3DXVECTOR2& position, float radius);
	void SaveMiniMap(Engine* pEngine, Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight);
	float GetDistance(int x1, int y1, int x2, int y2);
	bool IsHit(const WallRect& rect, const D3DXVECTOR2& position, float radius);

	// ボックスとレイが当たっているか
	bool RayToRectIntersection(const D3DXVECTOR3& rayOrigin, const D3DXVECTOR3& rayEnd, const WallRect& rect, D3DXVECTOR3* outIntersection);

	//ここからファイルデータ(変更不可)
	int f_miniMapDiv;
	int f_miniMapSourHalfSize;
	float f_wallSize;
	float f_wallHeight;
	float f_groundHeight;
	float f_miniMapHeight;
	float f_miniMapSaveScale;
	float f_miniMapScreenRadius;
	float f_cameraWallOffset;
	float f_goalPinOffset;
	D3DXVECTOR2 f_pinSourSize;
	D3DXVECTOR2 f_playerPinDestHalfSize;
	D3DXVECTOR2 f_enemyPinDestHalfSize;
	D3DXVECTOR2 f_goalPinDestHalfSize;
	D3DXVECTOR2 f_miniMapPosition;
	D3DCOLORVALUE f_lineColor;
	D3DMATERIAL9 f_groundMaterial;
	D3DMATERIAL9 f_wallMaterial;
	//ここまで

	enum CHIP_KIND
	{
		WALL,
		GROUND,
		PLAYER,
		GOAL,
		WAYPOINT_OFFSET = 10,
	};

	struct GridCell
	{
		std::vector<WallRect*> wallRects;
	};

	int m_row;
	int m_col;

	std::vector<std::vector<int>> m_map;
	std::vector<D3DXVECTOR3> m_waypoints;
	std::vector<std::vector<GridCell>> m_grid;
	std::vector<std::unique_ptr<Primitive>> m_wall;
	std::vector<std::unique_ptr<BoundingBox>> m_wallBB;
	std::vector<std::unique_ptr<WallRect>> m_wallRect;

	D3DXVECTOR3 m_mapSize;
	D3DXVECTOR2 m_mapSaveSize;
	D3DXVECTOR3 m_playerStartPosition;
	D3DXVECTOR3 m_goalPosition;

	Primitive m_ground;
	GoalEffect m_goalEffect;
};