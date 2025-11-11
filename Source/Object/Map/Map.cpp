 #define _USING_V110_SDK71_ 1

#include "Map.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;

using namespace std;

void Map::Initialize(Engine * pEngine, PatrollerManager& patrollerManager, Camera * pCamera, Projection * pProj, AmbientLight * pAmbient, DirectionalLight * pLight, const int mapNumber)
{
	LoadMap(pEngine, patrollerManager, pCamera, pProj, pAmbient, pLight, mapNumber);

	CreateWall(pEngine);

	// マップのサイズに合わせて地面を生成
	m_mapSize = D3DXVECTOR3(m_col * f_wallSize, f_wallHeight, m_row * f_wallSize);
	m_ground.CreateBox(pEngine, m_mapSize.x, f_groundHeight, m_mapSize.z);
	m_ground.SetMaterial(f_groundMaterial);

	D3DXMATRIX matTrans;
	D3DXVECTOR3 position = D3DXVECTOR3(m_mapSize.x / 2.0f, 0.0f, m_mapSize.z / 2.0f);

	D3DXMatrixTranslation(&matTrans, &position);
	m_ground.SetWorldTransform(&matTrans);

	SaveMiniMap(pEngine, pCamera, pProj, pAmbient, pLight);

	pEngine->AddTexture(TEXTURE_MINI_MAP);
	pEngine->AddTexture(TEXTURE_PLAYER_PIN);
	pEngine->AddTexture(TEXTURE_ENEMY_PIN);
	pEngine->AddTexture(TEXTURE_GOAL_PIN);
	pEngine->AddTexture(TEXTURE_NORMAL_CIRCLE);
	pEngine->AddTexture(TEXTURE_CHASE_CIRCLE);
}

void Map::Release(Engine * pEngine)
{
	pEngine->ReleaseTexture(TEXTURE_MINI_MAP);
	pEngine->ReleaseTexture(TEXTURE_PLAYER_PIN);
	pEngine->ReleaseTexture(TEXTURE_ENEMY_PIN);
	pEngine->ReleaseTexture(TEXTURE_GOAL_PIN);
	pEngine->ReleaseTexture(TEXTURE_NORMAL_CIRCLE);
	pEngine->ReleaseTexture(TEXTURE_CHASE_CIRCLE);
}

void Map::UpdateGoalEffect()
{
	m_goalEffect.Update();
}
	
void Map::DrawMap(Engine* pEngine, Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight, vector<SpotLight>* lights)
{
	for (const auto& wall : m_wall) 
	{
		wall->Draw(pEngine, pCamera, pProj, pAmbient, pLight, lights);
	}

	m_ground.Draw(pEngine, pCamera, pProj, pAmbient, pLight, lights);
}

void Map::DrawGoalEffect(Camera * pCamera, Projection * pProj)
{
	m_goalEffect.Draw(pCamera, pProj);
}

void Map::DrawMiniMap(Engine* pEngine, const D3DXVECTOR2& playerPosition, const float arrowAngle, PatrollerManager& patrollerManager)
{
	pEngine->SpriteBegin();
	pEngine->BeginMiniMap(D3DXVECTOR2(f_miniMapPosition.x + f_miniMapScreenRadius, f_miniMapPosition.y + f_miniMapScreenRadius), f_miniMapScreenRadius, f_miniMapDiv);

	int displaySize = (int)(f_miniMapScreenRadius * 2);

	int x = (int)(playerPosition.x * f_miniMapSaveScale);
	int y = (int)(m_mapSaveSize.y - playerPosition.y * f_miniMapSaveScale);

	// ミニマップの描画
	RECT sour;
	sour.left = x - (int)(f_miniMapSourHalfSize * f_miniMapSaveScale);
	sour.top = y - (int)(f_miniMapSourHalfSize * f_miniMapSaveScale);
	sour.right = x + (int)(f_miniMapSourHalfSize * f_miniMapSaveScale);
	sour.bottom = y + (int)(f_miniMapSourHalfSize * f_miniMapSaveScale);

	RECT dest;
	dest.left = (int)f_miniMapPosition.x;
	dest.top = (int)f_miniMapPosition.y;
	dest.right = dest.left + displaySize;
	dest.bottom = dest.top + displaySize;
	pEngine->Blt(&dest, TEXTURE_MINI_MAP, &sour, 255, 0.0f);

	D3DXVECTOR2 miniMapCenter = D3DXVECTOR2(f_miniMapPosition.x + f_miniMapScreenRadius, f_miniMapPosition.y + f_miniMapScreenRadius);

	vector<D3DXVECTOR2> patrollerPositions = patrollerManager.GetMiniMapPositions(playerPosition, (float)f_miniMapSourHalfSize);

	SetRect(&sour, 0, 0, (int)f_pinSourSize.x, (int)f_pinSourSize.y);
	float scale = f_miniMapScreenRadius / f_miniMapSourHalfSize;

	// 敵の位置の描画
	for (const auto& patrollerPos : patrollerPositions)
	{
		D3DXVECTOR2 relativePos = patrollerPos - playerPosition;
		D3DXVECTOR2 drawPosition = miniMapCenter + D3DXVECTOR2(relativePos.x, -relativePos.y) * scale;

		dest.left = (int)(drawPosition.x - f_enemyPinDestHalfSize.x);
		dest.top = (int)(drawPosition.y - f_enemyPinDestHalfSize.y);
		dest.right = (int)(drawPosition.x + f_enemyPinDestHalfSize.x);
		dest.bottom = (int)(drawPosition.y + f_enemyPinDestHalfSize.y);

		pEngine->Blt(&dest, TEXTURE_ENEMY_PIN, &sour, 255, 0.0f);
	}

	// ゴールピンの描画
	D3DXVECTOR2 goalPosition2D(m_goalPosition.x, m_goalPosition.z);
	D3DXVECTOR2 relativePos = goalPosition2D - playerPosition;
	float goalDistSq = D3DXVec2LengthSq(&relativePos);
	float miniMapRadiusSq = (float)(f_miniMapSourHalfSize * f_miniMapSourHalfSize);
	D3DXVECTOR2 drawGoalPosition;
	float goalAngle = 0.0f;

	if (goalDistSq > miniMapRadiusSq) {
		// ミニマップの外にある場合、円周上に描画する
		D3DXVECTOR2 dir = relativePos;
		D3DXVec2Normalize(&dir, &dir);
		drawGoalPosition = miniMapCenter + D3DXVECTOR2(dir.x, -dir.y) * (f_miniMapScreenRadius - f_goalPinDestHalfSize.x - f_goalPinOffset);
		goalAngle = atan2f(-dir.x, -dir.y);
	}
	else {
		// ミニマップの内にある場合、そのまま描画する
		drawGoalPosition = miniMapCenter + D3DXVECTOR2(relativePos.x, -relativePos.y) * scale;
	}

	dest.left = (int)(drawGoalPosition.x - f_goalPinDestHalfSize.x);
	dest.top = (int)(drawGoalPosition.y - f_goalPinDestHalfSize.y);
	dest.right = (int)(drawGoalPosition.x + f_goalPinDestHalfSize.x);
	dest.bottom = (int)(drawGoalPosition.y + f_goalPinDestHalfSize.y);
	pEngine->Blt(&dest, TEXTURE_GOAL_PIN, &sour, 255, goalAngle);

	// プレイヤーの位置の描画
	dest.left = (int)(miniMapCenter.x - f_playerPinDestHalfSize.x);
	dest.top = (int)(miniMapCenter.y - f_playerPinDestHalfSize.y);
	dest.right = (int)(miniMapCenter.x + f_playerPinDestHalfSize.x);
	dest.bottom = (int)(miniMapCenter.y + f_playerPinDestHalfSize.y);
	pEngine->Blt(&dest, TEXTURE_PLAYER_PIN, &sour, 255, arrowAngle);

	// ミニマップを囲う円の描画
	SetRect(&sour, 0, 0, displaySize, displaySize);

	dest.left = (int)f_miniMapPosition.x;
	dest.top = (int)f_miniMapPosition.y;
	dest.right = dest.left + displaySize;
	dest.bottom = dest.top + displaySize;

	if (patrollerManager.IsViewPlayer()) pEngine->Blt(&dest, TEXTURE_CHASE_CIRCLE, &sour, 255, 0.0f);
	else pEngine->Blt(&dest, TEXTURE_NORMAL_CIRCLE, &sour, 255, 0.0f);

	pEngine->SpriteEnd();
	pEngine->EndMiniMap();
}

// 生成したボックスのラインを描画（デバッグ用）
void Map::DebugBoxLine(Engine * pEngine, Camera * pCamera, Projection * pProj)
{
	for (const auto& bb : m_wallBB) 
	{
		bb->Draw(pEngine, pCamera, pProj, &f_lineColor);
	}
}

//移動できるかの確認(移動はこの関数内で行う）
void Map::MoveCheck(D3DXVECTOR3& position, const D3DXVECTOR3& vector, const float& radius)
{
	D3DXVECTOR2 pos2D(position.x, position.z);

	// X方向の移動
	pos2D.x += vector.x;
	CheckWallsInNeighborhood(pos2D, radius);

	// Z方向の移動
	pos2D.y += vector.z;
	CheckWallsInNeighborhood(pos2D, radius);

	position.x = pos2D.x;
	position.z = pos2D.y;
}

//壁の張り付きの確認
bool Map::StickWallCheck(D3DXVECTOR3& position, float radius, float checkDist, D3DXVECTOR3* outNormal)
{
	D3DXVECTOR2 playerPos2D(position.x, position.z);
	D3DXVECTOR2 bestClosestPoint = D3DXVECTOR2(0.0f, 0.0f);
	D3DXVECTOR2 bestNormal2D = D3DXVECTOR2(0.0f, 0.0f);
	float closestDistSq = checkDist * checkDist;
	bool bFoundWall = false;

	// プレイヤーがいるグリッドセル
	int cellX = (int)(position.x / f_wallSize);
	int cellY = (int)((m_row * f_wallSize - position.z) / f_wallSize);
	 
	// 現在のセルとその周囲のセルをチェック
	for (int dy = -1; dy <= 1; dy++) 
	{
		for (int dx = -1; dx <= 1; dx++) 
		{
			int checkX = cellX + dx;
			int checkY = cellY + dy;

			// グリッドの境界チェック
			if (checkX < 0 || checkX >= m_col || checkY < 0 || checkY >= m_row) continue;
			// セル内のすべての壁をチェック
			for (const auto& rect : m_grid[checkY][checkX].wallRects) {
				// 辺との衝突を確認
				D3DXVECTOR2 closestPointOnEdge;
				float closestX = max(rect->min.x, min(playerPos2D.x, rect->max.x));
				float closestY = max(rect->min.y, min(playerPos2D.y, rect->max.y));

				closestPointOnEdge.x = closestX;
				closestPointOnEdge.y = closestY;

				D3DXVECTOR2 toClosestOnEdge = closestPointOnEdge - playerPos2D;
				float distSqOnEdge = D3DXVec2LengthSq(&toClosestOnEdge);

				//壁が近い位置にあるか
				if (distSqOnEdge >= closestDistSq) continue;

				D3DXVECTOR2 normal;
				if (fabs(toClosestOnEdge.x) > fabs(toClosestOnEdge.y)) 
				{
					normal = (toClosestOnEdge.x < 0) ? D3DXVECTOR2(1.0f, 0.0f) : D3DXVECTOR2(-1.0f, 0.0f);
				}
				else
				{
					normal = (toClosestOnEdge.y < 0) ? D3DXVECTOR2(0.0f, 1.0f) : D3DXVECTOR2(0.0f, -1.0f);
				}

				//壁の裏側にいる場合は処理しない
				if (D3DXVec2Dot(&toClosestOnEdge, &normal) >= 0.0f) continue;

				closestDistSq = distSqOnEdge;
				bestClosestPoint = closestPointOnEdge;
				bestNormal2D = normal;
				bFoundWall = true;
			}
		}
	}

	if (!bFoundWall) return false;

	D3DXVECTOR2 newPos2D = bestClosestPoint + bestNormal2D * radius;
	position.x = newPos2D.x;
	position.z = newPos2D.y;

	if (outNormal) *outNormal = D3DXVECTOR3(bestNormal2D.x, 0.0f, bestNormal2D.y);
	return true;
}

// DDAアルゴリズムを使いrayと壁の判定を行う
bool Map::RayToWallIntersection(const D3DXVECTOR3& rayOrigin, const D3DXVECTOR3& rayEnd, D3DXVECTOR3* outIntersection)
{
	D3DXVECTOR3 rayDirection = rayEnd - rayOrigin;
	float rayLength = D3DXVec3Length(&rayDirection);

	if (rayLength < 1e-6f) return false;

	D3DXVec3Normalize(&rayDirection, &rayDirection);

	// 始点と終点のグリッド座標を計算
	int startX = (int)(rayOrigin.x / f_wallSize);
	int startY = (int)((m_row * f_wallSize - rayOrigin.z) / f_wallSize);

	int currentX = startX;
	int currentY = startY;

	int stepX = (rayDirection.x >= 0) ? 1 : -1;
	int stepY = (rayDirection.z >= 0) ? -1 : 1; 

	// 1グリッド分移動するのにかかる時間
	float tDeltaX = f_wallSize / fabsf(rayDirection.x);
	float tDeltaY = f_wallSize / fabsf(rayDirection.z);

	float tMaxX, tMaxY;

	// x軸の始点から次のグリッド境界までの距離を計算
	if (rayDirection.x >= 0)
	{
		tMaxX = ((startX + 1) * f_wallSize - rayOrigin.x) / rayDirection.x;
	}
	else
	{
		tMaxX = (startX * f_wallSize - rayOrigin.x) / rayDirection.x;
	}

	// y軸（z軸）の始点から次のグリッド境界までの距離を計算
	if (rayDirection.z >= 0) 
	{
		tMaxY = ((m_row * f_wallSize - startY * f_wallSize) - rayOrigin.z) / rayDirection.z;
	}
	else 
	{
		tMaxY = ((m_row * f_wallSize - (startY + 1) * f_wallSize) - rayOrigin.z) / rayDirection.z;
	}


	float t = 0.0f;
	D3DXVECTOR3 rayVec = rayEnd - rayOrigin;
	float maxDistSq = D3DXVec3LengthSq(&rayVec);

	// グリッドをたどりながら壁をチェック
	while (currentX >= 0 && currentX < m_col && currentY >= 0 && currentY < m_row)
	{
		for (const auto& wallRect : m_grid[currentY][currentX].wallRects)
		{
			D3DXVECTOR3 intersection;
			if (!RayToRectIntersection(rayOrigin, rayEnd, *wallRect, &intersection)) continue;

			// 壁が始点と終点の間にあるか確認
			D3DXVECTOR3 distToIntersection = intersection - rayOrigin;
			if (D3DXVec3LengthSq(&distToIntersection) < maxDistSq)
			{
				*outIntersection = intersection;
				return true;
			}
		}

		// 次のグリッドセルに移動
		if (tMaxX < tMaxY)
		{
			t = tMaxX;
			tMaxX += tDeltaX;
			currentX += stepX;
		}
		else
		{
			t = tMaxY;
			tMaxY += tDeltaY;
			currentY += stepY;
		}

		// レイの終点を超えたらループを抜ける
		if (t > rayLength) break;
	}
	return false;
}

bool Map::CheckGoal(const D3DXVECTOR3 & playerPosition)
{
	int playerX = (int)(playerPosition.x / f_wallSize);
	int playerY = (int)((m_row * f_wallSize - playerPosition.z) / f_wallSize);

	int goalX = (int)(m_goalPosition.x / f_wallSize);
	int goalY = (int)((m_row * f_wallSize - m_goalPosition.z) / f_wallSize);

	return ((playerX == goalX) && (playerY == goalY));

}

// 3人称視点にしたときのカメラの位置が壁の裏側に行かないようにする
const D3DXVECTOR3 Map::AdjustCameraPosition(const D3DXVECTOR3& playerPosition, const D3DXVECTOR3& desiredCameraPosition)
{
	D3DXVECTOR3 finalCameraPosition = desiredCameraPosition;
	D3DXVECTOR3 closestIntersectionPoint;
	bool bFoundIntersection = false;
	float closestDistSq = FLT_MAX;

	// プレイヤーの位置からカメラの位置までのグリッドセルを効率的に探索
	if (RayToWallIntersection(playerPosition, desiredCameraPosition, &closestIntersectionPoint)) 
	{
		// 交差が見つかった場合
		D3DXVECTOR3 distVec = closestIntersectionPoint - playerPosition;
		closestDistSq = D3DXVec3LengthSq(&distVec);
		bFoundIntersection = true;
	}

	if (bFoundIntersection) 
	{
		float closestDist = sqrtf(closestDistSq);

		if (closestDist > f_cameraWallOffset) 
		{
			D3DXVECTOR3 dir = desiredCameraPosition - playerPosition;
			D3DXVec3Normalize(&dir, &dir);
			finalCameraPosition = playerPosition + dir * (closestDist - f_cameraWallOffset);
		}
		else 
		{
			finalCameraPosition = playerPosition + D3DXVECTOR3(0.0f, 1.0f, 0.0f);
		}
	}

	return finalCameraPosition;
}

// プレイヤーの開始位置の取得
const D3DXVECTOR3& Map::GetPlayerStartPosition()
{
	return m_playerStartPosition;
}

// A*アルゴリズムで目的地までの最短経路を調べる
const vector<D3DXVECTOR3> Map::FindPath(const D3DXVECTOR3& startPos, const D3DXVECTOR3& targetPos) 
{

	// D3DXVECTOR3の座標をグリッドのインデックスに変換
	vector<D3DXVECTOR3> path;
	int startX = (int)(startPos.x / f_wallSize);
	int startY = (int)((m_row * f_wallSize - startPos.z) / f_wallSize);
	int targetX = (int)(targetPos.x / f_wallSize);
	int targetY = (int)((m_row * f_wallSize - targetPos.z) / f_wallSize);

	// 開始点または目標点がマップの範囲外、または壁の場合は空のパスを返す
	if (startX < 0 || startX >= m_col || startY < 0 || startY >= m_row ||
		targetX < 0 || targetX >= m_col || targetY < 0 || targetY >= m_row ||
		m_map[startY][startX] == WALL || m_map[targetY][targetX] == WALL) 
	{
		return path;
	}

	// グリッドセルを保持するコンテナ
	vector<Node> allNodes(m_row * m_col);
	// 2次元座標を1次元インデックスに変換するラムダ関数
	auto getNodeIndex = [&](int x, int y) { return y * m_col + x; };

	// 全ノードの初期化
	for (size_t i = 0; i < allNodes.size(); ++i) 
	{
		allNodes[i].gCost = FLT_MAX; // 開始点からの距離
		allNodes[i].parent = -1; // 親ノード
		allNodes[i].x = i % m_col; // 1次元インデックスのx座標
		allNodes[i].y = i / m_col; // 1次元インデックスのy座標
		allNodes[i].hCost = GetDistance(allNodes[i].x, allNodes[i].y, targetX, targetY); // 目的地までの直線距離
	}

	// 開始ノードの設定
	int startIndex = getNodeIndex(startX, startY);
	allNodes[startIndex].gCost = 0;
	allNodes[startIndex].fCost = allNodes[startIndex].hCost; 

	// 探索すべきノードを優先度付きキューで管理（fCostが最も低いノードを優先）
	priority_queue<pair<float, int>, vector<pair<float, int>>, greater<pair<float, int>>> openList;
	openList.push({ allNodes[startIndex].fCost, startIndex });

	// 探索済みノードを管理するリスト
	vector<bool> closedList(m_row * m_col, false);

	int endNodeIndex = -1; // 目標ノードのインデックス

	int dx[] = { 0, 0, 1, -1 };
	int dy[] = { 1, -1, 0, 0 };

	// オープンリストが空になるまで探索を続ける
	while (!openList.empty()) {
		// fCostが最も低いノードを取得
		int currentIndex = openList.top().second;
		openList.pop();

		// 既に処理済みの場合はスキップ
		if (closedList[currentIndex]) continue;

		closedList[currentIndex] = true;

		Node& currentNode = allNodes[currentIndex];

		// 目標ノードに到達したら探索を終了
		if (currentNode.x == targetX && currentNode.y == targetY)
		{
			endNodeIndex = currentIndex;
			break;
		}

		// 上下左右の4方向を探索
		for (int i = 0; i < 4; i++) 
		{
			int neighborX = currentNode.x + dx[i];
			int neighborY = currentNode.y + dy[i];

			// 隣接ノードがマップの範囲内で、かつ壁ではないかチェック
			if (neighborX < 0 || neighborX >= m_col || neighborY < 0 || neighborY >= m_row || m_map[neighborY][neighborX] == WALL) continue;
			
			int neighborIndex = getNodeIndex(neighborX, neighborY);
			Node& neighborNode = allNodes[neighborIndex];

			float newGCost = currentNode.gCost + 1;

			// 移動回数が少ない場合は更新する
			if (newGCost < neighborNode.gCost) 
			{
				neighborNode.gCost = newGCost;
				neighborNode.fCost = newGCost + neighborNode.hCost;
				neighborNode.parent = currentIndex; 
				openList.push({ neighborNode.fCost, neighborIndex });
			}

		}
	}

	// 経路が見つかった場合、親をたどってパスを構築
	if (endNodeIndex != -1) 
	{
		int index = endNodeIndex;
		while (index != -1) 
		{
			Node& current = allNodes[index];
			// グリッド座標をワールド座標に変換
			D3DXVECTOR3 waypoint = D3DXVECTOR3(current.x * f_wallSize + f_wallSize / 2.0f, 0.0f, (m_row - current.y) * f_wallSize - f_wallSize / 2.0f);
			path.insert(path.begin(), waypoint); // パスを逆順に追加
			index = current.parent;
		}
	}

	return path;
}

void Map::LoadParameter()
{
	ifstream file(JSON_MAP_PARAMETER);
	if (!file.is_open())
	{
		throw DxSystemException(DxSystemException::OM_FILE_OPEN_ERROR);
	}

	nlohmann::json config;
	file >> config;
	file.close();

	f_wallSize = config["wallSize"];
	f_wallHeight = config["wallHeight"];
	f_groundHeight = config["groundHeight"];
	f_miniMapHeight = config["miniMapHeight"];
	f_miniMapSaveScale = config["miniMapSaveScale"];
	f_miniMapScreenRadius = config["miniMapScreenRadius"];
	f_miniMapSourHalfSize = config["miniMapSourHalfSize"];
	f_miniMapDiv = config["miniMapDiv"];
	f_goalPinOffset = config["goalPinOffset"];
	f_cameraWallOffset = config["cameraWallOffset"];
	
	for (int i = 0; i < 2; i++)
	{
		f_pinSourSize[i] = config["pinSourSize"][i];
		f_playerPinDestHalfSize[i] = config["playerPinDestHalfSize"][i];
		f_enemyPinDestHalfSize[i] = config["enemyPinDestHalfSize"][i];
		f_goalPinDestHalfSize[i] = config["goalPinDestHalfSize"][i];
		f_miniMapPosition[i] = config["miniMapPosition"][i];
	}

	// rgbaは[i]みたいに入れられないため1つずつ値を入れる
	f_groundMaterial.Diffuse.r = config["groundMaterial"][0];
	f_groundMaterial.Diffuse.g = config["groundMaterial"][1];
	f_groundMaterial.Diffuse.b = config["groundMaterial"][2];
	f_groundMaterial.Diffuse.a = config["groundMaterial"][3];

	f_wallMaterial.Diffuse.r = config["wallMaterial"][0];
	f_wallMaterial.Diffuse.g = config["wallMaterial"][1];
	f_wallMaterial.Diffuse.b = config["wallMaterial"][2];
	f_wallMaterial.Diffuse.a = config["wallMaterial"][3];

	f_lineColor.r = config["lineColor"][0];
	f_lineColor.g = config["lineColor"][1];
	f_lineColor.b = config["lineColor"][2];
	f_lineColor.a = config["lineColor"][3];
}

void Map::LoadMap(Engine * pEngine, PatrollerManager& patrollerManager, Camera * pCamera, Projection * pProj, AmbientLight * pAmbient, DirectionalLight * pLight, const int mapNumber)
{
	LoadParameter();

	ifstream ifsMap;
	stringstream filename;

	filename.str("");
	filename << MAP_FILE_PATH << mapNumber << MAP_FILE_END;

	ifsMap.open(filename.str(), ios::in);
	if (!ifsMap)
	{
		throw DxSystemException(DxSystemException::OM_FILE_OPEN_ERROR);
	}

	ifsMap >> m_row >> m_col;

	int maxWaypoint;
	ifsMap >> maxWaypoint;

	m_waypoints.resize(maxWaypoint);
	m_map.resize(m_row, vector<int>(m_col, WALL));

	// マップ情報をいったん格納する
	for (int i = 0; i < m_row; i++)
	{
		for (int j = 0; j < m_col; j++) {
			ifsMap >> m_map[i][j];

			if (m_map[i][j] == PLAYER)
			{
				m_map[i][j] = GROUND;
				m_playerStartPosition = D3DXVECTOR3(j * f_wallSize + f_wallSize / 2, f_groundHeight / 2, (m_row - i) * f_wallSize - f_wallSize / 2);
			}

			if (m_map[i][j] == GOAL)
			{
				m_goalPosition = D3DXVECTOR3(j * f_wallSize + f_wallSize / 2, f_groundHeight / 2, (m_row - i) * f_wallSize - f_wallSize / 2);
				m_goalEffect.Initialize(pEngine, m_goalPosition, ParticleBase::INIFINITY);
				m_map[i][j] = GROUND;
			}

			if (m_map[i][j] >= WAYPOINT_OFFSET)
			{
				m_waypoints[m_map[i][j] - WAYPOINT_OFFSET] = D3DXVECTOR3(j * f_wallSize + f_wallSize / 2, f_groundHeight / 2, (m_row - i) * f_wallSize - f_wallSize / 2);
				m_map[i][j] = GROUND;
			}
		}
	}

	int enemyCount;
	ifsMap >> enemyCount;

	int waypointCount;
	vector <D3DXVECTOR3*> waypoints;
	// ウェイポイントの番号を取得してその番号のウェイポイントを格納し、敵に渡す
	for (int i = 0; i < enemyCount; i++)
	{
		ifsMap >> waypointCount;
		waypoints.clear();
		for (int j = 0; j < waypointCount; j++)
		{
			int waypointNumber;
			ifsMap >> waypointNumber;
			waypoints.push_back(&m_waypoints[waypointNumber - WAYPOINT_OFFSET]);
		}
		patrollerManager.SetPatroller(pEngine, waypoints);
	}

	ifsMap.close();
}

// 壁の生成
void Map::CreateWall(Engine* pEngine)
{
	m_grid.resize(m_row, vector<GridCell>(m_col));
	vector<vector<bool>> bProcessed(m_row, vector<bool>(m_col, false));

	m_wall.clear();
	m_wallBB.clear();
	m_wallRect.clear();

	for (int i = 0; i < m_row; i++) 
	{
		for (int j = 0; j < m_col; j++) 
		{
			if (m_map[i][j] != WALL || bProcessed[i][j]) continue;
			int width = 0;
			// 連続して作れるブロックを探す
			while (j + width < m_col && m_map[i][j + width] == WALL && !bProcessed[i][j + width]) 
			{
				width++;
			}

			int depth = 0;
			while (i + depth < m_row)
			{
				bool bEnd = false;
				for (int k = 0; k < width; k++) {
					if (m_map[i + depth][j + k] != WALL || bProcessed[i + depth][j + k]) 
					{
						bEnd = true;
						break;
					}
				}
				if (bEnd) break;
				depth++;
			}

			// 壁を作成
			auto wall = make_unique<Primitive>();
			auto bb = make_unique<BoundingBox>();
			auto pRect = make_unique<WallRect>();

			float wallWidth = f_wallSize * width;
			float wallDepth = f_wallSize * depth;

			wall->CreateBox(pEngine, wallWidth, f_wallHeight, wallDepth);

			D3DXVECTOR3 min_bb_local = wall->GetBoundingBoxMin();
			D3DXVECTOR3 max_bb_local = wall->GetBoundingBoxMax();
			bb->CreateBB(pEngine, &max_bb_local, &min_bb_local, true);
			D3DXVECTOR3 position = D3DXVECTOR3((j * f_wallSize) + (wallWidth / 2.0f), f_wallHeight / 2.0f, (m_row * f_wallSize) - (i * f_wallSize) - (wallDepth / 2.0f));
			D3DXMATRIX matTrans;
			D3DXMatrixTranslation(&matTrans, &position);
			wall->SetWorldTransform(&matTrans);
			bb->SetWorldTransform(&matTrans);
			wall->SetMaterial(f_wallMaterial);

			// 当たり判定用のWallRectを作成
			pRect->min.x = j * f_wallSize;
			pRect->max.x = (j + width) * f_wallSize;
			pRect->min.y = (m_row * f_wallSize) - ((i + depth) * f_wallSize);
			pRect->max.y = (m_row * f_wallSize) - (i * f_wallSize);

			m_wall.push_back(move(wall));
			m_wallBB.push_back(move(bb));
			m_wallRect.push_back(move(pRect));

			// ボックスの生成に使用したセルにチェックを入れ、グリッドにポインタを登録
			for (int d = 0; d < depth; d++)
			{
				for (int w = 0; w < width; w++) 
				{
					bProcessed[i + d][j + w] = true;
					// m_wallRectの最後の要素のポインタを登録
					m_grid[i + d][j + w].wallRects.push_back(m_wallRect.back().get());
				}
			}
		}
	}
}

// 壁に埋まっていたら押し戻す
void Map::Resolve(const WallRect& rect, D3DXVECTOR2& position, float radius)
{
	float closestX = max(rect.min.x, min(position.x, rect.max.x));
	float closestY = max(rect.min.y, min(position.y, rect.max.y));

	float dx = position.x - closestX;
	float dy = position.y - closestY;

	float distSq = dx * dx + dy * dy;

	if (distSq != 0.0f) 
	{
		float dist = sqrtf(distSq);
		float overlap = radius - dist;

		D3DXVECTOR2 pushDir(dx, dy);
		pushDir /= dist;
		position += pushDir * overlap;
	}
	else 
	{
		position.x += radius; 
	}
}

//進めるかチェックする
void Map::CheckWallsInNeighborhood(D3DXVECTOR2& position, float radius)
{
	int cellX = (int)(position.x / f_wallSize);
	int cellY = (int)((m_row * f_wallSize - position.y) / f_wallSize);

	for (int dy = -1; dy <= 1; dy++) 
	{
		for (int dx = -1; dx <= 1; dx++) 
		{
			int checkX = cellX + dx;
			int checkY = cellY + dy;

			if (checkX < 0 || checkX >= m_col || checkY < 0 || checkY >= m_row) continue;

			for (const auto& rect : m_grid[checkY][checkX].wallRects) 
			{
				if (!IsHit(*rect, position, radius)) continue;
				//壁にぶつかっていたら押し戻す
				Resolve(*rect, position, radius);
			}
		}
	}
}

//ミニマップの画像を保存する
void Map::SaveMiniMap(Engine * pEngine, Camera * pCamera, Projection * pProj, AmbientLight * pAmbient, DirectionalLight * pLight)
{
	//ミニマップ保存用に設定を変える
	pCamera->m_vecEye = D3DXVECTOR3(m_mapSize.x / 2.0f, f_miniMapHeight, m_mapSize.z / 2.0f);
	pCamera->m_vecAt = D3DXVECTOR3(m_mapSize.x / 2.0f, 0.0f, m_mapSize.z / 2.0f);
	pCamera->m_vecUp = D3DXVECTOR3(0.0f, 0.0f, 1.0f);

	pCamera->SetDevice(pEngine);
	pProj->SetOrthoDevice(pEngine, m_mapSize.x, m_mapSize.z);

	m_mapSaveSize = D3DXVECTOR2(m_mapSize.x * f_miniMapSaveScale, m_mapSize.z * f_miniMapSaveScale);

	LPDIRECT3DSURFACE9 pOld = pEngine->BeginOffScreenRender((int)m_mapSaveSize.x, (int)m_mapSaveSize.y);
	pAmbient->SetColor(1.0f, 0.8f, 0.8f, 0.8f);
	pEngine->Clear(D3DCOLOR_XRGB(0, 0, 0));
	pEngine->BeginScene();
	vector<SpotLight> light;
	DrawMap(pEngine, pCamera, pProj, pAmbient, pLight, &light);
	pEngine->EndScene();

	pEngine->EndOffScreenRender(pOld, TEXTURE_MINI_MAP);

	//設定をもとに戻す
	pAmbient->SetColor(1.0f, 0.1f, 0.1f, 0.1f);

	pProj->SetData(D3DXToRadian(90.0f), 4.0f / 3.0f, 0.1f, 100.0f);
	pProj->SetDevice(pEngine);
}

// マンハッタン距離を計算 (ヒューリスティック関数)
float Map::GetDistance(int x1, int y1, int x2, int y2) 
{
	return (float)(abs(x1 - x2) + abs(y1 - y2));
}

// ボックスに対して当たっているか
bool Map::IsHit(const WallRect& rect, const D3DXVECTOR2& position, float radius)
{
	float closestX = max(rect.min.x, min(position.x, rect.max.x));
	float closestY = max(rect.min.y, min(position.y, rect.max.y));

	float dx = position.x - closestX;
	float dy = position.y - closestY;

	return (dx * dx + dy * dy) < (radius * radius);
}

// ボックスとレイが当たっているか
bool Map::RayToRectIntersection(const D3DXVECTOR3& rayOrigin, const D3DXVECTOR3& rayEnd, const WallRect& rect, D3DXVECTOR3* outIntersection)
{
	// 今の状態だと２Dで判定するほうがいいが、今後のことを考え３Dにしておく
	D3DXVECTOR3 rayDirection = rayEnd - rayOrigin;
	float rayLength = D3DXVec3Length(&rayDirection);
	if (rayLength < 1e-6f) return false;
	D3DXVec3Normalize(&rayDirection, &rayDirection);

	D3DXVECTOR3 boxMin = D3DXVECTOR3(rect.min.x, 0.0f, rect.min.y);
	D3DXVECTOR3 boxMax = D3DXVECTOR3(rect.max.x, f_wallHeight, rect.max.y);

	float t_min = 0.0f;
	float t_max = rayLength;

	if (abs(rayDirection.x) > 1e-6f) 
	{
		float t1 = (boxMin.x - rayOrigin.x) / rayDirection.x;
		float t2 = (boxMax.x - rayOrigin.x) / rayDirection.x;
		t_min = max(t_min, min(t1, t2));
		t_max = min(t_max, max(t1, t2));
	}
	if (abs(rayDirection.y) > 1e-6f) 
	{
		float t1 = (boxMin.y - rayOrigin.y) / rayDirection.y;
		float t2 = (boxMax.y - rayOrigin.y) / rayDirection.y;
		t_min = max(t_min, min(t1, t2));
		t_max = min(t_max, max(t1, t2));
	}
	if (abs(rayDirection.z) > 1e-6f)
	{
		float t1 = (boxMin.z - rayOrigin.z) / rayDirection.z;
		float t2 = (boxMax.z - rayOrigin.z) / rayDirection.z;
		t_min = max(t_min, min(t1, t2));
		t_max = min(t_max, max(t1, t2));
	}

	if (t_min > t_max)
	{
		return false;
	}

	if (outIntersection) 
	{
		*outIntersection = rayOrigin + rayDirection * t_min;
	}

	return true;
}
