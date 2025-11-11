//*****************************************************************************
//
// コリジョン判定クラス
//
// Collision.cpp
//
// K_Yamaoka
//
// 2013/03/04
//
// 2013/06/11 ColideBBAndRayOfMousePointer, ColideBSAndRayOfMousePointerを追加
//
// 2013/09/10 CheckCrossPoint関数 裏ポリゴンとの判定を除去
//
// 2014/10/17 GetWorldTransform→GetWorldTransform　名称変更
//            SetWorldTransformMatrix→SetWorldTransform　名称変更
//
// 2014/10/17 綴り修正　Colide　→　Collide
//
// 2015/01/06 CheckCrossPointのバグ修正（レイの逆方向にあるポリゴンも判定していた）
//
// 2015/01/06 CollideBBAndRay、CollideBSAndRay関数の追加
//
// 2015/01/07 CollideBBAndLine、CollideBSAndLine関数の追加
//
// 2017/04/21 OBBの判定の際に回転を考慮していなかったバグを修正
//
// 2017/09/27 CheckCrossPoint関数の「レイの逆方向にあるポリゴンの場合、交点無しとする」コードを修正
//
// 2018/12/17 GetCollideDistanceBBAndLine関数を追加
//
// 2019/01/18 CollideBBAndBS関数を追加
//
// 2019/01/22 CollideBBAndBS関数の不具合を修正
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "Collision.h"

#include <cmath>

//=============================================================================
// 衝突判定　バウンディングボックス　ＡＡＢＢ( Axis Align Bounding Box )
// 戻り値：衝突している場合 true、していない場合 false
// 引　数：const BoundingBox*　バウンディングボックス１へのポインタ
// 　　　　const BoundingBox*　バウンディングボックス２へのポインタ
//=============================================================================
bool Collision::CollideAABB(const BoundingBox* pBoundingBox1, const BoundingBox* pBoundingBox2)
{
	//移動後のバウンディングボックスの座標を計算

	D3DXVECTOR3 vecMax1, vecMax2;
	D3DXVECTOR3 vecMin1, vecMin2;

	BB_VertexData vd1 = pBoundingBox1->GetVertexData();
	BB_VertexData vd2 = pBoundingBox2->GetVertexData();

	D3DXMATRIX mat1 = pBoundingBox1->GetWorldTransform();
	D3DXMATRIX mat2 = pBoundingBox2->GetWorldTransform();

	D3DXVec3TransformCoord(&vecMax1, &vd1.vecMax, &mat1);
	D3DXVec3TransformCoord(&vecMin1, &vd1.vecMin, &mat1);

	D3DXVec3TransformCoord(&vecMax2, &vd2.vecMax, &mat2);
	D3DXVec3TransformCoord(&vecMin2, &vd2.vecMin, &mat2);

	//X-Z平面での判定
	if (vecMax1.x > vecMin2.x && vecMin1.x < vecMax2.x && vecMax1.z > vecMin2.z && vecMin1.z < vecMax2.z) {

		//Y座標の判定
		if (vecMax1.y > vecMin2.y && vecMin1.y < vecMax2.y) {
			return true;
		}
	}

	return false;
}

//=============================================================================
// 衝突判定　バウンディングボックス　ＯＢＢ( Oriented Bounding Box )
// 戻り値：衝突している場合 true、していない場合 false
// 引　数：BoundingBox*　バウンディングボックス１へのポインタ
// 　　　　BoundingBox*　バウンディングボックス２へのポインタ
//=============================================================================
bool Collision::CollideOBB(BoundingBox* pBoundingBox1, BoundingBox* pBoundingBox2)
{
	//OBBデータの更新

	BB_ObbData* pObb1 = pBoundingBox1->GetObbData();
	BB_ObbData* pObb2 = pBoundingBox2->GetObbData();

	D3DXMATRIX mat1 = pBoundingBox1->GetWorldTransform();
	D3DXMATRIX mat2 = pBoundingBox2->GetWorldTransform();

	D3DXMATRIX matWork;
	D3DXVECTOR3 vecWork;

	matWork = mat1;
	matWork.m[3][0] = 0; matWork.m[3][1] = 0; matWork.m[3][2] = 0;

	vecWork = pObb1->vecAdjust;
	vecWork = D3DXVec3TransformCoord(&vecWork, &matWork);

	pObb1->vecCenter.x = mat1.m[3][0] + vecWork.x;
	pObb1->vecCenter.y = mat1.m[3][1] + vecWork.y;
	pObb1->vecCenter.z = mat1.m[3][2] + vecWork.z;

	pObb1->vecAxisX.x = mat1.m[0][0]; pObb1->vecAxisX.y = mat1.m[0][1]; pObb1->vecAxisX.z = mat1.m[0][2];
	pObb1->vecAxisY.x = mat1.m[1][0]; pObb1->vecAxisY.y = mat1.m[1][1]; pObb1->vecAxisY.z = mat1.m[1][2];
	pObb1->vecAxisZ.x = mat1.m[2][0]; pObb1->vecAxisZ.y = mat1.m[2][1]; pObb1->vecAxisZ.z = mat1.m[2][2];


	matWork = mat2;
	matWork.m[3][0] = 0; matWork.m[3][1] = 0; matWork.m[3][2] = 0;

	vecWork = pObb2->vecAdjust;
	vecWork = D3DXVec3TransformCoord(&vecWork, &matWork);

	pObb2->vecCenter.x = mat2.m[3][0] + vecWork.x;
	pObb2->vecCenter.y = mat2.m[3][1] + vecWork.y;
	pObb2->vecCenter.z = mat2.m[3][2] + vecWork.z;

	pObb2->vecAxisX.x = mat2.m[0][0]; pObb2->vecAxisX.y = mat2.m[0][1]; pObb2->vecAxisX.z = mat2.m[0][2];
	pObb2->vecAxisY.x = mat2.m[1][0]; pObb2->vecAxisY.y = mat2.m[1][1]; pObb2->vecAxisY.z = mat2.m[1][2];
	pObb2->vecAxisZ.x = mat2.m[2][0]; pObb2->vecAxisZ.y = mat2.m[2][1]; pObb2->vecAxisZ.z = mat2.m[2][2];

	//２つの物体の距離を計算
	D3DXVECTOR3 vecDistance = pObb2->vecCenter - pObb1->vecCenter;

	//分離軸チェック

	if (!CompareLength(pObb1, pObb2, &pObb1->vecAxisX, &vecDistance)) {
		return false;
	}

	if (!CompareLength(pObb1, pObb2, &pObb1->vecAxisY, &vecDistance)) {
		return false;
	}

	if (!CompareLength(pObb1, pObb2, &pObb1->vecAxisZ, &vecDistance)) {
		return false;
	}

	if (!CompareLength(pObb1, pObb2, &pObb2->vecAxisX, &vecDistance)) {
		return false;
	}

	if (!CompareLength(pObb1, pObb2, &pObb2->vecAxisY, &vecDistance)) {
		return false;
	}

	if (!CompareLength(pObb1, pObb2, &pObb2->vecAxisZ, &vecDistance)) {
		return false;
	}

	D3DXVECTOR3 vAxis;

	D3DXVec3Cross(&vAxis, &pObb1->vecAxisX, &pObb2->vecAxisX);
	if (!CompareLength(pObb1, pObb2, &vAxis, &vecDistance)) {
		return false;
	}

	D3DXVec3Cross(&vAxis, &pObb1->vecAxisX, &pObb2->vecAxisY);
	if (!CompareLength(pObb1, pObb2, &vAxis, &vecDistance)) {
		return false;
	}

	D3DXVec3Cross(&vAxis, &pObb1->vecAxisX, &pObb2->vecAxisZ);
	if (!CompareLength(pObb1, pObb2, &vAxis, &vecDistance)) {
		return false;
	}

	D3DXVec3Cross(&vAxis, &pObb1->vecAxisY, &pObb2->vecAxisX);
	if (!CompareLength(pObb1, pObb2, &vAxis, &vecDistance)) {
		return false;
	}

	D3DXVec3Cross(&vAxis, &pObb1->vecAxisY, &pObb2->vecAxisY);
	if (!CompareLength(pObb1, pObb2, &vAxis, &vecDistance)) {
		return false;
	}

	D3DXVec3Cross(&vAxis, &pObb1->vecAxisY, &pObb2->vecAxisZ);
	if (!CompareLength(pObb1, pObb2, &vAxis, &vecDistance)) {
		return false;
	}

	D3DXVec3Cross(&vAxis, &pObb1->vecAxisZ, &pObb2->vecAxisX);
	if (!CompareLength(pObb1, pObb2, &vAxis, &vecDistance)) {
		return false;
	}

	D3DXVec3Cross(&vAxis, &pObb1->vecAxisZ, &pObb2->vecAxisY);
	if (!CompareLength(pObb1, pObb2, &vAxis, &vecDistance)) {
		return false;
	}

	D3DXVec3Cross(&vAxis, &pObb1->vecAxisZ, &pObb2->vecAxisZ);
	if (!CompareLength(pObb1, pObb2, &vAxis, &vecDistance)) {
		return false;
	}

	return true;
}

//=============================================================================
// 衝突判定　バウンディングボックスとレイ
// 戻り値：衝突している場合 true、していない場合 false
// 引　数：const D3DXVECTOR3*  レイの開始点へのポインタ
// 　　　　const D3DXVECTOR3*  レイの方向へのポインタ
// 　　　　BoundingBox*      　バウンディングボックスへのポインタ
//=============================================================================
bool Collision::CollideBBAndRay(const D3DXVECTOR3* pVecPos, const D3DXVECTOR3* pVecRay, BoundingBox* pBoundingBox)
{
	BB_Polygon polygon;

	D3DXVECTOR3 vecNormal;

	D3DXMATRIX matWorld = pBoundingBox->GetWorldTransform();

	for (int i = 0; i < BoundingBox::POLYGON_NUM; i++) {

		polygon = pBoundingBox->GetPolygon(i);

		polygon.vertex[0] = D3DXVec3TransformCoord(&polygon.vertex[0], &matWorld);
		polygon.vertex[1] = D3DXVec3TransformCoord(&polygon.vertex[1], &matWorld);
		polygon.vertex[2] = D3DXVec3TransformCoord(&polygon.vertex[2], &matWorld);

		vecNormal = D3DXComputeNormal(polygon.vertex);

		if (Collision::CheckCrossPoint(pVecPos, pVecRay, &vecNormal, polygon.vertex, nullptr)) {
			return true;
		}
	}

	return false;
}

//=============================================================================
// 衝突判定　バウンディングボックスと線分
// 戻り値：衝突している場合 true、していない場合 false
// 引　数：const D3DXVECTOR3*  レイの開始点へのポインタ
// 　　　　const D3DXVECTOR3*  レイの方向へのポインタ
// 　　　　const float　　　　 開始点からの線分の長さ
// 　　　　BoundingBox*      　バウンディングボックスへのポインタ
//=============================================================================
bool Collision::CollideBBAndLine(const D3DXVECTOR3* pVecPos, const D3DXVECTOR3* pVecRay, const float length, BoundingBox* pBoundingBox)
{
	BB_Polygon polygon;

	D3DXVECTOR3 vecNormal;

	D3DXMATRIX matWorld = pBoundingBox->GetWorldTransform();

	for (int i = 0; i < BoundingBox::POLYGON_NUM; i++) {

		polygon = pBoundingBox->GetPolygon(i);

		polygon.vertex[0] = D3DXVec3TransformCoord(&polygon.vertex[0], &matWorld);
		polygon.vertex[1] = D3DXVec3TransformCoord(&polygon.vertex[1], &matWorld);
		polygon.vertex[2] = D3DXVec3TransformCoord(&polygon.vertex[2], &matWorld);

		vecNormal = D3DXComputeNormal(polygon.vertex);

		D3DXVECTOR3 vecCrossPoint;

		if (Collision::CheckCrossPoint(pVecPos, pVecRay, &vecNormal, polygon.vertex, &vecCrossPoint)) {

			D3DXVECTOR3 vecPosToCrossPoint = vecCrossPoint - *pVecPos;

			float distance = D3DXVec3Length(&vecPosToCrossPoint);

			if (length > distance) {
				return true;
			}
		}
	}

	return false;
}

//=============================================================================
// 衝突判定　バウンディングボックスと線分の交点までの距離を取得
// 戻り値：衝突場所までの距離
// 引　数：const D3DXVECTOR3*  レイの開始点へのポインタ
// 　　　　const D3DXVECTOR3*  レイの方向へのポインタ
// 　　　　const float　　　　 開始点からの線分の長さ
// 　　　　BoundingBox*      　バウンディングボックスへのポインタ
// 備　考：衝突していない場合は引数lengthで渡した距離が戻される
//=============================================================================
float Collision::GetCollideDistanceBBAndLine(const D3DXVECTOR3* pVecPos, const D3DXVECTOR3* pVecRay, const float length, BoundingBox* pBoundingBox)
{
	BB_Polygon polygon;

	D3DXVECTOR3 vecNormal;

	D3DXMATRIX matWorld = pBoundingBox->GetWorldTransform();

	float returnValue = length;

	for (int i = 0; i < BoundingBox::POLYGON_NUM; i++) {

		polygon = pBoundingBox->GetPolygon(i);

		polygon.vertex[0] = D3DXVec3TransformCoord(&polygon.vertex[0], &matWorld);
		polygon.vertex[1] = D3DXVec3TransformCoord(&polygon.vertex[1], &matWorld);
		polygon.vertex[2] = D3DXVec3TransformCoord(&polygon.vertex[2], &matWorld);

		vecNormal = D3DXComputeNormal(polygon.vertex);

		D3DXVECTOR3 vecCrossPoint;

		if (Collision::CheckCrossPoint(pVecPos, pVecRay, &vecNormal, polygon.vertex, &vecCrossPoint)) {

			D3DXVECTOR3 vecPosToCrossPoint = vecCrossPoint - *pVecPos;

			float distance = D3DXVec3Length(&vecPosToCrossPoint);

			returnValue = distance;

			break;
		}
	}

	return returnValue;
}

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
bool Collision::CollideBBAndRayOfMousePointer(const POINT* pPoint, BoundingBox* pBoundingBox, const D3DXMATRIX* pView, const D3DXMATRIX* pProjection, const D3DVIEWPORT9* pViewport, const D3DXVECTOR3* pCameraPosition)
{
	D3DXVECTOR3 vecRay;
	D3DXVECTOR3 vecMouse;

	vecMouse.x = (float)pPoint->x;
	vecMouse.y = (float)pPoint->y;
	vecMouse.z = 0;

	D3DXMATRIX matWorld;

	D3DXMatrixIdentity(&matWorld);

	D3DXVec3Unproject(&vecRay, &vecMouse, pViewport, pProjection, pView, &matWorld);

	vecRay -= *pCameraPosition;

	vecRay = D3DXVec3Normalize(&vecRay);

	BB_Polygon polygon;

	D3DXVECTOR3 vecNormal;

	matWorld = pBoundingBox->GetWorldTransform();

	for (int i = 0; i < BoundingBox::POLYGON_NUM; i++) {

		polygon = pBoundingBox->GetPolygon(i);

		polygon.vertex[0] = D3DXVec3TransformCoord(&polygon.vertex[0], &matWorld);
		polygon.vertex[1] = D3DXVec3TransformCoord(&polygon.vertex[1], &matWorld);
		polygon.vertex[2] = D3DXVec3TransformCoord(&polygon.vertex[2], &matWorld);

		vecNormal = D3DXComputeNormal(polygon.vertex);

		if (Collision::CheckCrossPoint(pCameraPosition, &vecRay, &vecNormal, polygon.vertex, nullptr)) {
			return true;
		}
	}

	return false;
}


//=============================================================================
// 衝突判定　バウンディングスフィア（球体）
// 戻り値：衝突している場合 true、していない場合 false
// 引　数：const BoundingSphere*　バウンディングスフィア１へのポインタ
// 　　　　const BoundingSphere*　バウンディングスフィア２へのポインタ
//=============================================================================
bool Collision::CollideBS(const BoundingSphere* pBoundingSphere1, const BoundingSphere* pBoundingSphere2)
{
	float radius1 = pBoundingSphere1->GetRadius();
	float radius2 = pBoundingSphere2->GetRadius();

	D3DXMATRIX mat1 = pBoundingSphere1->GetWorldTransform();
	D3DXMATRIX mat2 = pBoundingSphere2->GetWorldTransform();

	D3DXVECTOR3 vecCenter1(mat1.m[3][0], mat1.m[3][1], mat1.m[3][2]);
	D3DXVECTOR3 vecCenter2(mat2.m[3][0], mat2.m[3][1], mat2.m[3][2]);

	D3DXVECTOR3 vecDistance = vecCenter1 - vecCenter2;

	float distance = D3DXVec3Length(&vecDistance);

	if (radius1 + radius2 > distance) {
		return true;
	}
	else {
		return false;
	}
}

//=============================================================================
// 衝突判定　バウンディングスフィアとレイ
// 戻り値：衝突している場合 true、していない場合 false
// 引　数：const D3DXVECTOR3*  レイの開始点へのポインタ
// 　　　　const D3DXVECTOR3*  レイの方向へのポインタ
// 　　　　BoundingSphere*     バウンディングスフィアへのポインタ
//=============================================================================
bool Collision::CollideBSAndRay(const D3DXVECTOR3* pVecPos, const D3DXVECTOR3* pVecRay, BoundingSphere* pBoundingSphere)
{
	D3DXVECTOR3 vecCenter;

	D3DXMATRIX matWorld = pBoundingSphere->GetWorldTransform();

	vecCenter.x = matWorld.m[3][0];
	vecCenter.y = matWorld.m[3][1];
	vecCenter.z = matWorld.m[3][2];

	float radius = pBoundingSphere->GetRadius();

	if (D3DXSphereBoundProbe(&vecCenter, radius, pVecPos, pVecRay)) {
		return true;
	}
	else {
		return false;
	}
}

//=============================================================================
// 衝突判定　バウンディングスフィアと線分
// 戻り値：衝突している場合 true、していない場合 false
// 引　数：const D3DXVECTOR3*  レイの開始点へのポインタ
// 　　　　const D3DXVECTOR3*  レイの方向へのポインタ
// 　　　　const float　　　　 開始点からの線分の長さ
// 　　　　BoundingSphere*     バウンディングスフィアへのポインタ
//=============================================================================
bool Collision::CollideBSAndLine(const D3DXVECTOR3* pVecPos, const D3DXVECTOR3* pVecRay, const float length, BoundingSphere* pBoundingSphere)
{
	D3DXMATRIX matWorld = pBoundingSphere->GetWorldTransform();

	D3DXVECTOR3 vecCenter;

	vecCenter.x = matWorld.m[3][0];
	vecCenter.y = matWorld.m[3][1];
	vecCenter.z = matWorld.m[3][2];

	float radiusSq = pBoundingSphere->GetRadius();
	radiusSq *= radiusSq;

	D3DXVECTOR3 distance = *pVecPos - vecCenter;

	float xc = D3DXVec3Dot(&distance, &distance);
	float vxc = D3DXVec3Dot(pVecRay, &distance);
	float D = vxc * vxc - xc + radiusSq;

	if (D < 0) {
		return false;
	}

	float tn = -vxc - sqrtf(D);
	float tp = -vxc + sqrtf(D);

	if (tn < 0) {
		return false;
	}
	else {
		if (length > tn) {
			return true;
		}
	}

	if (tp < 0) {
		return false;
	}
	else {
		if (length > tp) {
			return true;
		}
	}

	return false;
}

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
bool Collision::CollideBSAndRayOfMousePointer(const POINT* pPoint, BoundingSphere* pBoundingSphere, const D3DXMATRIX* pView, const D3DXMATRIX* pProjection, const D3DVIEWPORT9* pViewport, const D3DXVECTOR3* pCameraPosition)
{
	D3DXVECTOR3 vecRay;
	D3DXVECTOR3 vecMouse;

	vecMouse.x = (float)pPoint->x;
	vecMouse.y = (float)pPoint->y;
	vecMouse.z = 0;

	D3DXMATRIX matWorld;

	D3DXMatrixIdentity(&matWorld);

	D3DXVec3Unproject(&vecRay, &vecMouse, pViewport, pProjection, pView, &matWorld);

	vecRay -= *pCameraPosition;

	vecRay = D3DXVec3Normalize(&vecRay);

	float radius = pBoundingSphere->GetRadius();

	matWorld = pBoundingSphere->GetWorldTransform();

	D3DXVECTOR3 vecCenter(0, 0, 0);

	vecCenter = D3DXVec3TransformCoord(&vecCenter, &matWorld);

	D3DXVECTOR3 vecDirection = *pCameraPosition - vecCenter;

	float a = D3DXVec3Dot(&vecRay, &vecRay);
	float b = D3DXVec3Dot(&vecRay, &vecDirection);
	float c = D3DXVec3Dot(&vecDirection, &vecDirection) - radius * radius;

	float result = b * b - a * c;

	if (result < 0.0f) {
		return false;
	}

	// 衝突している！
	return true;
}

//=============================================================================
// 衝突判定　バウンディングボックスとバウンディングスフィア
// 戻り値：衝突している場合 true、していない場合 false
// 引　数：BoundingBox*      　バウンディングボックスへのポインタ
// 　　　　BoundingSphere*   　バウンディングスフィアへのポインタ
//=============================================================================
bool Collision::CollideBBAndBS(BoundingBox* pBoundingBox, BoundingSphere* pBoundingSphere)
{
	//バウンディングボックスの設定

	BB_VertexData vertexData = pBoundingBox->GetVertexData();

	D3DXMATRIX matWorld = pBoundingBox->GetWorldTransform();

	D3DXVECTOR3 vertices[8] = {
		{ vertexData.vecMin.x, vertexData.vecMax.y, vertexData.vecMin.z },
		{ vertexData.vecMax.x, vertexData.vecMax.y, vertexData.vecMin.z },
		{ vertexData.vecMin.x, vertexData.vecMin.y, vertexData.vecMin.z },
		{ vertexData.vecMax.x, vertexData.vecMin.y, vertexData.vecMin.z },
		{ vertexData.vecMin.x, vertexData.vecMax.y, vertexData.vecMax.z },
		{ vertexData.vecMax.x, vertexData.vecMax.y, vertexData.vecMax.z },
		{ vertexData.vecMin.x, vertexData.vecMin.y, vertexData.vecMax.z },
		{ vertexData.vecMax.x, vertexData.vecMin.y, vertexData.vecMax.z },
	};

	for (int i = 0; i < 8; i++) {
		vertices[i] = D3DXVec3TransformCoord(&vertices[i], &matWorld);
	}

	D3DXVECTOR3 face[6][4] = {
		{ vertices[0], vertices[1], vertices[3], vertices[2] },
		{ vertices[1], vertices[5], vertices[7], vertices[3] },
		{ vertices[5], vertices[4], vertices[6], vertices[7] },
		{ vertices[4], vertices[0], vertices[2], vertices[6] },
		{ vertices[4], vertices[5], vertices[1], vertices[0] },
		{ vertices[2], vertices[3], vertices[7], vertices[6] },
	};

	//バウンディングスフィアの設定

	matWorld = pBoundingSphere->GetWorldTransform();

	D3DXVECTOR3 center;
	center.x = matWorld.m[3][0];
	center.y = matWorld.m[3][1];
	center.z = matWorld.m[3][2];

	float radius = pBoundingSphere->GetRadius();

	//ボックスの頂点からスフィアの中心までの距離と半径を比較

	for (int i = 0; i < 8; i++) {

		D3DXVECTOR3 work = vertices[i] - center;

		float distance = D3DXVec3Length(&work);

		if (distance <= radius) {
			return true;
		}
	}

	//ボックスの面からスフィアの中心までの距離と半径を比較

	int minusCounter = 0;

	for (int i = 0; i < 6; i++) {

		D3DXVECTOR3 work = face[i][0] - center;

		D3DXVECTOR3 normal = D3DXComputeNormal(&face[i][0], &face[i][1], &face[i][2]);

		D3DXVECTOR3 ray = -normal;

		float distance = D3DXVec3Dot(&work, &ray);
		
		//距離が０以上であれば表面
		if (distance >= 0) {

			//平面までの距離が半径以下であれば平面と衝突している
			if (distance <= radius) {

				//交点を求める

				float numerator = D3DXVec3Dot(&normal, &work);

				float denominator = D3DXVec3Dot(&normal, &ray);

				float t = numerator / denominator;

				D3DXVECTOR3 crossPoint = center + ray * t;

				//バウンディングボックスの面の中に交点があれば衝突している
				D3DXVECTOR3 v1, v2;
				float result;

				v1 = face[i][1] - face[i][0];
				v2 = crossPoint - face[i][0];
				D3DXVec3Cross(&work, &v1, &v2);
				result = D3DXVec3Dot(&work, &normal);
				if (result < 0) {
					continue;
				}

				v1 = face[i][2] - face[i][1];
				v2 = crossPoint - face[i][1];
				D3DXVec3Cross(&work, &v1, &v2);
				result = D3DXVec3Dot(&work, &normal);
				if (result < 0) {
					continue;
				}

				v1 = face[i][3] - face[i][2];
				v2 = crossPoint - face[i][2];
				D3DXVec3Cross(&work, &v1, &v2);
				result = D3DXVec3Dot(&work, &normal);
				if (result < 0) {
					continue;
				}

				v1 = face[i][0] - face[i][3];
				v2 = crossPoint - face[i][3];
				D3DXVec3Cross(&work, &v1, &v2);
				result = D3DXVec3Dot(&work, &normal);
				if (result < 0) {
					continue;
				}

				return true;
			}
		}
		else {
			//マイナスであれば裏面（裏面の数を数える）
			minusCounter++;
		}
	}

	//６面全てが裏であればスフィアの中心がボックスに内包されているということ
	if (minusCounter >= 6) {
		return true;
	}

	//↑だけでは角にあたる部分の判定が出来ないため線分との判定を行う
	D3DXVECTOR3 lines[12][2] = {
		{ vertices[0], vertices[1] },
		{ vertices[1], vertices[2] },
		{ vertices[2], vertices[3] },
		{ vertices[3], vertices[0] },
		{ vertices[5], vertices[4] },
		{ vertices[4], vertices[6] },
		{ vertices[6], vertices[7] },
		{ vertices[7], vertices[5] },
		{ vertices[0], vertices[4] },
		{ vertices[1], vertices[5] },
		{ vertices[2], vertices[6] },
		{ vertices[3], vertices[7] },
	};

	for (int i = 0; i < 12; i++) {
		D3DXVECTOR3 ray = lines[i][1] - lines[i][0];
		float length = D3DXVec3Length(&ray);
		ray = D3DXVec3Normalize(&ray);
		if (CollideBSAndLine(&lines[i][0], &ray, length, pBoundingSphere)) {
			return true;
		}
	}

	return false;
}


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
bool Collision::CheckCrossPoint(const D3DXVECTOR3* pVecPos, const D3DXVECTOR3* pVecRay, const D3DXVECTOR3* pVecNormal, const D3DXVECTOR3 vPolygon[3], D3DXVECTOR3* pVecCrossPoint, bool bIgnoreStartPos)
{
	//平面との交点を求める

	D3DXVECTOR3 vecWork = vPolygon[0] - *pVecPos;

	float numerator = D3DXVec3Dot(pVecNormal, &vecWork);

	float denominator = D3DXVec3Dot(pVecNormal, pVecRay);

	//レイが平面に平行な場合や裏ポリゴンの場合、交点無しとする
	if (denominator >= 0) {
		return false;
	}

	float t = numerator / denominator;

	//平面との交点を求める
	D3DXVECTOR3 vP = *pVecPos + *pVecRay * t;

	if (!bIgnoreStartPos) {
		//レイの逆方向にあるポリゴンの場合、交点無しとする
		vecWork = vP - *pVecPos;
		D3DXVECTOR3 vecNor = D3DXVec3Normalize(&vecWork);
		if (D3DXVec3Dot(pVecRay, &vecNor) <= 0) {
			return false;
		}
	}

	//交点がポリゴン内にあるかどうか調べる
	D3DXVECTOR3 v1, v2;
	float result;

	v1 = vPolygon[1] - vPolygon[0];
	v2 = vP - vPolygon[1];
	D3DXVec3Cross(&vecWork, &v1, &v2);
	result = D3DXVec3Dot(&vecWork, pVecNormal);
	if (result < 0) {
		return false;
	}

	v1 = vPolygon[2] - vPolygon[1];
	v2 = vP - vPolygon[2];
	D3DXVec3Cross(&vecWork, &v1, &v2);
	result = D3DXVec3Dot(&vecWork, pVecNormal);
	if (result < 0) {
		return false;
	}

	v1 = vPolygon[0] - vPolygon[2];
	v2 = vP - vPolygon[0];
	D3DXVec3Cross(&vecWork, &v1, &v2);
	result = D3DXVec3Dot(&vecWork, pVecNormal);
	if (result < 0) {
		return false;
	}

	if (pVecCrossPoint) {
		*pVecCrossPoint = vP;
	}

	return true;
}

//=============================================================================
// 衝突判定　２Ｄ用ＯＢＢ
// 戻り値：交差している場合 true、していない場合 false
// 引　数：const ObbData2D & ２Ｄ用ＯＢＢデータ構造体の参照
// 　　　　const ObbData2D & ２Ｄ用ＯＢＢデータ構造体の参照
//=============================================================================
bool Collision::CollideOBB2D(const ObbData2D &obb1, const ObbData2D &obb2)
{
	//縦横の半径分のベクトルを求める
	D3DXVECTOR2 horizontalRadius1 = D3DXVECTOR2(obb1.m_width / 2.0f, 0.0f);
	D3DXVECTOR2 verticalRadius1 = D3DXVECTOR2(0.0f, obb1.m_height / 2.0f);

	D3DXVECTOR2 horizontalRadius2 = D3DXVECTOR2(obb2.m_width / 2.0f, 0.0f);
	D3DXVECTOR2 verticalRadius2 = D3DXVECTOR2(0.0f, obb2.m_height / 2.0f);

	//中心座標を求める
	D3DXVECTOR2 center1 = D3DXVECTOR2(obb1.m_position.x + obb1.m_width / 2.0f, obb1.m_position.y + obb1.m_height / 2.0f);
	D3DXVECTOR2 center2 = D3DXVECTOR2(obb2.m_position.x + obb2.m_width / 2.0f, obb2.m_position.y + obb2.m_height / 2.0f);

	//ベクトルを回転させる
	D3DXVECTOR2 rotateHorizontalRadius1 = D3DXVec2Rotate(&horizontalRadius1, obb1.m_angle);
	D3DXVECTOR2 rotateVerticalRadius1 = D3DXVec2Rotate(&verticalRadius1, obb1.m_angle);

	D3DXVECTOR2 rotateHorizontalRadius2 = D3DXVec2Rotate(&horizontalRadius2, obb2.m_angle);
	D3DXVECTOR2 rotateVerticalRadius2 = D3DXVec2Rotate(&verticalRadius2, obb2.m_angle);

	//中心から中心への距離を調べる
	D3DXVECTOR2 centerToCenter = center2 - center1;

	//射影する軸を決め、中心同士の長さと比較する

	D3DXVECTOR2 projection;

	projection = D3DXVECTOR2(1.0f, 0.0f);
	if (!CompareLength2D(centerToCenter, projection, rotateHorizontalRadius1, rotateVerticalRadius1, rotateHorizontalRadius2, rotateVerticalRadius2)) {
		return false;
	}
	
	projection = D3DXVECTOR2(0.0f, 1.0f);
	if (!CompareLength2D(centerToCenter, projection, rotateHorizontalRadius1, rotateVerticalRadius1, rotateHorizontalRadius2, rotateVerticalRadius2)) {
		return false;
	}
	
	projection = D3DXVec2Normalize(&rotateHorizontalRadius1);
	if (!CompareLength2D(centerToCenter, projection, rotateHorizontalRadius1, rotateVerticalRadius1, rotateHorizontalRadius2, rotateVerticalRadius2)) {
		return false;
	}

	projection = D3DXVec2Normalize(&rotateVerticalRadius1);
	if (!CompareLength2D(centerToCenter, projection, rotateHorizontalRadius1, rotateVerticalRadius1, rotateHorizontalRadius2, rotateVerticalRadius2)) {
		return false;
	}

	projection = D3DXVec2Normalize(&rotateHorizontalRadius2);
	if (!CompareLength2D(centerToCenter, projection, rotateHorizontalRadius1, rotateVerticalRadius1, rotateHorizontalRadius2, rotateVerticalRadius2)) {
		return false;
	}
	
	projection = D3DXVec2Normalize(&rotateVerticalRadius2);
	if (!CompareLength2D(centerToCenter, projection, rotateHorizontalRadius1, rotateVerticalRadius1, rotateHorizontalRadius2, rotateVerticalRadius2)) {
		return false;
	}

	return true;
}



//*****************************************************************************
// private関数
//*****************************************************************************


//-----------------------------------------------------------------------------
// ＯＢＢ用分離軸比較関数
// 戻り値：衝突している場合 true、していない場合 false
// 引　数：const BB_ObbData*　ＯＢＢデータ１へのポインタ
// 　　　　const BB_ObbData*　ＯＢＢデータ２へのポインタ
// 　　　　const D3DXVECTOR3* 射影する軸ベクトルへのポインタ（単位ベクトル）
// 　　　　const D3DXVECTOR3* ２つのバウンディングボックスの中心を結ぶ距離へのポインタ
//-----------------------------------------------------------------------------
bool Collision::CompareLength(const BB_ObbData* pObb1, const BB_ObbData* pObb2, const D3DXVECTOR3* pVecAxis, const D3DXVECTOR3* pVecDistance)
{
	double distance;
	double r1, r2;

	//中心間のベクトルを投影
	distance = fabs(D3DXVec3Dot(pVecAxis, pVecDistance));

	//object1の半径(x､y､z)を投影
	r1 = fabs(D3DXVec3Dot(&pObb1->vecAxisX, pVecAxis) * pObb1->vecRadius.x)
		+ fabs(D3DXVec3Dot(&pObb1->vecAxisY, pVecAxis) * pObb1->vecRadius.y)
		+ fabs(D3DXVec3Dot(&pObb1->vecAxisZ, pVecAxis) * pObb1->vecRadius.z);

	//object2の半径(x､y､z)を投影
	r2 = fabs(D3DXVec3Dot(&pObb2->vecAxisX, pVecAxis) * pObb2->vecRadius.x)
		+ fabs(D3DXVec3Dot(&pObb2->vecAxisY, pVecAxis) * pObb2->vecRadius.y)
		+ fabs(D3DXVec3Dot(&pObb2->vecAxisZ, pVecAxis) * pObb2->vecRadius.z);

	if (distance > r1 + r2) {
		return false;
	}

	return true;
}


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
bool Collision::CompareLength2D(D3DXVECTOR2 &centerToCenter, D3DXVECTOR2 &projection, D3DXVECTOR2 &rotateHorizontalRadius1, D3DXVECTOR2 &rotateVerticalRadius1, D3DXVECTOR2 &rotateHorizontalRadius2, D3DXVECTOR2 &rotateVerticalRadius2)
{
	float horizontalRadiusLength1;
	float verticalRadiusLength1;
	float horizontalRadiusLength2;
	float verticalRadiusLength2;

	float totalLength;

	float distance;

	horizontalRadiusLength1 = fabs(D3DXVec2Dot(&rotateHorizontalRadius1, &projection));
	verticalRadiusLength1 = fabs(D3DXVec2Dot(&rotateVerticalRadius1, &projection));
	horizontalRadiusLength2 = fabs(D3DXVec2Dot(&rotateHorizontalRadius2, &projection));
	verticalRadiusLength2 = fabs(D3DXVec2Dot(&rotateVerticalRadius2, &projection));

	totalLength = horizontalRadiusLength1 + verticalRadiusLength1 + horizontalRadiusLength2 + verticalRadiusLength2;

	distance = fabs(D3DXVec2Dot(&centerToCenter, &projection));

	if (totalLength < distance) {
		return false;
	}

	return true;
}






