// IceBlock.h - 溶ける氷ブロッククラス（球体版）
#pragma once

#include "../../GameBase.h"
#include "..\\System\\Engine\\Mesh\\Primitive.h"
#include <d3dx9.h>

class IceBlock
{
public:
	IceBlock();
	~IceBlock();

	// 初期化
	// width, height, depth: 氷のサイズ
	// position: 初期位置
	// meltDuration: 完全に溶けるまでの時間（秒）、0.0fで手動制御
	void Initialize(Engine* pEngine, float width, float height, float depth,
		const D3DXVECTOR3& position, float meltDuration = 0.0f);

	// 更新
	void Update(float deltaTime);

	// 描画
	void Draw(Engine* pEngine, Camera* pCamera, Projection* pProj,
		AmbientLight* pAmbient, DirectionalLight* pLight);

	// 溶け具合を設定（0.0f = 完全な氷、1.0f = 完全に溶けた）
	void SetMeltAmount(float amount);

	// 溶け具合を取得
	float GetMeltAmount() const { return m_meltAmount; }

	// 完全に溶けたか
	bool IsFullyMelted() const { return m_meltAmount >= 1.0f; }

	// 位置を設定
	void SetPosition(const D3DXVECTOR3& position);

	// 位置を取得
	const D3DXVECTOR3& GetPosition() const { return m_position; }

	// 回転を設定
	void SetRotation(float x, float y, float z);

	// スケールを設定
	void SetScale(float x, float y, float z);

	// 色を設定（氷の色調整用）
	void SetColor(const D3DXVECTOR4& color) { m_iceColor = color; }

	// ★★★ Engineポインタをpublicに（Runner::DrawEffectsから使用） ★★★
	Engine* m_pEngine;

private:
	void UpdateWorldMatrix();
	void ReleaseResources();

	Primitive m_primitiveSphere;  // ★★★ 球体メッシュのみ使用 ★★★

	D3DXVECTOR3 m_position;
	D3DXVECTOR3 m_rotation;
	D3DXVECTOR3 m_scale;
	D3DXVECTOR3 m_originalSize;  // 元のサイズを保持

	D3DXMATRIX m_matWorld;

	float m_meltAmount;           // 0.0f～1.0f
	float m_meltDuration;         // 溶けるまでの時間（秒）
	float m_elapsedTime;          // 経過時間

	D3DXVECTOR4 m_iceColor;       // 氷の色（デフォルトは青白い色）

	bool m_autoMelt;              // 自動で溶けるか
};