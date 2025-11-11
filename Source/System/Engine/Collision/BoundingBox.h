//*****************************************************************************
//
// バウンディングボックスクラス
//
// BoundingBox.h
//
// K_Yamaoka
//
// 2013/03/04
//
// 2014/10/17 GetWorldTransformMatrix→GetWorldTransform　名称変更
//            SetWorldTransformMatrix→SetWorldTransform　名称変更
//
// 2020/09/03 シェーダー対応に変更
//
//*****************************************************************************

#pragma once

#include <d3dx9.h>
#include <dxerr9.h>

#include "..\\Engine.h"
#include "..\\Matrix\\Camera.h"
#include "..\\Matrix\\Projection.h"

//バウンディングボックス　頂点構造体
typedef struct {
	D3DXVECTOR3 vecMax; //最大値
	D3DXVECTOR3 vecMin; //最小値
} BB_VertexData;


//ＯＢＢ用データ構造体
typedef struct {
	D3DXVECTOR3 vecCenter; //ﾊﾞｳﾝﾃﾞｨﾝｸﾞﾎﾞｯｸｽの中心位置
	D3DXVECTOR3 vecRadius; //ﾊﾞｳﾝﾃﾞｨﾝｸﾞﾎﾞｯｸｽの半径
	D3DXVECTOR3 vecAdjust; //ﾒｯｼｭの中心と座標軸の中心のずれを調整するための位置ﾍﾞｸﾄﾙ
	D3DXVECTOR3 vecAxisX; //分離軸X計算用ﾍﾞｸﾄﾙ
	D3DXVECTOR3 vecAxisY; //分離軸Y計算用ﾍﾞｸﾄﾙ
	D3DXVECTOR3 vecAxisZ; //分離軸Z計算用ﾍﾞｸﾄﾙ
} BB_ObbData;

//レイとの交差判定用構造体
typedef struct {
	D3DXVECTOR3 vertex[3];
} BB_Polygon;


class BoundingBox
{
public:

	static const int POLYGON_NUM;

	//=============================================================================
	// コンストラクタ
	//=============================================================================
	BoundingBox();

	//=============================================================================
	// デストラクタ
	//=============================================================================
	~BoundingBox();

	//=============================================================================
	// バウンディングボックスの作成
	// 引　数：Engine*　　　　　　３Ｄエンジンクラスへのポインタ
	// 　　　　const D3DXVECTOR3* 座標の最大値
	// 　　　　const D3DXVECTOR3* 座標の最小値
	// 　　　　const bool         描画フラグ（デバッグ用）デフォルトはfalse
	//=============================================================================
	void CreateBB(Engine* pEngine, const D3DXVECTOR3* pVecMax, const D3DXVECTOR3* pVecMin, const bool showFlag = false);

	//=============================================================================
	// バウンディングボックスの描画
	// 引　数：Engine*　３Ｄエンジンクラスへのポインタ
	// 　　　　Camera*           カメラクラスのポインタ
	// 　　　　Projection*       プロジェクションクラスのポインタ
	// 　　　　D3DCOLORVALUE*    色　デフォルトは白
	//=============================================================================
	void Draw(Engine* pEngine, Camera* pCamera, Projection* pProj, D3DCOLORVALUE* color = nullptr);

	//=============================================================================
	// ワールド座標変換行列の設定
	// 引　数：const LPD3DXMATRIX ワールド座標変換行列へのポインタ
	//=============================================================================
	void SetWorldTransform(const LPD3DXMATRIX pMatWorld);

	//=============================================================================
	// ワールド座標変換行列の取得
	// 戻り値：const D3DXMATRIX& ワールド座標変換行列
	//=============================================================================
	D3DXMATRIX GetWorldTransform() const;

	//=============================================================================
	// バウンディングボックス　頂点データの取得
	// 戻り値：BB_VertexData 頂点データ構造体
	//=============================================================================
	BB_VertexData GetVertexData() const;

	//=============================================================================
	// バウンディングボックス　ＯＢＢ用データの取得
	// 戻り値：BB_ObbData* ＯＢＢ用データへのポインタ（更新の必要があるため）
	//=============================================================================
	BB_ObbData* GetObbData();

	//=============================================================================
	// レイとの当たり判定用のポリゴン取得
	// 戻り値：BB_Polygon ポリゴンの頂点情報構造体
	// 引　数：const int ポリゴンのインデックス（０～１１）
	//=============================================================================
	BB_Polygon GetPolygon(const int index);

private:

	//頂点数
	static const int VERTEX_NUM;

	//インデックス数
	static const int INDEX_NUM;

	//バーテックスバッファへのポインタ
	LPDIRECT3DVERTEXBUFFER9 m_pVB;

	//インデックスバッファへのポインタ
	LPDIRECT3DINDEXBUFFER9  m_pIB;

	//ワールド座標変換行列
	D3DXMATRIX m_matWorld;
	
	//頂点座標（最大、最少）
	BB_VertexData m_vertexData;

	//OBB用データ
	BB_ObbData m_obbData;

	//ポリゴンデータ取得用
	BB_Polygon m_polygon;

	//エフェクト（シェーダー）
	ID3DXEffect* m_pEffect;

	//頂点宣言
	LPDIRECT3DVERTEXDECLARATION9 m_pVertexDeclaration;

	//-----------------------------------------------------------------------------
	// 頂点バッファ、インデックスバッファの削除
	//-----------------------------------------------------------------------------
	void Delete();

	//コピー防止
	BoundingBox& operator=(const BoundingBox&);
	BoundingBox(const BoundingBox&);
};