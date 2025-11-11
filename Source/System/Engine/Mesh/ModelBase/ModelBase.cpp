//*****************************************************************************
//
// モデルベースクラス（モデル管理クラスに本体を格納、モデルクラスへコピーして使う）
//
// ModelBase.cpp
//
// K_Yamaoka
//
// 2015/01/29
//
// 2017/09/27 CheckCrossPoint関数に「レイの逆方向にあるポリゴンの場合、交点無しとする」コードを追加
//
// 2017/09/28 GetDistance関数を追加
//
// 2020/09/01 シェーダー対応に変更
//
// 2021/11/02 SetCurrentPosition関数をSetCurrentIndex関数へ名称変更
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "ModelBase.h"

#include <cassert>

//=============================================================================
// コンストラクタ
//=============================================================================
ModelBase::ModelBase()
	: m_pAllocateHierarchy(nullptr)
	, m_pFrame(nullptr)
	, m_pAnimationController(nullptr)
	, m_bLoaded(false)
	, m_pAdjacencyData(nullptr)
	, m_pEffect(nullptr)
{
	D3DXMatrixIdentity(&m_matWorld);
}

//=============================================================================
// デストラクタ
//=============================================================================
ModelBase::~ModelBase()
{
	if (m_bLoaded) {
		D3DXFrameDestroy(*(m_pFrame->GetRootFramePointerAddress()), m_pAllocateHierarchy);
	}

	if (m_pAllocateHierarchy) {
		delete m_pAllocateHierarchy;
		m_pAllocateHierarchy = nullptr;
	}

	if (m_pFrame) {
		delete m_pFrame;
		m_pFrame = nullptr;
	}

	if (m_pAnimationController) {
		m_pAnimationController->Release();
		m_pAnimationController = nullptr;
	}

	if (m_pAdjacencyData) {
		delete[] m_pAdjacencyData;
		m_pAdjacencyData = nullptr;
	}

	if (m_pEffect) {
		m_pEffect->Release();
		m_pEffect = nullptr;
	}
}

//=============================================================================
// モデルデータの読み込み
// 引　数：LPDIRECT3DDEVICE9 ３Ｄデバイスへのポインタ
// 　　　　std::string       Ｘファイル名
//=============================================================================
void ModelBase::LoadModel(LPDIRECT3DDEVICE9 pDevice, std::string strFilename)
{
	//領域確保

	m_pAllocateHierarchy = new AllocateHierarchy();

	if (!m_pAllocateHierarchy) {
		throw DxSystemException(DxSystemException::OM_AH_ALLOC_ERROR);
	}

	m_pFrame = new Frame();

	if (!m_pFrame) {
		throw DxSystemException(DxSystemException::OM_AFRAME_ALLOC_ERROR);
	}

	//モデルデータの読み込み

	LPD3DXFRAME* ppFrameRoot = m_pFrame->GetRootFramePointerAddress();

	//階層構造を持ったメッシュの読み込み
	HRESULT hr = D3DXLoadMeshHierarchyFromX((LPCSTR)strFilename.c_str(), D3DXMESH_MANAGED, pDevice, m_pAllocateHierarchy, nullptr, ppFrameRoot, &m_pAnimationController);

	if (FAILED(hr)) {
		throw DxSystemException(DxSystemException::OM_MESH_LOAD_ERROR);
	}

	//ボーンマトリックスの設定
	hr = m_pFrame->SetupBoneMatrixPointers(*ppFrameRoot);

	if (FAILED(hr)) {
		throw DxSystemException(DxSystemException::OM_MESH_BONEMATRIX_SETUP_ERROR);
	}

	//シェーダー設定
	if (m_pAllocateHierarchy->IsSkinMesh()) {
		//シェーダー（スキンあり）の読み込み
		if (!m_pEffect) {

#ifdef _DEBUG
			hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_XFILE_SKIN_EFFECT), nullptr, nullptr, D3DXSHADER_DEBUG, nullptr, &m_pEffect, nullptr);
#else
			hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_XFILE_SKIN_EFFECT), nullptr, nullptr, 0, nullptr, &m_pEffect, nullptr);
#endif
			if (FAILED(hr)) {
				throw DxSystemException(DxSystemException::OM_XFILE_LOAD_RESOURCE_ERROR);
			}
		}
	}
	else {
		//シェーダー（スキンなし）の読み込み
		if (!m_pEffect) {

#ifdef _DEBUG
			hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_XFILE_NOSKIN_EFFECT), nullptr, nullptr, D3DXSHADER_DEBUG, nullptr, &m_pEffect, nullptr);
#else
			hr = D3DXCreateEffectFromResource(pDevice, nullptr, MAKEINTRESOURCE(FXID_XFILE_NOSKIN_EFFECT), nullptr, nullptr, 0, nullptr, &m_pEffect, nullptr);
#endif
			if (FAILED(hr)) {
				throw DxSystemException(DxSystemException::OM_XFILE_NOSKIN_LOAD_RESOURCE_ERROR);
			}
		}
	}

	//モデルデータロード済みフラグをオン
	m_bLoaded = true;
}

//=============================================================================
// アニメーションコントローラーのクローン
// 戻り値：クローンされたアニメーションコントローラーのポインタまたはnullptr
//=============================================================================
ID3DXAnimationController* ModelBase::CloneAnimationController()
{
	if (!m_pAnimationController) {
		return nullptr;
	}

	UINT maxNumAnimationOutputs = m_pAnimationController->GetMaxNumAnimationOutputs();
	UINT maxNumAnimationSets = m_pAnimationController->GetMaxNumAnimationSets();
	UINT maxNumTracks = m_pAnimationController->GetMaxNumTracks();
	UINT maxNumEvents = m_pAnimationController->GetMaxNumEvents();

	ID3DXAnimationController* pCloneAnimationController;

	HRESULT hr = m_pAnimationController->CloneAnimationController(maxNumAnimationOutputs, maxNumAnimationSets, maxNumTracks, maxNumEvents, &pCloneAnimationController);

	if (FAILED(hr)) {
		return nullptr;
	}

	return pCloneAnimationController;
}

//=============================================================================
// モデルデータの描画
// 引　数：LPD3DXMATRIX      ワールド座標変換マトリックスへのポインタ
// 　　　　LPD3DXMATRIX      ビュープロジェクション変換マトリックスへのポインタ
// 　　　　D3DCOLORVALUE*    アンビエントライトの色へのポインタ
// 　　　　D3DLIGHT9*        ライトへのポインタ
//=============================================================================
void ModelBase::Draw(LPD3DXMATRIX pMatWorld, LPD3DXMATRIX pMatVP, D3DCOLORVALUE* pAmbient, D3DLIGHT9* pLight)
{
	//アンビエントライトの色を渡す
	m_pEffect->SetValue("gAmbientColor", pAmbient, sizeof(D3DCOLORVALUE));

	//ディレクショナルライトの色と方向を渡す
	m_pEffect->SetValue("gLightColor", &pLight->Diffuse, sizeof(D3DCOLORVALUE));
	m_pEffect->SetValue("gLightDir", &pLight->Direction, sizeof(D3DVECTOR));

	//テクニックを設定
	if (m_pAllocateHierarchy->IsSkinMesh()) {
		m_pEffect->SetTechnique("XFileSkinTec");
	}
	else {
		m_pEffect->SetTechnique("XFileNoSkinTec");
	}

	//ﾏﾄﾘｯｸｽの更新
	m_pFrame->UpdateFrameMatrices(*(m_pFrame->GetRootFramePointerAddress()), pMatWorld);

	//全ﾌﾚｰﾑの描画
	m_pFrame->DrawFrame(*(m_pFrame->GetRootFramePointerAddress()), pMatVP, m_pEffect);
}

//=============================================================================
// ルートフレームの変換マトリックスを取得
// 引　数：LPD3DXMATRIX 変換マトリックスのポインタ
//=============================================================================
void ModelBase::GetMatrixFromRootFrame(LPD3DXMATRIX pMatrix)
{
	D3DXFRAME_DERIVED* pFrame = *((D3DXFRAME_DERIVED**)m_pFrame->GetRootFramePointerAddress());

	if (!pFrame) {
		throw DxSystemException(DxSystemException::OM_MESH_GETMATRIX_ROOTFRAME_ERROR);
	}

	*pMatrix = pFrame->CombinedTransformationMatrix;
}

//=============================================================================
// フレーム名から変換マトリックスを取得
// 引　数：std::string  フレーム名
// 　　　　LPD3DXMATRIX 変換マトリックスのポインタ
//=============================================================================
void ModelBase::GetMatrixFromFrameName(std::string strFrameName, LPD3DXMATRIX pMatrix)
{
	D3DXFRAME_DERIVED* pFrame = (D3DXFRAME_DERIVED*)m_pFrame->GetFrame(strFrameName.c_str());

	if (!pFrame) {
		throw DxSystemException(DxSystemException::OM_MESH_GETMATRIX_FRAMENAME_ERROR);
	}

	*pMatrix = pFrame->CombinedTransformationMatrix;
}

//=============================================================================
// ルートフレームのポインタを取得
// 戻り値：ルートフレームのポインタ
//=============================================================================
LPD3DXMESH ModelBase::GetRootMesh()
{
	return m_pFrame->GetRootMesh();
}

//=============================================================================
// 隣接データ配列の作成
//=============================================================================
void ModelBase::CreateAdjacencyData()
{
	//メッシュの取得
	LPD3DXMESH pMesh = m_pFrame->GetRootMesh();

	//頂点情報のバイト数を取得
	DWORD numVertexBytes = pMesh->GetNumBytesPerVertex();

	//ポリゴン数の取得
	DWORD numFaces = pMesh->GetNumFaces();

	//隣接データ構造体の配列を作成
	m_pAdjacencyData = new AdjacencyData[numFaces];

	if (!m_pAdjacencyData) {
		throw DxSystemException(DxSystemException::OM_MESH_ALOC_ADJACENCYDATA_ERROR);
	}

	//隣接データ配列の作成（DirectX用）
	DWORD* pAdjacency = new DWORD[numFaces * 3];

	if (!pAdjacency) {
		throw DxSystemException(DxSystemException::OM_MESH_ALOC_ADJACENCY_ERROR);
	}

	HRESULT hResult;

	//隣接データの取得
	hResult = pMesh->GenerateAdjacency(0.01f, pAdjacency);

	if (FAILED(hResult)) {
		delete[] pAdjacency;
		throw DxSystemException(DxSystemException::OM_MESH_GENERATE_ADJACENCY_ERROR);
	}

	//頂点バッファへのアクセス

	void* pVertex; //頂点情報の先頭番地

	//頂点バッファのロック
	hResult = pMesh->LockVertexBuffer(0, &pVertex);

	if (FAILED(hResult)) {
		delete[] pAdjacency;
		throw DxSystemException(DxSystemException::OM_MESH_LOCK_VB_ERROR);
	}

	//インデックスバッファへのアクセス

	void* pIndex; //インデックス情報の先頭番地

	//インデックスバッファのロック
	hResult = pMesh->LockIndexBuffer(0, &pIndex);

	if (FAILED(hResult)) {
		delete[] pAdjacency;
		throw DxSystemException(DxSystemException::OM_MESH_LOCK_IB_ERROR);
	}

	//隣接データ配列へデータをセット

	for (DWORD i = 0; i < numFaces; i++) {

		//ポリゴンを構成する頂点座標を取得

		m_pAdjacencyData[i].vertices[0] = *(D3DXVECTOR3*)((BYTE*)pVertex + (numVertexBytes * ((WORD*)pIndex)[i * 3]));
		m_pAdjacencyData[i].vertices[1] = *(D3DXVECTOR3*)((BYTE*)pVertex + (numVertexBytes * ((WORD*)pIndex)[i * 3 + 1]));
		m_pAdjacencyData[i].vertices[2] = *(D3DXVECTOR3*)((BYTE*)pVertex + (numVertexBytes * ((WORD*)pIndex)[i * 3 + 2]));

		//法線の計算
		m_pAdjacencyData[i].normal = D3DXComputeNormal(m_pAdjacencyData[i].vertices);

		//隣接データの移行//
		m_pAdjacencyData[i].adjacency[0] = pAdjacency[i * 3];
		m_pAdjacencyData[i].adjacency[1] = pAdjacency[i * 3 + 1];
		m_pAdjacencyData[i].adjacency[2] = pAdjacency[i * 3 + 2];
	}

	//バッファのアンロック
	pMesh->UnlockIndexBuffer();
	pMesh->UnlockVertexBuffer();

	delete[] pAdjacency;
}

//=============================================================================
// 地形上の高さを設定する
// 戻り値：成功 true、失敗 false
// 引　数：D3DXVECTOR3* 現在位置座標へのポインタ
// 備　考：成功した場合、pVecPosの[y座標]が更新される
// 　　　　動かないオブジェクトにはこちらを使う
//=============================================================================
bool ModelBase::SetPosition(D3DXVECTOR3* pVecPos)
{
	assert(m_pAdjacencyData);

	//メッシュの取得
	LPD3DXMESH pMesh = m_pFrame->GetRootMesh();

	D3DXVECTOR3 vecRay(0, -1, 0); //レイの方向
	D3DXVECTOR3 vecCrossPoint;    //交点

	DWORD numFaces = pMesh->GetNumFaces(); //ポリゴン数

	DWORD i;

	//全ポリゴンとレイの交差判定を行う
	for (i = 0; i < numFaces; i++) {
		if (CheckCrossPoint(pVecPos, &vecRay, &m_pAdjacencyData[i].normal, m_pAdjacencyData[i].vertices, &vecCrossPoint)) {
			break;
		}
	}

	//交点があった場合、Ｙ座標を更新
	if (i < numFaces) {
		pVecPos->y = vecCrossPoint.y;
		return true;
	}
	else {
		return false;
	}
}

//=============================================================================
// 現在上に乗っているポリゴンインデックスをセット
// 戻り値：成功 true、失敗 false
// 引　数：D3DXVECTOR3* 現在位置座標へのポインタ
// 　　　　DWORD*       現在のポリゴンインデックスへのポインタ
// 備　考：成功後、UpdatePositionで高さが変化する
//=============================================================================
bool ModelBase::SetCurrentIndex(const D3DXVECTOR3* pVecPos, DWORD* pCurrentIndex)
{
	assert(m_pAdjacencyData);

	//メッシュの取得
	LPD3DXMESH pMesh = m_pFrame->GetRootMesh();

	D3DXVECTOR3 vecRay(0, -1, 0); //レイの方向
	D3DXVECTOR3 vecCrossPoint;      //交点

	DWORD numFaces = pMesh->GetNumFaces(); //ポリゴン数

	DWORD i;

	//全ポリゴンとレイの交差判定を行う
	for (i = 0; i < numFaces; i++) {
		if (CheckCrossPoint(pVecPos, &vecRay, &m_pAdjacencyData[i].normal, m_pAdjacencyData[i].vertices, &vecCrossPoint)) {
			break;
		}
	}

	//交点があった場合、交差したポリゴンインデックスを格納
	if (i < numFaces) {
		*pCurrentIndex = i;
		return true;
	}
	else {
		*pCurrentIndex = 0xffffffff;
		return false;
	}
}

//=============================================================================
// 隣接ポリゴンチェック用フラグのクリア
// 備　考：UpdatePositionを呼び出す前に必ず呼び出さなければならない
//=============================================================================
void ModelBase::ClearCheckFlag()
{
	assert(m_pAdjacencyData);

	//メッシュの取得
	LPD3DXMESH pMesh = m_pFrame->GetRootMesh();

	DWORD numFaces = pMesh->GetNumFaces();

	for (DWORD i = 0; i < numFaces; i++) {
		m_pAdjacencyData[i].bChecked = false;
	}
}

//=============================================================================
// 移動後の位置を設定する
// 戻り値：成功 true、失敗 false
// 引　数：D3DXVECTOR3* 現在位置座標へのポインタ
// 　　　　DWORD        現在のポリゴンインデックス
// 　　　　DWORD*       現在のポリゴンインデックスへのポインタ
// 備　考：成功した場合、pVecPosの[y座標]が更新される
//=============================================================================
bool ModelBase::UpdatePosition(D3DXVECTOR3* pVecPos, DWORD currentIndex, DWORD* pSourceIndex)
{
	assert(m_pAdjacencyData);

	//隣接するポリゴンが無い場合
	if (currentIndex == 0xffffffff) {
		return false;
	}

	//チェック済み
	if (m_pAdjacencyData[currentIndex].bChecked) {
		return false;
	}

	//チェックを入れる
	m_pAdjacencyData[currentIndex].bChecked = true;

	D3DXVECTOR3 vecRay(0, -1, 0);
	D3DXVECTOR3 vecCrossPoint;

	//レイとの交点を調べる
	if (CheckCrossPoint(pVecPos, &vecRay, &m_pAdjacencyData[currentIndex].normal, m_pAdjacencyData[currentIndex].vertices, &vecCrossPoint)) {
		*pSourceIndex = currentIndex; //現在位置の更新
		pVecPos->y = vecCrossPoint.y; //高さの更新
		return true;
	}

	//隣接ポリゴンのチェック
	for (int i = 0; i < 3; i++) {
		if (UpdatePosition(pVecPos, m_pAdjacencyData[currentIndex].adjacency[i], pSourceIndex)) {
			return true;
		}
	}

	return false;
}

//=============================================================================
// バウンディングスフィア半径の取得
// 戻り値：バウンディングスフィアの半径
//=============================================================================
float ModelBase::GetBoundingShpereRadius()
{
	LPD3DXMESH pMesh = m_pFrame->GetRootMesh();

	DWORD numVertices = pMesh->GetNumVertices();

	UINT stride = D3DXGetFVFVertexSize(pMesh->GetFVF());

	D3DXVECTOR3 center;

	float radius = 0;

	void* pVertex;

	HRESULT hResult = pMesh->LockVertexBuffer(0, &pVertex);

	if (SUCCEEDED(hResult)) {

		D3DXComputeBoundingSphere((D3DXVECTOR3*)pVertex, numVertices, stride, &center, &radius);

		pMesh->UnlockVertexBuffer();
	}
	else {
		DxSystemException(DxSystemException::OM_MESH_LOCK_VB_ERROR).ShowOriginalMessage();
	}

	return radius;
}

//=============================================================================
// バウンディングボックス最大値の取得
// 戻り値：バウンディングボックス最大値
//=============================================================================
D3DXVECTOR3 ModelBase::GetBoundingBoxMax()
{
	LPD3DXMESH pMesh = m_pFrame->GetRootMesh();

	DWORD numVertices = pMesh->GetNumVertices();

	UINT stride = D3DXGetFVFVertexSize(pMesh->GetFVF());

	D3DXVECTOR3 min = D3DXVECTOR3(0, 0, 0);
	D3DXVECTOR3 max = D3DXVECTOR3(0, 0, 0);

	void* pVertex;

	HRESULT hResult = pMesh->LockVertexBuffer(0, &pVertex);

	if (SUCCEEDED(hResult)) {

		D3DXComputeBoundingBox((D3DXVECTOR3*)pVertex, numVertices, stride, &min, &max);

		pMesh->UnlockVertexBuffer();
	}
	else {
		DxSystemException(DxSystemException::OM_MESH_LOCK_VB_ERROR).ShowOriginalMessage();
	}

	return max;
}

//=============================================================================
// バウンディングボックス最小値の取得
// 戻り値：バウンディングボックス最小値
//=============================================================================
D3DXVECTOR3 ModelBase::GetBoundingBoxMin()
{
	LPD3DXMESH pMesh = m_pFrame->GetRootMesh();

	DWORD numVertices = pMesh->GetNumVertices();

	UINT stride = D3DXGetFVFVertexSize(pMesh->GetFVF());

	D3DXVECTOR3 min = D3DXVECTOR3(0, 0, 0);
	D3DXVECTOR3 max = D3DXVECTOR3(0, 0, 0);

	void* pVertex;

	HRESULT hResult = pMesh->LockVertexBuffer(0, &pVertex);

	if (SUCCEEDED(hResult)) {

		D3DXComputeBoundingBox((D3DXVECTOR3*)pVertex, numVertices, stride, &min, &max);

		pMesh->UnlockVertexBuffer();
	}
	else {
		DxSystemException(DxSystemException::OM_MESH_LOCK_VB_ERROR).ShowOriginalMessage();
	}

	return min;
}

//===========================================================================
// レイとポリゴンの交点までの距離を取得
// 戻り値：レイとポリゴンの交点距離
// 引　数：D3DXVECTOR3& 現在位置座標への参照
//		　 D3DXVECTOR3& キャラクターの向きへの参照
// 　　　　float        チェックする最大距離
//===========================================================================
float ModelBase::GetDistance(const D3DXVECTOR3 &position, const D3DXVECTOR3 &ray, const float limitDistance)
{
	assert(m_pAdjacencyData);

	//メッシュ取得
	LPD3DXMESH pMesh = m_pFrame->GetRootMesh();

	//ポリゴンの数
	DWORD NumFaces = pMesh->GetNumFaces();

	//交点
	D3DXVECTOR3 vecCrossPoint;

	//交点との距離ベクトル
	D3DXVECTOR3 distance;

	//距離
	float saveLength = limitDistance;

	//距離の一時保存
	float length;

	//全ポリゴンとレイの交差判定
	for (DWORD i = 0; i < NumFaces; i++) {

		if (CheckCrossPoint(&position, &ray, &m_pAdjacencyData[i].normal, m_pAdjacencyData[i].vertices, &vecCrossPoint)) {

			distance = vecCrossPoint - position;

			length = D3DXVec3Length(&distance);

			if (length < saveLength) {
				saveLength = length;
			}
		}
	}

	return saveLength;
}



//*****************************************************************************
// private関数
//*****************************************************************************


//-----------------------------------------------------------------------------
// 衝突判定　レイとポリゴンの交点を求める
// 戻り値：交差している場合 true、していない場合 false
// 引　数：const D3DXVECTOR3* レイの発射点へのポインタ
// 　　　　const D3DXVECTOR3* レイの方向へのポインタ（単位ベクトル）
// 　　　　const D3DXVECTOR3* ポリゴンの法線へのポインタ
// 　　　　const D3DXVECTOR3  ポリゴンの頂点配列
// 　　　　D3DXVECTOR3*       交点があった場合、交点の座標を格納する変数へのポインタ
// 備　考：表ポリゴンのみ判定を行う。
//-----------------------------------------------------------------------------
bool ModelBase::CheckCrossPoint(const D3DXVECTOR3* pVecPos, const D3DXVECTOR3* pVecRay, const D3DXVECTOR3* pVecNormal, const D3DXVECTOR3 vPolygon[3], D3DXVECTOR3* pVecCrossPoint)
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

	vecWork = vP - *pVecPos;
	D3DXVECTOR3 vecNor = D3DXVec3Normalize(&vecWork);

	//レイの逆方向にあるポリゴンの場合、交点無しとする
	if (D3DXVec3Dot(pVecRay, &vecNor) <= 0) {
		return false;
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

