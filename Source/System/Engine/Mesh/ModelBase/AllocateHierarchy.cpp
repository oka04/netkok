//*****************************************************************************
//
// AllocateHierarchy.cpp
//
// 階層構造をもったメッシュの管理クラス
//
// k.yamaoka
//
// 2008/12/16
//
// DirectXサンプル「SkinMesh」を元に作成
//
// 2014/11/28 エラーメッセージを追加
//
// 2020/09/01 シェーダー対応に変更
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "AllocateHierarchy.h"

//#define _XFILE_CHECKER

#include <windows.h>

//=============================================================================
// コンストラクタ
//=============================================================================
AllocateHierarchy::AllocateHierarchy() : m_bSkinMesh(false)
{

}

//=============================================================================
// 新しいフレームの作成
// 引　数：LPCSTR       フレーム名
// 　　　　LPD3DXFRAME* D3DXFRAME構造体のポインタへのアドレス
//=============================================================================
HRESULT __stdcall AllocateHierarchy::CreateFrame(LPCSTR Name, LPD3DXFRAME* ppNewFrame)
{
	HRESULT hr = S_OK;

	//新しいﾌﾚｰﾑの作成
	D3DXFRAME_DERIVED* pFrame = nullptr;

	pFrame = new D3DXFRAME_DERIVED;

	if (!pFrame) {
		hr = E_OUTOFMEMORY;
		DxSystemException(DxSystemException::OM_AH_CREATE_FRAME_DERIVED_ERROR).ShowOriginalMessage();
		goto EXIT_FUNCTION;
	}

	//ﾌﾚｰﾑ名のｺﾋﾟｰ
	hr = CopyName(Name, &pFrame->Name);

	if (FAILED(hr)) {
		DxSystemException(DxSystemException::OM_AH_COPYFRAMENAME_ERROR).ShowOriginalMessage();
		goto EXIT_FUNCTION;
	}

	//変換行列の初期化
	D3DXMatrixIdentity(&pFrame->TransformationMatrix);
	D3DXMatrixIdentity(&pFrame->CombinedTransformationMatrix);

	//各ﾎﾟｲﾝﾀの初期化
	pFrame->pMeshContainer = nullptr;
	pFrame->pFrameSibling = nullptr;
	pFrame->pFrameFirstChild = nullptr;

	//作成したﾌﾚｰﾑを引数に代入
	*ppNewFrame = pFrame;

	pFrame = nullptr;

EXIT_FUNCTION:

	if (pFrame) {
		delete pFrame;
		pFrame = nullptr;
	}

	return hr;
}

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
HRESULT __stdcall AllocateHierarchy::CreateMeshContainer(
	LPCSTR Name,
	CONST D3DXMESHDATA* pMeshData,
	CONST D3DXMATERIAL* pMaterials,
	CONST D3DXEFFECTINSTANCE* pEffectInstances,
	DWORD NumMaterials,
	CONST DWORD* pAdjacency,
	LPD3DXSKININFO pSkinInfo,
	LPD3DXMESHCONTAINER* ppNewMeshContainer)
{
	//１つの関数として長すぎる
	//スキンメッシュの作成はprivateメンバ関数に分ける

	//エフェクトは無視
	UNREFERENCED_PARAMETER(pEffectInstances);

	HRESULT hr;

	D3DXMESHCONTAINER_DERIVED* pMeshContainer = nullptr;
	UINT NumFaces;
	UINT iMaterial;
	UINT iBone, cBones;
	LPDIRECT3DDEVICE9 pDevice = nullptr;

	LPD3DXMESH pMesh = nullptr;

	*ppNewMeshContainer = nullptr;

	//ﾒｯｼｭﾀｲﾌﾟはﾉｰﾏﾙﾒｯｼｭのみ扱う
	if (pMeshData->Type != D3DXMESHTYPE_MESH) {
		hr = E_FAIL;
		DxSystemException(DxSystemException::OM_AH_MESH_TYPE_ERROR).ShowOriginalMessage();
		goto EXIT_FUNCTION;
	}

	//ﾒｯｼｭの取り出し
	pMesh = pMeshData->pMesh;

	//FVFが設定されていなければｴﾗｰ
	if (pMesh->GetFVF() == 0) {
		hr = E_FAIL;
		DxSystemException(DxSystemException::OM_AH_NONE_FVF_ERROR).ShowOriginalMessage();
		goto EXIT_FUNCTION;
	}

	//拡張ﾒｯｼｭｺﾝﾃﾅの作成
	pMeshContainer = new D3DXMESHCONTAINER_DERIVED;

	if (!pMeshContainer) {
		hr = E_OUTOFMEMORY;
		DxSystemException(DxSystemException::OM_AH_CREATE_CONTAINER_DERIVED_ERROR).ShowOriginalMessage();
		goto EXIT_FUNCTION;
	}

	//領域のｾﾞﾛｸﾘｱ
	ZeroMemory(pMeshContainer, sizeof(D3DXMESHCONTAINER_DERIVED));

	//ﾒｯｼｭｺﾝﾃﾅ名のｺﾋﾟｰ
	hr = CopyName(Name, &pMeshContainer->Name);

	if (FAILED(hr)) {
		DxSystemException(DxSystemException::OM_AH_COPYCONTAINERNAME_ERROR).ShowOriginalMessage();
		goto EXIT_FUNCTION;
	}

	//ﾃﾞﾊﾞｲｽの取得
	pMesh->GetDevice(&pDevice);

	//登録されているﾎﾟﾘｺﾞﾝ数を取得
	NumFaces = pMesh->GetNumFaces();

	//法線情報が無い場合付加する
	if (!(pMesh->GetFVF() & D3DFVF_NORMAL)) {

		//ﾒｯｼｭﾀｲﾌﾟの設定(ﾉｰﾏﾙﾒｯｼｭのみ)
		pMeshContainer->MeshData.Type = D3DXMESHTYPE_MESH;

		//法線情報を付加したﾌｫｰﾏｯﾄを設定
		hr = pMesh->CloneMeshFVF(pMesh->GetOptions(),
			pMesh->GetFVF() | D3DFVF_NORMAL,
			pDevice,
			&pMeshContainer->MeshData.pMesh);

		if (FAILED(hr)) {
			DxSystemException(DxSystemException::OM_AH_CLONE_MESH_ERROR).ShowOriginalMessage();
			goto EXIT_FUNCTION;
		}

		//法線情報を付加したﾒｯｼｭを再取得
		pMesh = pMeshContainer->MeshData.pMesh;

		//法線の計算
		D3DXComputeNormals(pMesh, nullptr);
	}
	else {

		//法線が設定されていた場合、そのままﾒｯｼｭｺﾝﾃﾅに設定
		pMeshContainer->MeshData.pMesh = pMesh;

		//ﾒｯｼｭﾀｲﾌﾟの設定(ﾉｰﾏﾙﾒｯｼｭのみ)
		pMeshContainer->MeshData.Type = D3DXMESHTYPE_MESH;

		//法線の再計算（法線が壊れているケースがある）
		D3DXComputeNormals(pMeshContainer->MeshData.pMesh, nullptr);

		//参照ｶｳﾝﾄを増やす
		pMesh->AddRef();
	}

	//ﾏﾃﾘｱﾙ数(最低1)を設定
	pMeshContainer->NumMaterials = max(1, NumMaterials);

	//ﾏﾃﾘｱﾙ数分のﾏﾃﾘｱﾙ格納領域を確保
	pMeshContainer->pMaterials = new D3DXMATERIAL[pMeshContainer->NumMaterials];

	//ﾏﾃﾘｱﾙ数分のﾃｸｽﾁｬ格納領域を確保
	pMeshContainer->ppTextures = new LPDIRECT3DTEXTURE9[pMeshContainer->NumMaterials];

	//隣接ﾎﾟﾘｺﾞﾝのｲﾝﾃﾞｯｸｽ格納領域を確保
	pMeshContainer->pAdjacency = new DWORD[NumFaces * 3];

	//領域が確保できなければｴﾗｰ
	if (!pMeshContainer->pMaterials || !pMeshContainer->ppTextures || !pMeshContainer->pAdjacency) {
		hr = E_OUTOFMEMORY;
		DxSystemException(DxSystemException::OM_AH_MTRAL_TEX_ADJA_ALLOC_ERROR).ShowOriginalMessage();
		goto EXIT_FUNCTION;
	}

	//隣接ﾎﾟﾘｺﾞﾝのｲﾝﾃﾞｯｸｽをｺﾋﾟｰ
	memcpy(pMeshContainer->pAdjacency, pAdjacency, sizeof(DWORD) * NumFaces * 3);

	//ﾃｸｽﾁｬ領域のｾﾞﾛｸﾘｱ
	ZeroMemory(pMeshContainer->ppTextures, sizeof(LPDIRECT3DTEXTURE9) * pMeshContainer->NumMaterials);

	if (NumMaterials > 0) {

		//ﾏﾃﾘｱﾙ情報のｺﾋﾟｰ
		memcpy(pMeshContainer->pMaterials, pMaterials, sizeof(D3DXMATERIAL) * NumMaterials);

		for (iMaterial = 0; iMaterial < NumMaterials; iMaterial++) {

			//ｱﾝﾋﾞｴﾝﾄにﾃﾞｨﾌｭｰｽﾞ色を設定
			pMeshContainer->pMaterials[iMaterial].MatD3D.Ambient = pMeshContainer->pMaterials[iMaterial].MatD3D.Diffuse;

			//ﾃｸｽﾁｬﾌｧｲﾙ名が設定されていた場合
			if (pMeshContainer->pMaterials[iMaterial].pTextureFilename) {

				//ﾃｸｽﾁｬ読み込み
				if (FAILED(D3DXCreateTextureFromFile(pDevice, (LPCSTR)pMeshContainer->pMaterials[iMaterial].pTextureFilename, &pMeshContainer->ppTextures[iMaterial]))) {
					pMeshContainer->ppTextures[iMaterial] = nullptr;
				}
			}
		}
	}
	else {

		//ﾏﾃﾘｱﾙ情報が無い場合
		pMeshContainer->pMaterials[0].pTextureFilename = nullptr;
		ZeroMemory(&pMeshContainer->pMaterials[0].MatD3D, sizeof(D3DMATERIAL9));
		pMeshContainer->pMaterials[0].MatD3D.Diffuse.r = 0.5f;
		pMeshContainer->pMaterials[0].MatD3D.Diffuse.g = 0.5f;
		pMeshContainer->pMaterials[0].MatD3D.Diffuse.b = 0.5f;
		pMeshContainer->pMaterials[0].MatD3D.Ambient = pMeshContainer->pMaterials[0].MatD3D.Diffuse;
		pMeshContainer->pMaterials[0].MatD3D.Specular = pMeshContainer->pMaterials[0].MatD3D.Diffuse;
	}

	//ｽｷﾝ情報の確認
	if (pSkinInfo) {

		m_bSkinMesh = true;

		//ｽｷﾝ情報のｺﾋﾟｰ
		pMeshContainer->pSkinInfo = pSkinInfo;
		pSkinInfo->AddRef();

		//ｵﾘｼﾞﾅﾙﾒｯｼｭの格納
		pMeshContainer->pOrigMesh = pMesh;
		pMesh->AddRef();

		//ﾎﾞｰﾝの数を取得
		cBones = pSkinInfo->GetNumBones();

		//ﾎﾞｰﾝの数だけﾏﾄﾘｯｸｽ領域を確保
		pMeshContainer->pBoneOffsetMatrices = new D3DXMATRIX[cBones];

		if (!pMeshContainer->pBoneOffsetMatrices) {
			hr = E_OUTOFMEMORY;
			DxSystemException(DxSystemException::OM_AH_BONEOFFSETMATRICES_ALLOC_ERROR).ShowOriginalMessage();
			goto EXIT_FUNCTION;
		}

		//各ボーンの初期姿勢の情報をコピー
		for (iBone = 0; iBone < cBones; iBone++) {
			pMeshContainer->pBoneOffsetMatrices[iBone] = *(pMeshContainer->pSkinInfo->GetBoneOffsetMatrix(iBone));
		}

		//ﾊｰﾄﾞｳｪｱに最適化されたｽｷﾝﾒｯｼｭを作成
		hr = GenerateSkinnedMesh(pMeshContainer);

		if (FAILED(hr)) {
			DxSystemException(DxSystemException::OM_AH_GENERATE_SKINMESH_ERROR).ShowOriginalMessage();
			goto EXIT_FUNCTION;
		}
	}

	D3DVERTEXELEMENT9 VertexElement[65];
	pMeshContainer->MeshData.pMesh->GetDeclaration(VertexElement);

	//頂点宣言の作成
	if (!pMeshContainer->pVertexDeclaration) {
		hr = pDevice->CreateVertexDeclaration(VertexElement, &pMeshContainer->pVertexDeclaration);
		if (FAILED(hr)) {
			throw DxSystemException(DxSystemException::OM_XFILE_DECLARE_ERROR);
		}
	}

#ifdef _XFILE_CHECKER
	D3DVERTEXELEMENT9 ve[65] = { 0 };
	pMeshContainer->MeshData.pMesh->GetDeclaration(ve);

	OutputDebugString("\n\n");
	OutputDebugString(pMeshContainer->Name);
	OutputDebugString("\n");

	for (int i = 0; i < 65; i++) {
		if (ve[i].Type == 0x11) {
			break;
		}
		switch (ve[i].Type) {
		case 0: OutputDebugString("FLOAT1"); break;
		case 1: OutputDebugString("FLOAT2"); break;
		case 2: OutputDebugString("FLOAT3"); break;
		case 3: OutputDebugString("FLOAT4"); break;
		case 4: OutputDebugString("COLOR"); break;
		default: OutputDebugString("OTHER"); break;
		}
		switch (ve[i].Usage) {
		case 0: OutputDebugString(" : POSITION\n"); break;
		case 1:	 OutputDebugString(" : BLENDWEIGHT\n"); break;
		case 2:	 OutputDebugString(" : BLENDINDICES\n"); break;
		case 3:	 OutputDebugString(" : NORMAL\n"); break;
		case 4:	 OutputDebugString(" : PSIZE\n"); break;
		case 5:	 OutputDebugString(" : TEXCOORD\n"); break;
		case 6:	 OutputDebugString(" : TANGENT\n"); break;
		case 7:	 OutputDebugString(" : BINORMAL\n"); break;
		case 8:	 OutputDebugString(" : TESSFACTOR\n"); break;
		case 9:	 OutputDebugString(" : POSITIONT\n"); break;
		case 10: OutputDebugString(" : COLOR\n"); break;
		case 11: OutputDebugString(" : FOG\n"); break;
		case 12: OutputDebugString(" : DEPTH\n"); break;
		case 13: OutputDebugString(" : SAMPLE\n"); break;
		}
	}
#endif

	*ppNewMeshContainer = pMeshContainer;

	pMeshContainer = nullptr;

EXIT_FUNCTION:

	if (pDevice) {
		pDevice->Release();
		pDevice = nullptr;
	}

	if (pMeshContainer) {
		DestroyMeshContainer(pMeshContainer);
	}

	return hr;
}

//=============================================================================
// フレームの解放
// 引　数：LPD3DXFRAME 解放するフレームのポインタ
//=============================================================================
HRESULT __stdcall AllocateHierarchy::DestroyFrame(LPD3DXFRAME pFrameToFree)
{
	//ダウンキャスト
	D3DXFRAME_DERIVED *pFrame = static_cast<D3DXFRAME_DERIVED*>(pFrameToFree);

	//ﾌﾚｰﾑ名の解放
	if (pFrame->Name) {
		delete[] pFrame->Name;
		pFrame->Name = nullptr;
	}

	//ﾌﾚｰﾑの解放
	if (pFrame) {
		delete pFrame;
		pFrame = nullptr;
	}

	return D3D_OK;
}

//=============================================================================
// メッシュコンテナの解放
// 引　数：LPD3DXMESHCONTAINER 解放するメッシュコンテナのポインタ
//=============================================================================
HRESULT __stdcall AllocateHierarchy::DestroyMeshContainer(LPD3DXMESHCONTAINER pMeshContainerBase)
{
	D3DXMESHCONTAINER_DERIVED* pMeshContainer = (D3DXMESHCONTAINER_DERIVED*)pMeshContainerBase;

	//ﾒｯｼｭｺﾝﾃﾅ名の解放
	if (pMeshContainerBase->Name) {
		delete[] pMeshContainerBase->Name;
		pMeshContainerBase->Name = nullptr;
	}

	//隣接ﾎﾟﾘｺﾞﾝｲﾝﾃﾞｯｸｽの解放
	if (pMeshContainerBase->pAdjacency) {
		delete[] pMeshContainerBase->pAdjacency;
		pMeshContainerBase->pAdjacency = nullptr;
	}

	//ﾏﾃﾘｱﾙﾃﾞｰﾀの解放
	if (pMeshContainerBase->pMaterials) {
		delete[] pMeshContainerBase->pMaterials;
		pMeshContainerBase->pMaterials = nullptr;
	}

	//頂点宣言の解放
	if (pMeshContainer->pVertexDeclaration) {
		pMeshContainer->pVertexDeclaration->Release();
		pMeshContainer->pVertexDeclaration = nullptr;
	}

	//ﾎﾞｰﾝｵﾌｾｯﾄﾏﾄﾘｯｸｽの解放
	if (pMeshContainer->pBoneOffsetMatrices) {
		delete[] pMeshContainer->pBoneOffsetMatrices;
		pMeshContainer->pBoneOffsetMatrices = nullptr;
	}

	//ﾃｸｽﾁｬの解放
	if (pMeshContainer->ppTextures) {

		for (UINT i = 0; i < pMeshContainer->NumMaterials; i++) {

			if (pMeshContainer->ppTextures[i]) {
				pMeshContainer->ppTextures[i]->Release();
				pMeshContainer->ppTextures[i] = nullptr;
			}
		}

		delete[] pMeshContainer->ppTextures;
		pMeshContainer->ppTextures = nullptr;
	}

	//ﾎﾞｰﾝﾏﾄﾘｯｸｽの解放
	if (pMeshContainer->ppBoneMatrixPtrs) {
		delete[] pMeshContainer->ppBoneMatrixPtrs;
		pMeshContainer->ppBoneMatrixPtrs = nullptr;
	}

	//ﾎﾞｰﾝ結合ﾊﾞｯﾌｧの解放
	if (pMeshContainer->pBoneCombinationTable) {
		pMeshContainer->pBoneCombinationTable->Release();
		pMeshContainer->pBoneCombinationTable = nullptr;
	}

	//ﾒｯｼｭﾃﾞｰﾀの解放
	if (pMeshContainer->MeshData.pMesh) {
		pMeshContainer->MeshData.pMesh->Release();
		pMeshContainer->MeshData.pMesh = nullptr;
	}

	//ｽｷﾝ情報の解放
	if (pMeshContainer->pSkinInfo) {
		pMeshContainer->pSkinInfo->Release();
		pMeshContainer->pSkinInfo = nullptr;
	}

	//ｵﾘｼﾞﾅﾙﾒｯｼｭﾃﾞｰﾀの解放
	if (pMeshContainer->pOrigMesh) {
		pMeshContainer->pOrigMesh->Release();
		pMeshContainer->pOrigMesh = nullptr;
	}

	//ﾒｯｼｭｺﾝﾃﾅの解放
	if (pMeshContainer) {
		delete pMeshContainer;
		pMeshContainer = nullptr;
	}

	return S_OK;
}

//=============================================================================
// スキンメッシュかどうか調べる
//=============================================================================
bool AllocateHierarchy::IsSkinMesh() const
{
	return m_bSkinMesh;
}


//-----------------------------------------------------------------------------
// 名前のコピー（領域確保を含む）
// 引　数：LPCSTR コピー元の名前
// 　　　　LPSTR  コピー先のポインタ
//-----------------------------------------------------------------------------
HRESULT AllocateHierarchy::CopyName(LPCSTR Name, LPSTR* pNewName)
{
	UINT length;

	if (Name != nullptr) {
		length = (UINT)strlen(Name) + 1;
		*pNewName = new CHAR[length];
		if (*pNewName == nullptr) {
			return E_OUTOFMEMORY;
		}
		memcpy(*pNewName, Name, length * sizeof(CHAR));
	}
	else {
		*pNewName = nullptr;
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// 最適化スキンメッシュの作成
// 引　数：D3DXMESHCONTAINER_DERIVED* メッシュコンテナのポインタ
//-----------------------------------------------------------------------------
HRESULT AllocateHierarchy::GenerateSkinnedMesh(D3DXMESHCONTAINER_DERIVED* pMeshContainer)
{
	HRESULT hr = S_OK;

	//現在設定されているﾒｯｼｭを削除
	if (pMeshContainer->MeshData.pMesh) {
		pMeshContainer->MeshData.pMesh->Release();
		pMeshContainer->MeshData.pMesh = nullptr;
	}

	//ﾎﾞｰﾝ結合ﾊﾞｯﾌｧの削除
	if (pMeshContainer->pBoneCombinationTable) {
		pMeshContainer->pBoneCombinationTable->Release();
		pMeshContainer->pBoneCombinationTable = nullptr;
	}

	//頂点単位のﾌﾞﾚﾝﾄﾞの重みとﾎﾞｰﾝの組み合わせﾃｰﾌﾞﾙを適用した新しいﾒｯｼｭを返す
	hr = pMeshContainer->pSkinInfo->ConvertToBlendedMesh(
		pMeshContainer->pOrigMesh,
		D3DXMESH_MANAGED | D3DXMESHOPT_VERTEXCACHE,
		pMeshContainer->pAdjacency,
		nullptr, nullptr, nullptr,
		&pMeshContainer->numMaxFaceInfle,
		&pMeshContainer->numBoneCombinations,
		&pMeshContainer->pBoneCombinationTable,
		&pMeshContainer->MeshData.pMesh
	);

	if (FAILED(hr)) {
		goto EXIT_FUNCTION;
	}

EXIT_FUNCTION:

	return hr;
}





























