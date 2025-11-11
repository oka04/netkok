//*****************************************************************************
//
// Frame.cpp
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
// 2020/09/01 シェーダー対応に変更
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "Frame.h"

//=============================================================================
// コンストラクタ
//=============================================================================
Frame::Frame()
	: m_pFrameRoot(nullptr)
{

}

//=============================================================================
// フレームへボーンマトリックスを設定
// 戻り値：成功 S_OK　失敗 エラーコード
// 引　数：LPD3DXFRAME フレームへのポインタ
//=============================================================================
HRESULT Frame::SetupBoneMatrixPointers(LPD3DXFRAME pFrame)
{
	HRESULT hr;

	//ﾌﾚｰﾑに設定されてるﾒｯｼｭｺﾝﾃﾅへﾎﾞｰﾝﾏﾄﾘｯｸｽを設定
	if (pFrame->pMeshContainer) {

		hr = SetupBoneMatrixPointersOnMesh(pFrame->pMeshContainer);

		if (FAILED(hr)) {
			DxSystemException(DxSystemException::OM_FRAME_SETUPBONEMATRIX_ERROR).ShowOriginalMessage();
			return hr;
		}
	}

	//ﾌﾚｰﾑに設定されてる兄弟ﾌﾚｰﾑへﾎﾞｰﾝﾏﾄﾘｯｸｽを設定
	if (pFrame->pFrameSibling) {

		hr = SetupBoneMatrixPointers(pFrame->pFrameSibling);

		if (FAILED(hr)) {
			DxSystemException(DxSystemException::OM_FRAME_SETUPBONEMATRIX_SIBLING_ERROR).ShowOriginalMessage();
			return hr;
		}
	}

	//ﾌﾚｰﾑに設定されてる最初の子ﾌﾚｰﾑへﾎﾞｰﾝﾏﾄﾘｯｸｽを設定
	if (pFrame->pFrameFirstChild) {

		hr = SetupBoneMatrixPointers(pFrame->pFrameFirstChild);

		if (FAILED(hr)) {
			DxSystemException(DxSystemException::OM_FRAME_SETUPBONEMATRIX_CHILD_ERROR).ShowOriginalMessage();
			return hr;
		}
	}

	return S_OK;
}

//=============================================================================
// フレーム行列の更新（親子関係の設定）
// 引　数：LPD3DXFRAME  フレームへのポインタ
// 　　　　LPD3DXMATRIX 親フレームのマトリックス
//=============================================================================
void Frame::UpdateFrameMatrices(LPD3DXFRAME pFrameBase, LPD3DXMATRIX pParentMatrix)
{
	D3DXFRAME_DERIVED* pFrame = (D3DXFRAME_DERIVED*)pFrameBase;

	if (pParentMatrix) {
		//親ﾌﾚｰﾑに設定されたﾏﾄﾘｯｸｽがあれば乗算する
		D3DXMatrixMultiply(&pFrame->CombinedTransformationMatrix, &pFrame->TransformationMatrix, pParentMatrix);
	}
	else {
		pFrame->CombinedTransformationMatrix = pFrame->TransformationMatrix;
	}

	//兄弟ﾌﾚｰﾑの設定(再帰的に呼び出し、末端まで設定する)
	if (pFrame->pFrameSibling) {
		UpdateFrameMatrices(pFrame->pFrameSibling, pParentMatrix);
	}

	//子ﾌﾚｰﾑの設定(再帰的に呼び出し、末端まで設定する)
	if (pFrame->pFrameFirstChild) {
		UpdateFrameMatrices(pFrame->pFrameFirstChild, &pFrame->CombinedTransformationMatrix);
	}
}

//=============================================================================
// フレームの描画
// 引　数：LPD3DXFRAME       フレームへのポインタ
// 　　　　LPD3DXMATRIX      ビュープロジェクション変換マトリックスへのポインタ
//         ID3DXEffect*      エフェクトへのポインタ
//=============================================================================
void Frame::DrawFrame(LPD3DXFRAME pFrame, LPD3DXMATRIX pMatVP, ID3DXEffect* pEffect)
{
	LPD3DXMESHCONTAINER pMeshContainer;

	//ﾌﾚｰﾑからﾒｯｼｭｺﾝﾃﾅを取り出す
	pMeshContainer = pFrame->pMeshContainer;

	while (pMeshContainer) {

		//ﾒｯｼｭの描画
		DrawMeshContainer(pMeshContainer, pFrame, pMatVP, pEffect);

		//次のﾒｯｼｭｺﾝﾃﾅへ移動
		pMeshContainer = pMeshContainer->pNextMeshContainer;
	}

	//兄弟ﾌﾚｰﾑが存在した場合、再帰的に描画
	if (pFrame->pFrameSibling) {
		DrawFrame(pFrame->pFrameSibling, pMatVP, pEffect);
	}

	//子ﾌﾚｰﾑが存在した場合、再帰的に描画
	if (pFrame->pFrameFirstChild) {
		DrawFrame(pFrame->pFrameFirstChild, pMatVP, pEffect);
	}
}

//=============================================================================
// ルートフレームのポインタのアドレス取得
// 戻り値：ルートフレームのポインタのアドレス
//=============================================================================
LPD3DXFRAME* Frame::GetRootFramePointerAddress()
{
	return &m_pFrameRoot;
}

//=============================================================================
// フレーム名からフレームを取得
// 戻り値：フレームのアドレス
// 引　数：LPCSTR フレーム名
//=============================================================================
LPD3DXFRAME Frame::GetFrame(LPCSTR FrameName)
{
	return D3DXFrameFind(m_pFrameRoot, FrameName);
}

//=============================================================================
// ルートフレーム内のメッシュへのポインタを取得
// 戻り値：ルートフレーム内のメッシュへのポインタ
//=============================================================================
LPD3DXMESH Frame::GetRootMesh() const
{
	return m_pFrameRoot->pMeshContainer->MeshData.pMesh;
}




//*****************************************************************************
// private関数
//*****************************************************************************


//-----------------------------------------------------------------------------
// ボーン変換マトリックスのポインタをメッシュに設定
// 戻り値：成功 S_OK　失敗 エラーコード
// 引　数：LPD3DXMESHCONTAINER メッシュコンテナのポインタ
//-----------------------------------------------------------------------------
HRESULT Frame::SetupBoneMatrixPointersOnMesh(LPD3DXMESHCONTAINER pMeshContainerBase)
{
	D3DXFRAME_DERIVED* pFrame;

	D3DXMESHCONTAINER_DERIVED* pMeshContainer = (D3DXMESHCONTAINER_DERIVED*)pMeshContainerBase;

	if (pMeshContainer->pSkinInfo) {

		//ﾎﾞｰﾝ数の取得
		UINT cBones = pMeshContainer->pSkinInfo->GetNumBones();

		//ﾎﾞｰﾝﾏﾄﾘｯｸｽ格納用ﾎﾟｲﾝﾀ配列の領域確保
		pMeshContainer->ppBoneMatrixPtrs = new D3DXMATRIX*[cBones];

		if (!pMeshContainer->ppBoneMatrixPtrs) {
			return E_OUTOFMEMORY;
		}

		for (UINT i = 0; i < cBones; i++) {

			//ボーン名からボーンのフレームを探す
			pFrame = (D3DXFRAME_DERIVED*)D3DXFrameFind(m_pFrameRoot, pMeshContainer->pSkinInfo->GetBoneName(i));

			if (!pFrame) {
				return E_FAIL;
			}

			//各ボーンの変換行列のアドレスを登録
			pMeshContainer->ppBoneMatrixPtrs[i] = &pFrame->CombinedTransformationMatrix;
		}
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// メッシュの描画
// 引　数：LPD3DXMESHCONTAINER メッシュコンテナのポインタ
// 　　　　LPD3DXFRAME         フレームのポインタ
// 　　　　LPD3DXMATRIX      ビュープロジェクション変換マトリックスへのポインタ
//         ID3DXEffect*      エフェクトへのポインタ
//-----------------------------------------------------------------------------
void Frame::DrawMeshContainer(LPD3DXMESHCONTAINER pMeshContainerBase, LPD3DXFRAME pFrameBase, LPD3DXMATRIX pMatVP, ID3DXEffect* pEffect)
{
	D3DXMESHCONTAINER_DERIVED* pMeshContainer = (D3DXMESHCONTAINER_DERIVED*)pMeshContainerBase;
	D3DXFRAME_DERIVED* pFrame = (D3DXFRAME_DERIVED*)pFrameBase;

	//ｽｷﾝ情報の確認
	if (pMeshContainer->pSkinInfo) {

		//ﾊｰﾄﾞｳｪｱ処理

		//前の属性IDの初期化(0xffffffff)
		DWORD AttribIdPrev = UNUSED32;

		//ボーンと頂点の対応表配列を準備
		LPD3DXBONECOMBINATION pBoneComb = reinterpret_cast<LPD3DXBONECOMBINATION>(pMeshContainer->pBoneCombinationTable->GetBufferPointer());

		//対応表に登録されたサブセットをすべてチェック
		for (UINT subset = 0; subset < pMeshContainer->numBoneCombinations; subset++) {

			//ﾌﾞﾚﾝﾄﾞ数(他のﾎﾞｰﾝに影響する数)の初期化
			UINT NumBlend = 0;

			//ﾌﾞﾚﾝﾄﾞ数をﾁｪｯｸ
			for (DWORD i = 0; i < pMeshContainer->numMaxFaceInfle; i++) {
				if (pBoneComb[subset].BoneId[i] != UINT_MAX) {
					NumBlend = i;
				}
			}

			//１つの頂点に影響を与えるボーンの変換行列（今使っているモデルは最大４のため固定している）
			D3DXMATRIX matWorld[4];

			//初期値は単位行列
			for (int i = 0; i < 4; i++) {
				D3DXMatrixIdentity(&matWorld[i]);
			}

			//１つの頂点に影響を与えるボーンの最大数分ループ
			for (DWORD i = 0; i < pMeshContainer->numMaxFaceInfle; i++) {

				//ボーン番号の取得
				UINT boneNo = pBoneComb[subset].BoneId[i];

				//ボーン番号がUINT_MAXの場合は最大数より少ないということ
				if (boneNo != UINT_MAX) {

					D3DXMATRIXA16 matTemp;

					//元の姿勢に現在の行列を乗算したものを計算
					D3DXMatrixMultiply(&matTemp, &pMeshContainer->pBoneOffsetMatrices[boneNo], pMeshContainer->ppBoneMatrixPtrs[boneNo]);

					//行列の格納
					matWorld[i] = matTemp;
				}
			}

			//ボーンの変換行列を送る
			pEffect->SetMatrixArray("gMatW", matWorld, 4);

			//ビュープロジェクション変換行列を送る
			pEffect->SetMatrix("gMatVP", pMatVP);

			//１つの頂点に影響を与えるボーンの数を送る
			pEffect->SetInt("gNumMaxInfle", pMeshContainer->numMaxFaceInfle);

			//属性が異なっていた場合、ﾏﾃﾘｱﾙとﾃｸｽﾁｬを変更
			if (AttribIdPrev != pBoneComb[subset].AttribId || AttribIdPrev == UNUSED32) {

				//マテリアルのディフューズとアンビエントを渡す
				pEffect->SetValue("gMaterialDiffuse", &pMeshContainer->pMaterials[pBoneComb[subset].AttribId].MatD3D.Diffuse, sizeof(D3DCOLORVALUE));
				pEffect->SetValue("gMaterialAmbient", &pMeshContainer->pMaterials[pBoneComb[subset].AttribId].MatD3D.Ambient, sizeof(D3DCOLORVALUE));

				//テクスチャーを渡す
				pEffect->SetTexture("gTexture", pMeshContainer->ppTextures[pBoneComb[subset].AttribId]);

				//新しい属性を保管
				AttribIdPrev = pBoneComb[subset].AttribId;
			}

			UINT numPass;
			pEffect->Begin(&numPass, 0);

			//パスを決める
			bool bUseTexture = false;
			bool bUseColor = false;
			UINT passNo = 0;

			if (pMeshContainer->ppTextures[pBoneComb[subset].AttribId]) {
				bUseTexture = true;
			}

			DWORD fvf = pMeshContainer->MeshData.pMesh->GetFVF();

			if (fvf & D3DFVF_DIFFUSE) {
				bUseColor = true;
			}

			if ((fvf & D3DFVF_XYZB1) == D3DFVF_XYZB1) {
				if (bUseColor) {
					if (bUseTexture) passNo = 2;
					else             passNo = 3;
				}
				else {
					if (bUseTexture) passNo = 0;
					else             passNo = 1;
				}
			}
			else if ((fvf & D3DFVF_XYZB3) == D3DFVF_XYZB3) {
				if (bUseColor) {
					if (bUseTexture) passNo = 6;
					else             passNo = 7;
				}
				else {
					if (bUseTexture) passNo = 8;
					else             passNo = 9;
				}
			}
			else if ((fvf & D3DFVF_XYZB2) == D3DFVF_XYZB2) {
				if (bUseTexture) passNo = 4;
				else             passNo = 5;
			}

			pEffect->BeginPass(passNo);

			//ｻﾌﾞｾｯﾄの描画
			pMeshContainer->MeshData.pMesh->DrawSubset(subset);

			pEffect->EndPass();
			pEffect->End();
		}
	}
	else {

		//ｽｷﾝ情報が無い場合(通常の描画方法と同じ)

		//ワールド座標変換行列を渡す
		D3DXMATRIX matWorld = pFrame->CombinedTransformationMatrix;
		pEffect->SetMatrix("gMatW", &matWorld);

		//ワールドビュー射影変換行列を渡す
		matWorld = matWorld * *pMatVP;
		pEffect->SetMatrix("gMatWVP", &matWorld);

		for (UINT i = 0; i < pMeshContainer->NumMaterials; i++) {

			UINT numPass;
			pEffect->Begin(&numPass, 0);

			//マテリアルのディフューズとアンビエントを渡す
			pEffect->SetValue("gMaterialDiffuse", &pMeshContainer->pMaterials[i].MatD3D.Diffuse, sizeof(D3DCOLORVALUE));
			pEffect->SetValue("gMaterialAmbient", &pMeshContainer->pMaterials[i].MatD3D.Ambient, sizeof(D3DCOLORVALUE));

			//テクスチャーを渡す
			pEffect->SetTexture("gTexture", pMeshContainer->ppTextures[i]);
			if (pMeshContainer->ppTextures[i]) {
				pEffect->BeginPass(0);
			}
			else {
				pEffect->BeginPass(1);
			}

			pMeshContainer->MeshData.pMesh->DrawSubset(i);

			pEffect->EndPass();
			pEffect->End();
		}
	}
}



















