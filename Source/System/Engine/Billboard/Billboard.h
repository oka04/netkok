//*****************************************************************************
//
// ビルボードクラス
//
// Billboard.h
//
// K_Yamaoka
//
// 2013/03/04
//
// 2020/08/24 Draw関数の引数を変更
//
// 2020/08/26 マテリアルを削除
//
//*****************************************************************************

#pragma once

#include <d3dx9.h>
#include <string>
#include <vector>

#include "..\\Engine.h"
#include "..\\Matrix\\Camera.h"
#include "..\\Matrix\Projection.h"
#include "..\\Light\\Ambient.h"
#include "..\\Light\\Directional.h"

#include "..\\Mesh\\Primitive.h" //Coord構造体

#include "..\\..\\..\\Effect\\resource.h"

//-----------------------------------------------------------------------------
//モデル用データ構造体
//-----------------------------------------------------------------------------
typedef struct {
	float      width;  //メッシュ幅
	float      height; //メッシュ高さ
	LPD3DXMESH pMesh;  //メッシュのポインタ
} BillboardModelData;

//-----------------------------------------------------------------------------
//テクスチャ用データ構造体
//-----------------------------------------------------------------------------
typedef struct {
	std::string        strFilename; //テクスチャファイル名
	LPDIRECT3DTEXTURE9 pTexture;    //テクスチャクラスのポインタ
} BillboardTextureData;

//-----------------------------------------------------------------------------
//ビルボード用データ構造体
//-----------------------------------------------------------------------------
typedef struct {
	int         modelNo;     //モデルデータ配列のインデックス
	int         textureNo;   //テクスチャデータ配列のインデックス
	D3DXVECTOR3 vecPosition; //ビルボードの位置座標
} BillboardData;


class Billboard
{
public:

	//=============================================================================
	// コンストラクタ
	//=============================================================================
	Billboard();

	//=============================================================================
	// デストラクタ
	//=============================================================================
	~Billboard();

	//=============================================================================
	// ビルボードの追加
	// 引　数：Engine*            ３Ｄエンジンクラスへのポインタ
	// 　　　　const D3DXVECTOR3* ビルボードの位置へのポインタ
	// 　　　　const float        幅
	// 　　　　const float        高さ
	// 　　　　std::string        テクスチャファイル名
	// 　　　　const DWORD        カラーキー（デフォルトは紫）
	//=============================================================================
	void Add(Engine* pEngine, const D3DXVECTOR3* pVecPosition, const float width, const float height, std::string strFilename, const DWORD colorKey = 0x00ff00ff);

	//=============================================================================
	// ビルボードの描画
	// 引　数：Engine*           ３Ｄエンジンクラスのポインタ
	// 　　　　Camera*           カメラクラスのポインタ
	// 　　　　Projection*       プロジェクションクラスのポインタ
	// 　　　　AmbientLight*     アンビエントライトクラスのポインタ
	// 　　　　DirectionalLight* ディレクショナルライトのポインタ
	//=============================================================================
	void Billboard::Draw(Engine* pEngine, Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight);

private:

	//同じ大きさ、テクスチャのビルボードデータは共有する

	//モデルデータコンテナ
	std::vector< BillboardModelData > m_modelContainer;

	//テクスチャデータコンテナ
	std::vector< BillboardTextureData > m_textureContainer;

	//モデルデータとテクスチャをつなぐ、ビルボードデータコンテナ
	std::vector< BillboardData > m_billboardContainer;

	//エフェクト（シェーダー）
	ID3DXEffect* m_pEffect;

	//頂点宣言
	LPDIRECT3DVERTEXDECLARATION9 m_pVertexDeclaration;

	//-----------------------------------------------------------------------------
	// メッシュの削除
	//-----------------------------------------------------------------------------
	void DeleteMesh();

	//-----------------------------------------------------------------------------
	// テクスチャの削除
	//-----------------------------------------------------------------------------
	void DeleteTexture();

	//コピー防止
	Billboard& operator=(const Billboard&);
	Billboard(const Billboard&);
};