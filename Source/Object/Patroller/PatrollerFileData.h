#pragma once

struct PatrollerFileData 
{
	//個々の敵に必要な設定
	int searchMaxCount;
	float searchSpeed;
	float chaseSpeed;
	float turnSpeed;
	float arriveDistance;
	float angleTolerance;
	float eyePositionY;
	float radius;
	float theta;
	float phi;
	float searchLookAngle;
	float searchMaxAngle;
	float gameOverDistance;
	float obstructionValue;
	float occlusionValue;
	float distFactorMultiplier;
	float heartbeatAlphaSpeedBase;
	float heartbeatAlphaSpeedScale;
	float heartbeatMaxAlphaBase;
	float heartbeatMaxAlphaScale;
	float heartbeatLerpFactor;
	SpotLight baseLight;

	//Managerに必要な設定
	int maxLights;
	float fovMargin;
	float patrolDistance;
	float chaseDistance;
};		  