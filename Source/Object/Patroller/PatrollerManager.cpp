#define _USING_V110_SDK71_ 1

#include "PatrollerManager.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;
using namespace Common;

using namespace std;

void PatrollerManager::Initialize(Engine * pEngine, int sceneNumber)
{
	m_patrollers.clear();
	LoadParameter();
	m_heartbeatAlpha = 0.0f;
	m_beatTime = 0.0f;
	m_bAlert = false;

	pEngine->AddTexture(TEXTURE_HEARTBEAT);
	m_sceneNumber = sceneNumber;
	if (m_sceneNumber == Common::SCENE_GAME) {
		SoundManager::SetSwitch(AK::SWITCHES::GAME_BGM::GROUP, AK::SWITCHES::GAME_BGM::SWITCH::SEARCH, ID_BGM);
		SoundManager::Play(AK::EVENTS::PLAY_BGM_GAME, ID_BGM);
	}
}

void PatrollerManager::Release(Engine * pEngine)
{
	pEngine->ReleaseTexture(TEXTURE_HEARTBEAT);
}

void PatrollerManager::ReleaseSE()
{
	for (auto& patroller : m_patrollers)
	{
		patroller->ReleaseSE();
	}
}

void PatrollerManager::SetPatroller(Engine* pEngine, vector<D3DXVECTOR3*> waypoints)
{
	AkGameObjectID id = ID_PATROLLER_START + m_patrollers.size();

	auto patroller = make_unique<Patroller>();
	patroller->Initialize(pEngine, waypoints, &f_fileData, id);
	m_patrollers.push_back(move(patroller));
}

bool PatrollerManager::Update(Engine* pEngine, Map& map, const D3DXVECTOR3& playerPos, D3DXVECTOR3* m_outPatrollerPosition, float deltaTime)
{
	bool bAlert = false;
	pEngine->LoopBGM();

	for (auto& patroller : m_patrollers)
	{
		//ゲームオーバーになれば他を調べず終了する
		if(patroller->Update(pEngine, map, playerPos, m_patrollers, m_outPatrollerPosition, deltaTime)) return true;

		if (patroller->IsAlert()) bAlert = true;
	}

	if (m_sceneNumber == Common::SCENE_GAME) {
		//敵の状態によってBGMを切り替える
		if (!m_bAlert && bAlert)
		{
			SoundManager::SetSwitch(AK::SWITCHES::GAME_BGM::GROUP, AK::SWITCHES::GAME_BGM::SWITCH::CHASE, ID_BGM);
			m_bAlert = bAlert;
		}
		else if (m_bAlert && !bAlert)
		{
			SoundManager::SetSwitch(AK::SWITCHES::GAME_BGM::GROUP, AK::SWITCHES::GAME_BGM::SWITCH::SEARCH, ID_BGM);
			m_bAlert = bAlert;
		}
	}

	return false;
}
void PatrollerManager::Draw(Engine* pEngine, Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight)
{
	for (const auto& patroller : m_patrollers)
	{
		patroller->Draw(pEngine, pCamera, pProj, pAmbient, pLight);
	}
}

void PatrollerManager::DebugViewLine(Engine * pEngine, Camera * pCamera, Projection * pProj)
{
	for (const auto& patroller : m_patrollers) 
	{
		patroller->DebugViewLine(pEngine, pCamera, pProj);
	}
}

void PatrollerManager::DebugPrint(Engine * pEngine)
{
}

//心音（敵が近くなるほど早くなる）
void PatrollerManager::DrawHeartBeat(Engine* pEngine, const D3DXVECTOR3& playerPos)
{
	float chaseDistanceSq = f_fileData.chaseDistance * f_fileData.chaseDistance;
	float gameOverDistanceSq = f_fileData.gameOverDistance * f_fileData.gameOverDistance;

	float minPlayerDistanceSq = FLT_MAX;
	for (const auto& patroller : m_patrollers) 
	{
		const D3DXVECTOR3& patrollerPos = patroller->GetPosition();
		D3DXVECTOR3 toPlayer = playerPos - patrollerPos;
		float distSq = D3DXVec3LengthSq(&toPlayer);
		if (distSq < minPlayerDistanceSq) 
		{
			minPlayerDistanceSq = distSq;
		}
	}

	//敵が一定距離より遠い場合は描画しない
	if (minPlayerDistanceSq > chaseDistanceSq) 
	{
		m_heartbeatAlpha = 0.0f;
		m_beatTime = 0.0f;
		return;
	}

	//敵の距離に基づいて心音の速度と最大アルファ値を決定する
	//距離がf_gameOverDistanceに近いほど0、f_chaseDistanceに近いほど1になるように補間
	float normalizedDistance = (minPlayerDistanceSq - gameOverDistanceSq) / (chaseDistanceSq - gameOverDistanceSq);
	if (normalizedDistance < 0) normalizedDistance = 0;
	if (normalizedDistance > 1) normalizedDistance = 1;

	// 距離が近いほどアルファ値の変化が速くなる
	float alphaSpeed = f_fileData.heartbeatAlphaSpeedScale * (1.0f - normalizedDistance) + f_fileData.heartbeatAlphaSpeedBase;

	// 距離が近いほど最大アルファ値が濃くなる
	float maxAlpha = f_fileData.heartbeatMaxAlphaScale * (1.0f - normalizedDistance) + f_fileData.heartbeatMaxAlphaBase;

	m_beatTime += alphaSpeed;

	float targetAlpha = (sinf(m_beatTime) + 1.0f) / 2.0f * maxAlpha;

	//アルファ値の急激な変化を避けるために線形補間
	m_heartbeatAlpha = m_heartbeatAlpha + (targetAlpha - m_heartbeatAlpha) * f_fileData.heartbeatLerpFactor;

	RECT fullScreenRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
	int alphaValue = (int)(m_heartbeatAlpha * 255);
	pEngine->Blt(&fullScreenRect, TEXTURE_HEARTBEAT, &fullScreenRect, (int)alphaValue, 0.0f);
}

const int PatrollerManager::GetAlertCount() const
{
	int totalCount = 0;

	for (const auto& patroller : m_patrollers)
	{
		totalCount += patroller->GetAlertCount();
	}

	return totalCount;
}

const bool PatrollerManager::IsViewPlayer() const
{
	for (const auto& patroller : m_patrollers) 
	{
		if (patroller->IsViewPlayer()) return true;
	}

	return false;
}

vector<SpotLight>* PatrollerManager::GetLights(const D3DXVECTOR3& playerPos, const D3DXVECTOR3& playerDir, const float fov)
{
	vector<pair<SpotLight*, float>> visibleLights;

	//視界用計算
	const float cosFov = cosf(fov / 2.0f + f_fileData.fovMargin);

	for (auto& patroller : m_patrollers) 
	{

		//敵が持つライトの情報を取得
		SpotLight* light = patroller->GetLight();

		//ライトが届く最大距離を取得
		const float lightRange = light->GetRange();

		D3DXVECTOR3 enemyVec = patroller->GetPosition() - playerPos;

		float distSq = D3DXVec3LengthSq(&enemyVec);

		D3DXVECTOR3 enemyDir;
		D3DXVec3Normalize(&enemyDir, &enemyVec);

		//プレイヤーの少し広く取った視界内にいるか敵の視界距離内にいるかを調べる
		if ((D3DXVec3Dot(&playerDir, &enemyDir) > cosFov) || (distSq < lightRange * lightRange))
		{
			visibleLights.push_back({ light, distSq });
		}
	}

	sort(visibleLights.begin(), visibleLights.end(), [](const pair<SpotLight*, float>& a, const pair<SpotLight*, float>& b) 
	{
		return a.second < b.second;
	});


	//以前の描画用ライトリストをクリア
	m_lights.clear();

	//並び替えたリストから、最大4個のライトを選び出す
	for (const auto& visibleLight : visibleLights) 
	{
		if ((int)m_lights.size() >= f_fileData.maxLights)break;
		//描画用リストにライトを追加
		m_lights.push_back(*visibleLight.first);
	}

	//描画用ライトリストへのポインタを返す
	return &m_lights;
}

//ミニマップの範囲内の敵を渡す
vector<D3DXVECTOR2> PatrollerManager::GetMiniMapPositions(const D3DXVECTOR2& playerPos, float mapSize)
{
	vector<D3DXVECTOR2> positions;
	for (const auto& patroller : m_patrollers)
	{
		D3DXVECTOR2 patrollerPos = D3DXVECTOR2(patroller->GetPosition().x, patroller->GetPosition().z);
		//プレイヤーの位置からの相対距離がミニマップの範囲内かチェック
		D3DXVECTOR2 toPlayer = playerPos - patrollerPos;
		if (D3DXVec2Length(&toPlayer) < mapSize) 
		{
			positions.push_back(patrollerPos);
		}
	}
	return positions;
}

void PatrollerManager::LoadParameter()
{
	ifstream file(JSON_PATROLLER_PARAMETER);
	if (!file.is_open())
	{
		throw DxSystemException(DxSystemException::OM_FILE_OPEN_ERROR);
	}
	nlohmann::json config;
	file >> config;
	file.close();

	f_fileData.angleTolerance = D3DXToRadian(config["angleTolerance"]);
	f_fileData.arriveDistance = config["arriveDistance"];
	f_fileData.searchSpeed = config["searchSpeed"];
	f_fileData.chaseSpeed = config["chaseSpeed"];
	f_fileData.turnSpeed = D3DXToRadian(config["turnSpeed"]);
	f_fileData.radius = config["radius"];
	f_fileData.eyePositionY = config["eyePositionY"];
	f_fileData.gameOverDistance = config["gameOverDistance"];

	f_fileData.obstructionValue = config["obstructionValue"];
	f_fileData.occlusionValue = config["occlusionValue"];
	f_fileData.distFactorMultiplier = config["distFactorMultiplier"];

	f_fileData.maxLights = config["maxLights"];
	f_fileData.fovMargin = config["fovMargin"];
	f_fileData.radius = config["radius"];
	f_fileData.searchLookAngle = D3DXToRadian(config["searchLookAngle"]);
	f_fileData.searchMaxAngle = D3DXToRadian(config["searchMaxAngle"]);
	f_fileData.patrolDistance = config["patrolDistance"];
	f_fileData.chaseDistance = config["chaseDistance"];

	f_fileData.heartbeatAlphaSpeedBase = config["heartbeatAlphaSpeedBase"];
	f_fileData.heartbeatAlphaSpeedScale = config["heartbeatAlphaSpeedScale"];
	f_fileData.heartbeatMaxAlphaBase = config["heartbeatMaxAlphaBase"];
	f_fileData.heartbeatMaxAlphaScale = config["heartbeatMaxAlphaScale"];
	f_fileData.heartbeatLerpFactor = config["heartbeatLerpFactor"];
	f_fileData.searchMaxCount = config["searchMaxCount"];

	float value0 = config["lightDiffuse"][0];
	float value1 = config["lightDiffuse"][1];
	float value2 = config["lightDiffuse"][2];
	float value3 = config["lightDiffuse"][3];
	f_fileData.baseLight.SetDiffuse(value0, value1, value2, value3);

	value0 = config["lightAttenuation"][0];
	value1 = config["lightAttenuation"][1];
	value2 = config["lightAttenuation"][2];
	f_fileData.baseLight.SetAttenuation(value0, value1, value2);

	value0 = config["lightCone"][0];
	f_fileData.theta = D3DXToRadian(config["lightCone"][1]);
	f_fileData.phi = D3DXToRadian(config["lightCone"][2]);
	f_fileData.baseLight.SetCone(value0, value1, f_fileData.phi);
	f_fileData.baseLight.SetRange(f_fileData.patrolDistance);
}
