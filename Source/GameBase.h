//*****************************************************************************
//
// ゲーム全体で使用するリソース、構造体などの宣言
//
// GameBase.h
//
// K_Yamaoka
//
// 2013/03/04
//
//*****************************************************************************

#pragma once

//-----------------------------------------------------------------------------
//システム関係のヘッダファイルです。変更しないでください。
//-----------------------------------------------------------------------------

#include "System\\Engine\\Matrix\\Camera.h"
#include "System\\Engine\\Matrix\\Projection.h"
#include "System\\Engine\\Matrix\\Viewport.h"

#include "System\\Engine\\Light\\Ambient.h"
#include "System\\Engine\\Light\\Directional.h"
#include "System\\Engine\\Light\\Point.h"
#include "System\\Engine\\Light\\Spot.h"

#include "System\\Engine\\Mesh\\Primitive.h"
#include "System\\Engine\\Billboard\\Billboard.h"
#include "System\\Engine\\Particle\\ParticleManage.h"

#include "System\\Engine\\Collision\\Collision.h"
#include "System\\Engine\\Collision\\BoundingBox.h"
#include "System\\Engine\\Collision\\BoundingSphere.h"

#include "System\\Engine\\Mesh\\Model.h"

#include "System\\Engine\\Engine.h"

#include "Object\\SoundManager\\SoundManager.h"
#include "Object\\IntVector2\\IntVector2.h"

//-----------------------------------------------------------------------------
//Ｃ言語でのdefine、構造体などはこちらで宣言します。
//-----------------------------------------------------------------------------

static constexpr AkGameObjectID ID_BGM = 2;
static constexpr AkGameObjectID ID_UI = 3;
static constexpr AkGameObjectID ID_PALYER = 4;
static constexpr AkGameObjectID ID_PATROLLER_START = 100;

//-----------------------------------------------------------------------------
//ゲーム中で使用するテクスチャ、BGM、SE、フォントのパス付ファイル名を書きます。
//-----------------------------------------------------------------------------
namespace KeyString
{
	constexpr char* FONT_GOTHIC40 = "40:ＭＳ ゴシック";
	constexpr char* FONT_GOTHIC60 = "60:ＭＳ ゴシック";
	constexpr char* FONT_GOTHIC160 = "160:ＭＳ ゴシック";
	constexpr char* FONT_GOTHIC120 = "120:ＭＳ ゴシック";

	constexpr char* MAP_FILE_PATH = "Resource\\MapData\\";
	constexpr char* MAP_FILE_END = ".txt";
	constexpr char* MODEL_CHARACTER = "Resource\\Model\\Character\\Character.x";

	constexpr char* TEXTURE_MINI_MAP = "Resource\\Texture\\MiniMap.png";
	constexpr char* TEXTURE_NORMAL_CIRCLE = "Resource\\Texture\\NormalCircle.png";
	constexpr char* TEXTURE_CHASE_CIRCLE = "Resource\\Texture\\ChaseCircle.png";
	constexpr char* TEXTURE_ENEMY_PIN = "Resource\\Texture\\EnemyPin.png";
	constexpr char* TEXTURE_PLAYER_PIN = "Resource\\Texture\\PlayerPin.png";
	constexpr char* TEXTURE_GOAL_PIN = "Resource\\Texture\\GoalPin.png";
	constexpr char* TEXTURE_HEARTBEAT = "Resource\\Texture\\Heartbeat.png";
	constexpr char* TEXTURE_BUTTON = "Resource\\Texture\\Button.png";
	constexpr char* TEXTURE_OPERATION = "Resource\\Texture\\Operation.png";
	constexpr char* TEXTURE_PAUSE = "Resource\\Texture\\Pause.png";
	constexpr char* TEXTURE_EFFECT = "Resource\\Texture\\Effect.png";
	constexpr char* TEXTURE_FADE = "Resource\\Texture\\Fade.png";
	constexpr char* TEXTURE_STAMINA_GAUGE = "Resource\\Texture\\StaminaGauge.png";
	
	constexpr char* JSON_PLAYER_PARAMETER = "Resource\\Parameter\\Player.json";
	constexpr char* JSON_MAP_PARAMETER = "Resource\\Parameter\\Map.json";
	constexpr char* JSON_PATROLLER_PARAMETER = "Resource\\Parameter\\Patroller.json";
	constexpr char* JSON_MENU_PARAMETER = "Resource\\Parameter\\Menu.json";
	constexpr char* JSON_GOAL_EFFECT_PARAMETER = "Resource\\Parameter\\GoalEffect.json";
	constexpr char* JSON_FADE_PARAMETER = "Resource\\Parameter\\Fade.json";
}
