//*****************************************************************************
//
// AllocateHierarchy.h
//
// 階層構造をもったメッシュの管理クラス
//
// k.yamaoka
//
// 2008/12/16
//
// DirectXサンプル「SkinMesh」を元に作成
//
// 2020/09/01 シェーダー対応に変更
//
//*****************************************************************************

#pragma once

#include <windows.h>
#include <d3dx9.h>
#include <dxerr9.h>

#include "Frame.h"

class AllocateHierarchy : public ID3DXAllocateHierarchy
{
public:

	//=============================================================================
	// コンストラクタ
	//=============================================================================
	AllocateHierarchy();

	//=============================================================================
	// 新しいフレームの作成
	// 引　数：LPCSTR       フレーム名
	// 　　　　LPD3DXFRAME* D3DXFRAME構造体のポインタへのアドレス
	//=============================================================================
	HRESULT __stdcall CreateFrame(LPCSTR Name, LPD3DXFRAME* ppNewFrame);

	//=============================================================================
	// メッシュコンテナの作成
	// 引　数：LPCSTR                    メッシュ名
	// 　　　　CONST D3DXMESHDATA*       メッシュデータへのポインタ
	// 　　　　CONST D3DXMATERIAL*       マテリアルへのポインタ
	// 　　　　CONST D3DXEFFECTINSTANCE* エフェクトインスタンスへのポインタ
	// 　　　　DWORD                     マテリアル数
	// 　　　　CONST DWORD*              隣接ポリゴンインデックス
	// 　　　　LPD3DXSKININFO            スキン情報
	// 　　　　LPD3DXMESHCONTAINER*　    新しく作成するメッシュコンテナのポインタへのアドレス
	//=============================================================================
	HRESULT __stdcall CreateMeshContainer(
		LPCSTR Name,
		CONST D3DXMESHDATA* pMeshData,
		CONST D3DXMATERIAL* pMaterials,
		CONST D3DXEFFECTINSTANCE* pEffectInstance,
		DWORD NumMaterials,
		CONST DWORD* pAdjacency,
		LPD3DXSKININFO pSkinInfo,
		LPD3DXMESHCONTAINER* ppNewMeshContainer
	);

	//=============================================================================
	// フレームの解放
	// 引　数：LPD3DXFRAME 解放するフレームのポインタ
	//=============================================================================
	HRESULT __stdcall DestroyFrame(LPD3DXFRAME pFrameToFree);

	//=============================================================================
	// メッシュコンテナの解放
	// 引　数：LPD3DXMESHCONTAINER 解放するメッシュコンテナのポインタ
	//=============================================================================
	HRESULT __stdcall DestroyMeshContainer(LPD3DXMESHCONTAINER pMeshContainerBase);

	//=============================================================================
	// スキンメッシュかどうか調べる
	//=============================================================================
	bool IsSkinMesh() const;

private:

	//-----------------------------------------------------------------------------
	// 名前のコピー（領域確保を含む）
	// 引　数：LPCSTR コピー元の名前
	// 　　　　LPSTR  コピー先のポインタ
	//-----------------------------------------------------------------------------
	HRESULT CopyName(LPCSTR Name, LPSTR* pNewName);

	//-----------------------------------------------------------------------------
	// 最適化スキンメッシュの作成
	// 引　数：D3DXMESHCONTAINER_DERIVED* メッシュコンテナのポインタ
	//-----------------------------------------------------------------------------
	HRESULT GenerateSkinnedMesh(D3DXMESHCONTAINER_DERIVED* pMeshContainer);

	//スキンメッシュフラグ
	bool m_bSkinMesh;

	//コピー防止
	AllocateHierarchy(const AllocateHierarchy&);
	AllocateHierarchy& operator=(const AllocateHierarchy&);
};