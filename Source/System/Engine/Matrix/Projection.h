//*****************************************************************************
//
// プロジェクションクラス
//
// Projection.h
//
// K_Yamaoka
//
// 2013/03/04
//
//*****************************************************************************

#pragma once

#include <d3dx9.h>

#include "..\\Engine.h"

class Projection
{
public:

	//=============================================================================
	// コンストラクタ
	//=============================================================================
	Projection();

	//=============================================================================
	// コンストラクタ
	// 引　数：const float 視野角（ラジアン角）
	// 　　　　const float アスペクト比（４：３なら 4.0 / 3.0 ）
	// 　　　　const float 前方クリップ面
	// 　　　　const float 後方クリップ面
	//=============================================================================
	Projection(const float fov, const float aspect, const float nearClip, const float farClip);

	//=============================================================================
	// データの設定
	// 引　数：const float 視野角（ラジアン角）
	// 　　　　const float アスペクト比（４：３なら 4.0 / 3.0 ）
	// 　　　　const float 前方クリップ面
	// 　　　　const float 後方クリップ面
	//=============================================================================
	void SetData(const float fov, const float aspect, const float nearClip, const float farClip);

	//=============================================================================
	// プロジェクション変換行列の取得
	// 戻り値：D3DXMATRIX プロジェクション変換行列
	//=============================================================================
	D3DXMATRIX GetProjectionMatrix() const;

	//=============================================================================
	// デバイスへセット
	// 引　数：Engine* ３Ｄエンジンクラスへのポインタ
	//=============================================================================
	void SetDevice(Engine* pEngine);

	void SetOrthoDevice(Engine* pEngine, const float width, const float height);

	//=============================================================================
	// 変換行列の作成
	// 備　考：シャドウマップ用
	//=============================================================================
	void CreateMatrix();

	void SetFov(const float fov);

	float GetFov() const;

	void SetOrtho(const float width, const float height, const float nearClip, const float farClip);
private:

	//視野角
	float m_fov;

	//アスペクト比
	float m_aspect;

	//前方クリップ面
	float m_nearClip;

	//後方クリップ面
	float m_farClip;

	//プロジェクション変換行列
	D3DXMATRIX m_matProjection;
};












