//*****************************************************************************
//
// Frame.h
//
// フレーム管理クラス
//
// k.yamaoka
//
// 2008/12/17
//
// DirectXサンプル「SkinMesh」を元に作成
//
// 2014/11/28 エラーメッセージを追加
//            ShowErrorMessage関数を削除
//
// 2017/12/20 D3DXFRAME_DERIVED構造体のアラインメント問題を解消
//
// 2020/09/01 シェーダー対応に変更
//
//*****************************************************************************

#pragma once

#include <windows.h>
#include <d3dx9.h>
#include <dxerr9.h>

#include "..\\..\\Exception\\DxSystemException.h"

//-----------------------------------------------------------------------------
//　D3DXFRAME構造体の拡張
//-----------------------------------------------------------------------------
struct D3DXFRAME_DERIVED : public D3DXFRAME
{
	//親ﾌﾚｰﾑとの変換行列と合成するための行列
	D3DXMATRIXA16 CombinedTransformationMatrix;

	void* operator new(size_t size) {
		return _mm_malloc(size, alignof(D3DXFRAME_DERIVED));
	}

	void operator delete(void* p) {
		return _mm_free(p);
	}
};

//-----------------------------------------------------------------------------
//　D3DXMESHCONTAINERの拡張
//-----------------------------------------------------------------------------
struct D3DXMESHCONTAINER_DERIVED: public D3DXMESHCONTAINER
{
	//ﾃｸｽﾁｬ配列用
    LPDIRECT3DTEXTURE9*  ppTextures;
                                
    //ｽｷﾝﾒｯｼｭ情報
    LPD3DXMESH           pOrigMesh;             //元のメッシュデータ
    DWORD                numMaxFaceInfle;       //１つの頂点に影響するボーンの最大数
    DWORD                numBoneCombinations;   //ボーンコンビネーションの数
    LPD3DXBUFFER         pBoneCombinationTable; //ボーンと頂点の対応表配列
    D3DXMATRIX**         ppBoneMatrixPtrs;      //コンテナにある各ボーンの行列へのポインタ
    D3DXMATRIX*          pBoneOffsetMatrices;   //各ボーンの初期姿勢を格納した配列（ボーンの数だけ用意される）

    //DWORD                iAttributeSW;

	//シェーダー用
	LPDIRECT3DVERTEXDECLARATION9 pVertexDeclaration;
};

#if false
typedef struct _D3DXMESHCONTAINER
{
	LPSTR                   Name;         //メッシュコンテナ名

	D3DXMESHDATA            MeshData;     //変換後メッシュデータ

	LPD3DXMATERIAL          pMaterials;   //マテリアル配列
	LPD3DXEFFECTINSTANCE    pEffects;     
	DWORD                   NumMaterials; //マテリアル数
	DWORD                  *pAdjacency;

	LPD3DXSKININFO          pSkinInfo;    //スキン情報

	struct _D3DXMESHCONTAINER *pNextMeshContainer;
} D3DXMESHCONTAINER, *LPD3DXMESHCONTAINER;

#endif

#include "AllocateHierarchy.h"

class Frame
{
public:

	//=============================================================================
	// コンストラクタ
	//=============================================================================
	Frame();

	//=============================================================================
	// フレームへボーンマトリックスを設定
	// 戻り値：成功 S_OK　失敗 エラーコード
	// 引　数：LPD3DXFRAME フレームへのポインタ
	//=============================================================================
	HRESULT SetupBoneMatrixPointers(LPD3DXFRAME pFrame);

	//=============================================================================
	// フレーム行列の更新（親子関係の設定）
	// 引　数：LPD3DXFRAME  フレームへのポインタ
	// 　　　　LPD3DXMATRIX 親フレームのマトリックス
	//=============================================================================
	void UpdateFrameMatrices(LPD3DXFRAME pFrameBase, LPD3DXMATRIX pParentMatrix);

	//=============================================================================
	// フレームの描画
	// 引　数：LPD3DXFRAME       フレームへのポインタ
	// 　　　　LPD3DXMATRIX      ビュープロジェクション変換マトリックスへのポインタ
	//         ID3DXEffect*      エフェクトへのポインタ
	//=============================================================================
	void DrawFrame(LPD3DXFRAME pFrame, LPD3DXMATRIX pMatVP, ID3DXEffect* pEffect);

	//=============================================================================
	// ルートフレームのポインタのアドレス取得
	// 戻り値：ルートフレームのポインタのアドレス
	//=============================================================================
	LPD3DXFRAME* GetRootFramePointerAddress();

	//=============================================================================
	// フレーム名からフレームを取得
	// 戻り値：フレームのアドレス
	// 引　数：LPCSTR フレーム名
	//=============================================================================
	LPD3DXFRAME GetFrame(LPCSTR FrameName);

	//=============================================================================
	// ルートフレーム内のメッシュへのポインタを取得
	// 戻り値：ルートフレーム内のメッシュへのポインタ
	//=============================================================================
	LPD3DXMESH GetRootMesh() const;

private:

	//ルートフレーム
	LPD3DXFRAME m_pFrameRoot;

	//-----------------------------------------------------------------------------
	// ボーン変換マトリックスのポインタをメッシュに設定
	// 戻り値：成功 S_OK　失敗 エラーコード
	// 引　数：LPD3DXMESHCONTAINER メッシュコンテナのポインタ
	//-----------------------------------------------------------------------------
	HRESULT SetupBoneMatrixPointersOnMesh(LPD3DXMESHCONTAINER pMeshContainerBase);

	//-----------------------------------------------------------------------------
	// メッシュの描画
	// 引　数：LPD3DXMESHCONTAINER メッシュコンテナのポインタ
	// 　　　　LPD3DXFRAME         フレームのポインタ
	// 　　　　LPD3DXMATRIX      ビュープロジェクション変換マトリックスへのポインタ
	//         ID3DXEffect*      エフェクトへのポインタ
	//-----------------------------------------------------------------------------
	void DrawMeshContainer(LPD3DXMESHCONTAINER pMeshContainerBase, LPD3DXFRAME pFrameBase, LPD3DXMATRIX pMatVP, ID3DXEffect* pEffect);

	//ｺﾋﾟｰ防止
	Frame(const Frame&);
	Frame& operator=(const Frame&);
};