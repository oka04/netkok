#define _USING_V110_SDK71_ 1

#include "Patroller.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;

using namespace std;

const D3DXVECTOR3 Patroller::UP_DIRECTION = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
const D3DXVECTOR3 Patroller::DEPTH_DIRECTION = D3DXVECTOR3(0.0f, 0.0f, 1.0f);

void Patroller::Initialize(Engine* pEngine, vector<D3DXVECTOR3*> waypoints, PatrollerFileData* fileData, AkGameObjectID id)
{
	CharacterBase::Initialize(pEngine, MODEL_CHARACTER);
	m_waypoints = waypoints;
	m_waypointIndex = 0;
	m_position = *m_waypoints[m_waypointIndex];

	m_direction = *m_waypoints[(m_waypointIndex + 1) % (int)m_waypoints.size()] - m_position;
	D3DXVec3Normalize(&m_direction, &m_direction);
	m_angle = atan2f(m_direction.x, m_direction.z);
	m_initialAngle = m_angle;

	m_depth = DEPTH_DIRECTION;

	m_id = id;
	SoundManager::RegisterGameObject(id, "PATROLLER");

	m_hAngle = 0.0f;
	m_vAngle = 0.0f;
	m_vector = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	m_lightDistance = fileData->patrolDistance;

	m_lookAroundCount = 0;
	m_bLookingLeft = true;
	m_bViewPlayer = false;
	m_state = PATROLLER_STATE::PATROL;
	m_returnWaypointIndex = 0;
	m_pathIndex = 0;
	m_alertCount = 0;

	f_fileData = fileData;

	D3DLIGHT9 light = fileData->baseLight.GetLight();
	m_spotLight.SetLight(&light);

	D3DXVECTOR3 eyePosition = D3DXVECTOR3(m_position.x, m_position.y + f_fileData->eyePositionY, m_position.z);
	m_spotLight.SetPosition(eyePosition);
	m_spotLight.SetDirection(m_direction);
	m_spotLight.SetDevice(pEngine, 0);
	UpdateMatrix(pEngine);
}

void Patroller::ReleaseSE()
{
	if (m_seFootId != AK_INVALID_PLAYING_ID)
	{
		SoundManager::StopEvent(m_seFootId);
		m_seFootId = AK_INVALID_PLAYING_ID;
	}

	SoundManager::UnregisterGameObject(m_id);
}

bool Patroller::Update(Engine* pEngine, Map& map, const D3DXVECTOR3& playerPos, const vector<unique_ptr<Patroller>>& patrollers, D3DXVECTOR3* m_outPatrollerPosition, const float deltaTime)
{
	//プレイヤーを視認しているか
	m_bViewPlayer = IsSeePlayer(playerPos, map);

	if (m_bViewPlayer && (m_state == PATROLLER_STATE::PATROL || m_state == PATROLLER_STATE::RETURN))
	{
		m_alertCount++;
		SoundManager::Play(AK::EVENTS::PLAY_SE_ALERT, m_id);
	}

	//ゲームオーバーになった場合は処理を終了する
	if (m_bViewPlayer && IsGameOver(playerPos))
	{
		if (m_outPatrollerPosition) *m_outPatrollerPosition = m_position;
		return true;
	}

	//プレイヤーを視認している場合は追いかける
	if (m_bViewPlayer)
	{
		m_state = PATROLLER_STATE::CHASE;
		m_lightDistance = f_fileData->chaseDistance;
		m_spotLight.SetRange(m_lightDistance);
		m_lastPlayerPosition = playerPos;
	}

	switch (m_state)
	{
	case PATROLLER_STATE::PATROL:
		UpdatePatrol(pEngine, map, deltaTime);
		break;

	case PATROLLER_STATE::CHASE:
		UpdateChase(pEngine, map, playerPos, deltaTime);
		break;

	case PATROLLER_STATE::SEARCH:
		UpdateSearch(pEngine, map, playerPos, deltaTime);
		break;

	case PATROLLER_STATE::RETURN:
		UpdateReturn(pEngine, map, deltaTime);
		break;
	}

	//他の敵キャラとの衝突を判定
	ResolveCollision(patrollers);

	UpdateSound(playerPos, map);

	UpdateMatrix(pEngine);
	return false;
}

void Patroller::Draw(Engine* pEngine, Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight)
{
	CharacterBase::Draw(pCamera, pProj, pAmbient, pLight);
}

void Patroller::DebugViewLine(Engine * pEngine, Camera* pCamera, Projection* pProj)
{
	D3DXVECTOR3 eyePosition = D3DXVECTOR3(m_position.x, m_position.y + f_fileData->eyePositionY, m_position.z);
	D3DXMATRIX viewMat = pCamera->GetViewMatrix();
	D3DXMATRIX projMat = pProj->GetProjectionMatrix();
	pEngine->DrawLine(eyePosition, eyePosition + m_direction * m_spotLight.GetRange(), Color::CYAN, &viewMat, &projMat);
}

void Patroller::DebugPrint(Engine * pEngine)
{
}

const int Patroller::GetAlertCount() const
{
	return m_alertCount;
}

const bool Patroller::IsAlert() const
{
	return (m_state == PATROLLER_STATE::CHASE || m_state == PATROLLER_STATE::SEARCH);
}

const bool Patroller::IsViewPlayer() const
{
	return m_bViewPlayer;
}

SpotLight* Patroller::GetLight()
{
	return &m_spotLight;
}

void Patroller::UpdateSound(const D3DXVECTOR3& playerPos, Map& map)
{
	SoundManager::SetPosition(m_position, m_direction, UP_DIRECTION, m_id);

	D3DXVECTOR3 eyePos(m_position.x, m_position.y + f_fileData->eyePositionY, m_position.z);
	D3DXVECTOR3 intersection;
	bool blocked = map.RayToWallIntersection(eyePos, playerPos, &intersection);

	//距離の計算（引数での計算を避ける）
	D3DXVECTOR3 dir_toPlayer = playerPos - eyePos;
	float dist = D3DXVec3Length(&dir_toPlayer);
	float maxDist = f_fileData->chaseDistance;

	//距離による遮蔽/遮音の影響度 
	float distFactor = min(dist / maxDist, 1.0f) / 2.0f;

	float obstruction = 0.0f;
	float occlusion = 0.0f;

	if (blocked) {
		//壁を挟んでいる場合 (音のこもり/定位のぼやけ)
		obstruction = f_fileData->obstructionValue + distFactor;
		occlusion = f_fileData->occlusionValue + distFactor;
	}
	else {
		//壁を挟んでいないが距離がある場合
		obstruction = distFactor;
		occlusion = distFactor * f_fileData->distFactorMultiplier;
	}

	//遮蔽/遮音の値をWwiseに送信
	AK::SoundEngine::SetObjectObstructionAndOcclusion(m_id,	SoundManager::ID_LISTENER, min(obstruction, 1.0f), min(occlusion, 1.0f));

	D3DXVECTOR3 dir_velocity = m_vector;
	bool bMoving = (D3DXVec3Length(&dir_velocity) > 0.01f);

	if (bMoving)
	{
		float interval = 0.0f;

		if (m_state == PATROLLER_STATE::CHASE) {
			interval = 1.0f;
		}


		AK::SoundEngine::SetRTPCValue(AK::GAME_PARAMETERS::FOOTSPEED, interval, m_id);

		if (m_seFootId == AK_INVALID_PLAYING_ID) {
			m_seFootId = SoundManager::Play(AK::EVENTS::PLAY_SE_FOOT, m_id);
		}
	}
	else 
	{
		if (m_seFootId != AK_INVALID_PLAYING_ID) {
			SoundManager::StopEvent(m_seFootId);
			m_seFootId = AK_INVALID_PLAYING_ID;
		}
	}
}


void Patroller::UpdatePatrol(Engine* pEngine, Map& map, const float deltaTime)
{
	D3DXVECTOR3* targetWaypoint = m_waypoints[m_waypointIndex];
	D3DXVECTOR3 toWaypoint = *targetWaypoint - m_position;

	//巡回ポイントに到着したら次の巡回ポイントに向かう
	if (D3DXVec3Length(&toWaypoint) < f_fileData->arriveDistance)
	{
		m_waypointIndex = (m_waypointIndex + 1) % (int)m_waypoints.size();
	}

	MoveTowards(map, *m_waypoints[m_waypointIndex], deltaTime, false);
}

void Patroller::UpdateChase(Engine * pEngine, Map & map, const D3DXVECTOR3 & playerPos, const float deltaTime)
{
	if (m_bViewPlayer)
	{
		D3DXVECTOR3 toPlayer = playerPos - m_position;
		float distance = D3DXVec3Length(&toPlayer);

		//プレイヤーが近い状態で視界の外側にいる場合は回転のみ
		//それ以外の状態は移動しながら回転させる
		D3DXVec3Normalize(&toPlayer, &toPlayer);

		bool bMove = (distance < f_fileData->patrolDistance && D3DXVec3Dot(&m_direction, &toPlayer) <= cosf(f_fileData->theta / 2.0f));

		MoveTowards(map, playerPos, deltaTime, !bMove);
	}
	else
	{
		//プレイヤーを見失ったら探す状態に入る
		m_state = PATROLLER_STATE::SEARCH;
		m_lookAroundCount = 0;
		m_initialAngle = m_angle;
		m_bLookingLeft = true;
	}
}

void Patroller::UpdateSearch(Engine* pEngine, Map& map, const D3DXVECTOR3& playerPos, const float deltaTime) {
	D3DXVECTOR3 toLastPos = m_lastPlayerPosition - m_position;
	float distToLastPos = D3DXVec3Length(&toLastPos);

	if (distToLastPos > f_fileData->arriveDistance)
	{
		//プレイヤーを最後に見た場所まで移動する
		MoveTowards(map, m_lastPlayerPosition, deltaTime, true);
	}
	else
	{
		//左右を見渡してプレイヤーを探す
		float turnAmount = f_fileData->turnSpeed * deltaTime;

		if (m_bLookingLeft)
		{
			m_angle += turnAmount; 
		}
		else
		{
			m_angle -= turnAmount; 
		}

		float maxAngle = m_initialAngle + f_fileData->searchMaxAngle;
		float minAngle = m_initialAngle - f_fileData->searchMaxAngle;

		if (m_bLookingLeft && m_angle > maxAngle)
		{
			m_angle = maxAngle;
			m_bLookingLeft = !m_bLookingLeft;
			m_lookAroundCount++;
		}
		else if (!m_bLookingLeft && m_angle < minAngle)
		{
			m_angle = minAngle;
			m_bLookingLeft = !m_bLookingLeft;
			m_lookAroundCount++;
		}

		if (m_lookAroundCount < f_fileData->searchMaxCount) return;

		//f_searchMaxCount回分左右を見渡したら元の巡回ポイントまで戻る
		m_state = PATROLLER_STATE::RETURN;
		m_lightDistance = f_fileData->patrolDistance;
		m_spotLight.SetRange(m_lightDistance);

		float closestDist = FLT_MAX;
		int closestWaypointIndex = 0;
		//一番近いウェイポイントを探す
		for (int i = 0; i < (int)m_waypoints.size(); ++i)
		{
			D3DXVECTOR3 toWaypoint = (*m_waypoints[i]) - m_position;
			float dist = D3DXVec3Length(&toWaypoint);
			if (dist < closestDist)
			{
				closestDist = dist;
				closestWaypointIndex = (int)i;
			}
		}
		m_returnWaypointIndex = closestWaypointIndex;

		//一番近いウェイポイントまでの道順を取得する
		m_pathToReturn = map.FindPath(m_position, *m_waypoints[m_returnWaypointIndex]);
		m_pathIndex = 0;
	}
}

void Patroller::UpdateReturn(Engine* pEngine, Map& map, const float deltaTime)
{
	if (m_pathIndex < (int)m_pathToReturn.size())
	{
		//一番近いウェイポイントまでの道順をたどる
		D3DXVECTOR3 targetPos = m_pathToReturn[m_pathIndex];
		D3DXVECTOR3 toTarget = targetPos - m_position;

		//パスに近づいたら次のパスを設定する
		if (D3DXVec3Length(&toTarget) < f_fileData->arriveDistance)
		{
			m_pathIndex++;
		}

		MoveTowards(map, targetPos, deltaTime, false);
	}
	else
	{
		m_state = PATROLLER_STATE::PATROL;
		m_lightDistance = f_fileData->patrolDistance;
		m_spotLight.SetRange(m_lightDistance);
		m_waypointIndex = m_returnWaypointIndex;
	}
}

void Patroller::UpdateMatrix(Engine* pEngine)
{
	m_direction.x = sinf(m_angle);
	m_direction.y = 0.0f;
	m_direction.z = cosf(m_angle);
	D3DXVec3Normalize(&m_direction, &m_direction);

	D3DXVECTOR3 eyePosition = D3DXVECTOR3(m_position.x, m_position.y + f_fileData->eyePositionY, m_position.z);
	m_spotLight.SetPosition(eyePosition);
	m_spotLight.SetDirection(m_direction);
	m_spotLight.SetDevice(pEngine, 0);

	CharacterBase::UpdateMatrix();
}

void Patroller::MoveTowards(Map& map, const D3DXVECTOR3& targetPos, const float deltaTime, bool bRotateMove)
{
	D3DXVECTOR3 toTarget = targetPos - m_position;
	float dist = D3DXVec3Length(&toTarget);

	m_vector = D3DXVECTOR3(0, 0, 0);

	//目的地点に十分近づいたら移動を停止
	if (dist < f_fileData->arriveDistance) return;

	float targetAngle = atan2f(toTarget.x, toTarget.z);
	float angleDiff = targetAngle - m_angle;

	while (angleDiff > D3DX_PI) angleDiff -= 2 * D3DX_PI;
	while (angleDiff < -D3DX_PI) angleDiff += 2 * D3DX_PI;

	float turnAmount = f_fileData->turnSpeed * deltaTime;
	bool bFacingTarget = (fabs(angleDiff) < f_fileData->angleTolerance);

	if (bFacingTarget)
	{
		//移動方向がほとんどあっているので移動のみ
		m_vector = m_direction * ((m_state == PATROLLER_STATE::CHASE) ? f_fileData->chaseSpeed : f_fileData->searchSpeed) * deltaTime;
		map.MoveCheck(m_position, m_vector, f_fileData->radius);
	}
	else
	{
		if (bRotateMove)
		{
			//回転しながら移動
			if (angleDiff > 0)
			{
				m_angle += min(angleDiff, turnAmount);
			}
			else
			{
				m_angle += max(angleDiff, -turnAmount);
			}
			m_vector = m_direction * ((m_state == PATROLLER_STATE::CHASE) ? f_fileData->chaseSpeed : f_fileData->searchSpeed) * deltaTime;
			map.MoveCheck(m_position, m_vector, f_fileData->radius);
		}
		else
		{
			//停止して回転
			if (angleDiff > 0)
			{
				m_angle += min(angleDiff, turnAmount);
			}
			else
			{
				m_angle += max(angleDiff, -turnAmount);
			}
		}
	}
}

//ほかの敵キャラとの当たり判定
void Patroller::ResolveCollision(const vector<unique_ptr<Patroller>>& patrollers)
{
	for (const auto& patroller : patrollers)
	{
		//自分自身との比較はスキップ
		if (patroller.get() == this) continue;

		D3DXVECTOR2 myPos2D(m_position.x, m_position.z);
		D3DXVECTOR2 otherPos2D(patroller->GetPosition().x, patroller->GetPosition().z);
		D3DXVECTOR2 toOther = otherPos2D - myPos2D;

		float distSq = D3DXVec2LengthSq(&toOther);
		float combinedRadius = f_fileData->radius + f_fileData->radius;

		//衝突しているかチェック
		if (distSq >= combinedRadius * combinedRadius) continue;
		float dist = sqrtf(distSq);
		float overlap = combinedRadius - dist;

		//押し戻す方向
		D3DXVECTOR2 pushDir;
		if (dist == 0) pushDir = D3DXVECTOR2(1.0f, 0.0f);
		else pushDir = toOther / dist;

		m_position.x -= pushDir.x * overlap * 0.5f;
		m_position.z -= pushDir.y * overlap * 0.5f;
	}
}

//プレイヤーを視認できるかチェック
bool Patroller::IsSeePlayer(const D3DXVECTOR3& playerPos, Map& map)
{
	//距離のチェック
	D3DXVECTOR3 eyePosition = D3DXVECTOR3(m_position.x, m_position.y + f_fileData->eyePositionY, m_position.z);
	D3DXVECTOR3 toPlayer = playerPos - eyePosition;
	float distToPlayer = D3DXVec3Length(&toPlayer); 
	if (distToPlayer > m_lightDistance) return false;

	//視野角内のチェック
	D3DXVec3Normalize(&toPlayer, &toPlayer);

	float fov = f_fileData->phi;
	if (D3DXVec3Dot(&m_direction, &toPlayer) < cosf(fov)) return false;

	//障害物のチェック
	D3DXVECTOR3 intersectionPoint;
	if (map.RayToWallIntersection(eyePosition, playerPos, &intersectionPoint))
	{
		D3DXVECTOR3 toWall = intersectionPoint - eyePosition;
		float distToWall = D3DXVec3Length(&toWall);

		// 壁の距離がプレイヤーまでの距離よりも近ければ、遮蔽されていると判定する
		if (distToWall < distToPlayer) return false;
	}
	return true;
}

//プレイヤーを視認した状態で一定距離以内にいた場合はゲームオーバー
bool Patroller::IsGameOver(const D3DXVECTOR3& playerPos)
{
	D3DXVECTOR2 enemyPos2D(m_position.x, m_position.z);
	D3DXVECTOR2 playerPos2D(playerPos.x, playerPos.z);
	D3DXVECTOR2 dir = enemyPos2D - playerPos2D;
	float dist = D3DXVec2Length(&dir);

	return dist < f_fileData->gameOverDistance;
}