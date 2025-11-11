//*****************************************************************************
//
// コリジョン判定クラス
//
// Collision.h
//
// K_Yamaoka
//
// 2013/03/04
//
// 2013/06/11 ColideBBAndRayOfMousePointer, ColideBSAndRayOfMousePointerを追加
//
// 2014/10/17 綴り修正　Colide　→　Collide
//
// 2015/01/06 CollideBBAndRay、CollideBSAndRay関数の追加
//
// 2015/01/07 CollideBBAndLine、CollideBSAndLine関数の追加
//
// 2018/12/17 GetCollideDistanceBBAndLine関数を追加
//
// 2019/01/18 CollideBBAndBS関数を追加
//
//*****************************************************************************

#pragma once

#include <d3dx9.h>

#include "BoundingBox.h"
#include "BoundingSphere.h"

#include "..\\..\\Utility\\Vector2D.h"
#include "..\\..\\Utility\\Vector3D.h"

//２Ｄ用ＯＢＢデータ構造体
struct ObbData2D {
	D3DXVECTOR2 m_position;
	int m_width;
	int m_height;
	float m_angle;
};

class Collision
{
public:

	//=============================================================================
	// 衝突判定　バウンディングボックス　ＡＡＢＢ( Axis Align Bounding Box )
	// 戻り値：衝突している場合 true、していない場合 false
	// 引　数：const BoundingBox*　バウンディングボックス１へのポインタ
	// 　　　　const BoundingBox*　バウンディングボックス２へのポインタ
	//=============================================================================
	static bool CollideAABB(const BoundingBox* pBoundingBox1, const BoundingBox* pBoundingBox2);

	//=============================================================================
	// 衝突判定　バウンディングボックス　ＯＢＢ( Oriented Bounding Box )
	// 戻り値：衝突している場合 true、していない場合 false
	// 引　数：BoundingBox*　バウンディングボックス１へのポインタ
	// 　　　　BoundingBox*　バウンディングボックス２へのポインタ
	//=============================================================================
	static bool CollideOBB(BoundingBox* pBoundingBox1, BoundingBox* pBoundingBox2);

	//=============================================================================
	// 衝突判定　バウンディングボックスとレイ
	// 戻り値：衝突している場合 true、していない場合 false
	// 引　数：const D3DXVECTOR3*  レイの開始点へのポインタ
	// 　　　　const D3DXVECTOR3*  レイの方向へのポインタ
	// 　　　　BoundingBox*      　バウンディングボックスへのポインタ
	//=============================================================================
	static bool CollideBBAndRay(const D3DXVECTOR3* pVecPos, const D3DXVECTOR3* pVecRay, BoundingBox* pBoundingBox);

	//=============================================================================
	// 衝突判定　バウンディングボックスと線分
	// 戻り値：衝突している場合 true、していない場合 false
	// 引　数：const D3DXVECTOR3*  レイの開始点へのポインタ
	// 　　　　const D3DXVECTOR3*  レイの方向へのポインタ
	// 　　　　const float　　　　 開始点からの距離
	// 　　　　BoundingBox*      　バウンディングボックスへのポインタ
	//=============================================================================
	static bool CollideBBAndLine(const D3DXVECTOR3* pVecPos, const D3DXVECTOR3* pVecRay, const float distance, BoundingBox* pBoundingBox);
	
	//=============================================================================
	// 衝突判定　バウンディングボックスと線分の交点までの距離を取得
	// 戻り値：衝突場所までの距離
	// 引　数：const D3DXVECTOR3*  レイの開始点へのポインタ
	// 　　　　const D3DXVECTOR3*  レイの方向へのポインタ
	// 　　　　const float　　　　 開始点からの線分の長さ
	// 　　　　BoundingBox*      　バウンディングボックスへのポインタ
	// 備　考：衝突していない場合は引数lengthで渡した距離が戻される
	//=============================================================================
	static float GetCollideDistanceBBAndLine(const D3DXVECTOR3* pVecPos, const D3DXVECTOR3* pVecRay, const float length, BoundingBox* pBoundingBox);

	//=============================================================================
	// 衝突判定　バウンディングボックスとマウスポインタ
	// 戻り値：衝突している場合 true、していない場合 false
	// 引　数：const POINT*     　 マウス座標へのポインタ
	// 　　　　BoundingBox*      　バウンディングボックスへのポインタ
	// 　　　　const D3DXMATRIX*   ビュー変換行列へのポインタ
	// 　　　　const D3DXMATRIX*   プロジェクション変換行列へのポインタ
	// 　　　　const D3DVIEWPORT9* ビューポートへのポインタ
	// 　　　　const D3DXVECTOR3*  カメラ位置へのポインタ
	//=============================================================================
	static bool CollideBBAndRayOfMousePointer(const POINT* pPoint, BoundingBox* pBoundingBox, const D3DXMATRIX* pView, const D3DXMATRIX* pProjection, const D3DVIEWPORT9* pViewport, const D3DXVECTOR3* pCameraPosition);

	//=============================================================================
	// 衝突判定　バウンディングスフィア（球体）
	// 戻り値：衝突している場合 true、していない場合 false
	// 引　数：const BoundingSphere*　バウンディングスフィア１へのポインタ
	// 　　　　const BoundingSphere*　バウンディングスフィア２へのポインタ
	//=============================================================================
	static bool CollideBS(const BoundingSphere* pBoundingSphere1, const BoundingSphere* pBoundingSphere2);

	//=============================================================================
	// 衝突判定　バウンディングスフィアとレイ
	// 戻り値：衝突している場合 true、していない場合 false
	// 引　数：const D3DXVECTOR3*  レイの開始点へのポインタ
	// 　　　　const D3DXVECTOR3*  レイの方向へのポインタ
	// 　　　　BoundingSphere*     バウンディングスフィアへのポインタ
	//=============================================================================
	static bool CollideBSAndRay(const D3DXVECTOR3* pVecPos, const D3DXVECTOR3* pVecRay, BoundingSphere* pBoundingSphere);

	//=============================================================================
	// 衝突判定　バウンディングスフィアと線分
	// 戻り値：衝突している場合 true、していない場合 false
	// 引　数：const D3DXVECTOR3*  レイの開始点へのポインタ
	// 　　　　const D3DXVECTOR3*  レイの方向へのポインタ
	// 　　　　const float　　　　 開始点からの線分の長さ
	// 　　　　BoundingSphere*     バウンディングスフィアへのポインタ
	//=============================================================================
	static bool CollideBSAndLine(const D3DXVECTOR3* pVecPos, const D3DXVECTOR3* pVecRay, const float length, BoundingSphere* pBoundingSphere);

	//=============================================================================
	// 衝突判定　バウンディングスフィアとマウスポインタ
	// 戻り値：衝突している場合 true、していない場合 false
	// 引　数：const POINT*     　 マウス座標へのポインタ
	// 　　　　BoundingSphere*   　バウンディングスフィアへのポインタ
	// 　　　　const D3DXMATRIX*   ビュー変換行列へのポインタ
	// 　　　　const D3DXMATRIX*   プロジェクション変換行列へのポインタ
	// 　　　　const D3DVIEWPORT9* ビューポートへのポインタ
	// 　　　　const D3DXVECTOR3*  カメラ位置へのポインタ
	//=============================================================================
	static bool CollideBSAndRayOfMousePointer(const POINT* pPoint, BoundingSphere* pBoundingSphere, const D3DXMATRIX* pView, const D3DXMATRIX* pProjection, const D3DVIEWPORT9* pViewport, const D3DXVECTOR3* pCameraPosition);

	//=============================================================================
	// 衝突判定　バウンディングボックスとバウンディングスフィア
	// 戻り値：衝突している場合 true、していない場合 false
	// 引　数：BoundingBox*      　バウンディングボックスへのポインタ
	// 　　　　BoundingSphere*   　バウンディングスフィアへのポインタ
	//=============================================================================
	static bool CollideBBAndBS(BoundingBox* pBoundingBox, BoundingSphere* pBoundingSphere);

	//=============================================================================
	// 衝突判定　レイとポリゴンの交点を求める
	// 戻り値：交差している場合 true、していない場合 false
	// 引　数：const D3DXVECTOR3* レイの発射点へのポインタ
	// 　　　　const D3DXVECTOR3* レイの方向へのポインタ（単位ベクトル）
	// 　　　　const D3DXVECTOR3* ポリゴンの法線へのポインタ
	// 　　　　const D3DXVECTOR3  ポリゴンの頂点配列
	// 　　　　D3DXVECTOR3*       交点があった場合、交点の座標を格納する変数へのポインタ
	//         bool               レイの始点を無視する（地形データなど例外的に使用）
	// 備　考：表ポリゴンのみ判定を行う。
	//=============================================================================
	static bool CheckCrossPoint(const D3DXVECTOR3* pVecPos, const D3DXVECTOR3* pVecRay, const D3DXVECTOR3* pVecNormal, const D3DXVECTOR3 vPolygon[3], D3DXVECTOR3* pVecCrossPoint, bool bIgnoreStartPos = false);

	//=============================================================================
	// 衝突判定　２Ｄ用ＯＢＢ
	// 戻り値：交差している場合 true、していない場合 false
	// 引　数：const ObbData2D & ２Ｄ用ＯＢＢデータ構造体の参照
	// 　　　　const ObbData2D & ２Ｄ用ＯＢＢデータ構造体の参照
	//=============================================================================
	static bool CollideOBB2D(const ObbData2D &obb1, const ObbData2D &obb2);

private:

	//-----------------------------------------------------------------------------
	// ＯＢＢ用分離軸比較関数
	// 戻り値：衝突している場合 true、していない場合 false
	// 引　数：const BB_ObbData*　ＯＢＢデータ１へのポインタ
	// 　　　　const BB_ObbData*　ＯＢＢデータ２へのポインタ
	// 　　　　const D3DXVECTOR3* 射影する軸ベクトルへのポインタ（単位ベクトル）
	// 　　　　const D3DXVECTOR3* ２つのバウンディングボックスの中心を結ぶ距離へのポインタ
	//-----------------------------------------------------------------------------
	static bool CompareLength(const BB_ObbData* pObb1, const BB_ObbData* pObb2, const D3DXVECTOR3* pVecAxis, const D3DXVECTOR3* pVecDistance);

	//-----------------------------------------------------------------------------
	// ＯＢＢ用分離軸比較関数（２Ｄ版）
	// 戻り値：衝突している場合 true、していない場合 false
	// 引　数：D3DXVECTOR2 &　２つのＯＢＢデータの中心から中心へのベクトルの参照
	// 　　　　D3DXVECTOR2 &　射影する軸ベクトルの参照
	// 　　　　D3DXVECTOR2 &　１つ目の水平方向の半径ベクトルの参照（回転済み）
	// 　　　　D3DXVECTOR2 &　１つ目の垂直方向の半径ベクトルの参照（回転済み）
	// 　　　　D3DXVECTOR2 &　２つ目の水平方向の半径ベクトルの参照（回転済み）
	// 　　　　D3DXVECTOR2 &　２つ目の垂直方向の半径ベクトルの参照（回転済み）
	//-----------------------------------------------------------------------------
	static bool CompareLength2D(D3DXVECTOR2 &centerToCenter, D3DXVECTOR2 &projection, D3DXVECTOR2 &rotateHorizontalRadius1, D3DXVECTOR2 &rotateVerticalRadius1, D3DXVECTOR2 &rotateHorizontalRadius2, D3DXVECTOR2 &rotateVerticalRadius2);
};















