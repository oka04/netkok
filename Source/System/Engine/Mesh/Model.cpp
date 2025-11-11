//*****************************************************************************
//
// モデルクラス（ユーザがモデルデータを扱う際に使用する）
//
// Model.cpp
//
// K_Yamaoka
//
// 2015/01/29
//
// 2017/04/12 アニメーション付きモデルを二重に作成するとメモリーリークが発生する問題を修正
//
// 2017/09/28 GetDistance関数を追加
//
// 2019/08/20 トラックが１つしかないアニメーションに対応するため以下の関数を追加
//            SetAdvanceTimeOneTrackAnimation ChangeAnimationOneTrackAnimation
//            LoopAnimationOneTrackAnimation WaitFinishAnimationOneTrackAnimation
//            GetNowFrameOneTrackAnimation
//
// 2020/09/01 シェーダー対応に変更
//
// 2021/10/10 ChangeAnimationSet関数のトラックスピード設定を削除
//            GetCurrentPosition関数をSetCurrentPosition関数へ名称変更
//
// 2021/11/02 SetCurrentPosition関数をSetCurrentIndex関数へ名称変更
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "Model.h"

#include <cassert>

//=============================================================================
// コンストラクタ
//=============================================================================
Model::Model()
	: m_pModelBase(nullptr)
	, m_pAnimationController(nullptr)
	, m_nowAnimationSet(0)
	, m_numAnimationSets(0)
	, m_pAnimationSets(nullptr)
	, m_advanceTime(0)
	, m_bOneTrackAnimation(false)
	, m_bEndLoop(true)
	, m_startFrame(0)
	, m_endFrame(0)
{
	D3DXMatrixIdentity(&m_matWorld);
}

//=============================================================================
// デストラクタ
//=============================================================================
Model::~Model()
{
	if (m_pAnimationSets) {
		delete[] m_pAnimationSets;
		m_pAnimationSets = nullptr;
	}
}

//=============================================================================
// モデルベースクラスの取得
// 引　数：ModelBase* モデルベースクラスへのポインタ
//=============================================================================
void Model::SetModel(ModelBase* pModelBase)
{
	m_pModelBase = pModelBase;
}

//=============================================================================
// アニメーションコントローラーの取得
// 引　数：ID3DXAnimationController* アニメーションコントローラーへのポインタ
//=============================================================================
bool Model::SetAnimationController(ID3DXAnimationController* pAnimationController)
{
	m_pAnimationController = pAnimationController;

	if (m_pAnimationController) {

		m_numAnimationSets = m_pAnimationController->GetNumAnimationSets();

		if (m_numAnimationSets >= 1) {

			if (m_pAnimationSets) {
				delete[] m_pAnimationSets;
				m_pAnimationSets = nullptr;
			}

			m_pAnimationSets = new AnimationSets[m_numAnimationSets];

			if (!m_pAnimationSets) {
				throw DxSystemException(DxSystemException::OM_MODEL_ANIMATIONSET_ALLOC_ERROR);
			}

			for (int i = 0; i < m_numAnimationSets; i++) {

				m_pAnimationController->GetAnimationSet(i, &(m_pAnimationSets[i].m_pAnimationSet));

				m_pAnimationSets[i].m_trackSpeed = static_cast<FLOAT>(m_pAnimationSets[i].m_pAnimationSet->GetPeriod());

				m_pAnimationSets[i].m_shiftTime = 0.1f;
			}
		}
	}

	return true;
}


//=============================================================================
// メッシュの描画
// 戻り値：成功 true、失敗 false
// 引　数：Camera*           カメラクラスのポインタ
// 　　　　Projection*       プロジェクションクラスのポインタ
// 　　　　AmbientLight*     アンビエントライトクラスのポインタ
// 　　　　DirectionalLight* ディレクショナルライトのポインタ
// 　　　　bool    アニメーションフラグ
//=============================================================================
void Model::Draw(Camera* pCamera, Projection* pProj, AmbientLight* pAmbient, DirectionalLight* pLight, bool bAnimation)
{
	if (bAnimation && !m_bOneTrackAnimation) {
		AdvanceTime();
	}

	D3DXMATRIX matView = pCamera->GetViewMatrix();
	D3DXMATRIX matProj = pProj->GetProjectionMatrix();
	D3DXMATRIX matVP = matView * matProj;

	D3DCOLORVALUE ambient = pAmbient->GetColorValue();

	D3DLIGHT9 light = pLight->GetLight();

	m_pModelBase->Draw(&m_matWorld, &matVP, &ambient, &light);
}

//=============================================================================
// ワールド座標変換行列の設定
// 引　数：const LPD3DXMATRIX ﾜｰﾙﾄﾞ座標変換行列へのポインタ
//=============================================================================
void Model::SetWorldTransform(const LPD3DXMATRIX pMatWorld)
{
	m_matWorld = *pMatWorld;
}

//=============================================================================
// ワールド座標変換行列の取得
// 戻り値：ワールド座標変換行列
//=============================================================================
D3DXMATRIX Model::GetWorldTransform() const
{
	return m_matWorld;
}

//=============================================================================
// ルートフレームの変換マトリックスを取得
// 引　数：LPD3DXMATRIX 変換マトリックスのポインタ
//=============================================================================
void Model::GetMatrixFromRootFrame(LPD3DXMATRIX pMatrix)
{
	m_pModelBase->GetMatrixFromRootFrame(pMatrix);
}

//=============================================================================
// フレーム名から変換マトリックスを取得
// 引　数：std::string  フレーム名
// 　　　　LPD3DXMATRIX 変換マトリックスのポインタ
//=============================================================================
void Model::GetMatrixFromFrameName(std::string strFrameName, LPD3DXMATRIX pMatrix)
{
	m_pModelBase->GetMatrixFromFrameName(strFrameName, pMatrix);
}

//=============================================================================
// ルートフレームのポインタを取得
// 戻り値：ルートフレームのポインタ
//=============================================================================
LPD3DXMESH Model::GetRootMesh()
{
	return m_pModelBase->GetRootMesh();
}

//=============================================================================
// アニメーションセットの数を取得
// 戻り値：アニメーションセットの数
//=============================================================================
UINT Model::GetAnimationSetsCount() const
{
	assert(m_pAnimationController);

	return m_pAnimationController->GetNumAnimationSets();
}

//=============================================================================
// アニメーションセットを変更する
// 引　数：const int アニメーションセットのインデックス
//=============================================================================
void Model::ChangeAnimationSet(const int index)
{
	assert(m_pAnimationController && index < m_numAnimationSets);

	//同じｱﾆﾒｰｼｮﾝｾｯﾄであれば何もしない
	if (m_nowAnimationSet == index) {
		return;
	}

	D3DXTRACK_DESC td;

	//現在のﾄﾗｯｸ情報を取得
	m_pAnimationController->GetTrackDesc(0, &td);

	//現在のｱﾆﾒｰｼｮﾝｾｯﾄをﾄﾗｯｸ1に移行
	m_pAnimationController->SetTrackAnimationSet(1, m_pAnimationSets[m_nowAnimationSet].m_pAnimationSet);
	m_pAnimationController->SetTrackDesc(1, &td);

	//ｱﾆﾒｰｼｮﾝの変更
	m_pAnimationController->SetTrackAnimationSet(0, m_pAnimationSets[index].m_pAnimationSet);

	//ﾄﾗｯｸｽﾋﾟｰﾄﾞ設定
	//m_pAnimationController->SetTrackSpeed(0, m_pAnimationSets[index].m_trackSpeed);

	//ﾄﾗｯｸの合成を許可
	m_pAnimationController->SetTrackEnable(0, true);
	m_pAnimationController->SetTrackEnable(1, true);

	//ﾄﾗｯｸﾎﾟｼﾞｼｮﾝを最初の位置へ戻す
	m_pAnimationController->SetTrackPosition(0, 0);

	//ｳｪｲﾄ時間の初期化
	m_pAnimationSets[index].m_weightTime = 0.0f;

	//現在のｱﾆﾒｰｼｮﾝｾｯﾄを保管
	m_nowAnimationSet = index;
}

//=============================================================================
// 現在のアニメーションセットを取得する
// 戻り値：アニメーションセットのインデックス
//=============================================================================
int Model::GetNowAnimationSet() const
{
	assert(m_pAnimationController);

	return m_nowAnimationSet;
}

//=============================================================================
// アニメーションを進める時間を設定する
// 引　数：DOUBLE アニメーションを進める時間(秒単位)をセット
//=============================================================================
void Model::SetAdvanceTime(DOUBLE timeDelta)
{
	assert(m_pAnimationController);

	m_advanceTime = timeDelta;
}

//=============================================================================
// アニメーションに設定されているグローバル時間を取得
// 戻り値：アニメーションに設定されているグローバル時間
//=============================================================================
double Model::GetTime() const
{
	assert(m_pAnimationController);

	return m_pAnimationController->GetTime();
}

//=============================================================================
// トラックスピードを設定する
// 引　数：const int    アニメーションセットのインデックス
// 　　　　const DOUBLE １ループにかかる時間
//=============================================================================
void Model::SetTrackSpeed(const int index, const DOUBLE time)
{
	assert(m_pAnimationController && index < m_numAnimationSets);

	DOUBLE defTime = m_pAnimationSets[index].m_pAnimationSet->GetPeriod();

	m_pAnimationSets[index].m_trackSpeed = static_cast<FLOAT>(defTime / time);
}

//=============================================================================
// アニメーションブレンドの移行時間を設定する
// 引　数：const int    アニメーションセットのインデックス
// 　　　　const DOUBLE アニメーションブレンドの移行時間（初期値0.1秒）
//=============================================================================
void Model::SetShiftTime(const int index, const DOUBLE time)
{
	assert(m_pAnimationController && index < m_numAnimationSets);

	m_pAnimationSets[index].m_shiftTime = static_cast<FLOAT>(time);
}

//=============================================================================
// 地形上の高さを設定する
// 戻り値：成功 true、失敗 false
// 引　数：D3DXVECTOR3* 現在位置座標へのポインタ
// 備　考：成功した場合、pVecPosの[y座標]が更新される
// 　　　　動かないオブジェクトにはこちらを使う
//=============================================================================
bool Model::SetPosition(D3DXVECTOR3* pVecPos)
{
	return m_pModelBase->SetPosition(pVecPos);
}

//=============================================================================
// 現在上に乗っているポリゴンインデックスをセット
// 戻り値：成功 true、失敗 false
// 引　数：D3DXVECTOR3* 現在位置座標へのポインタ
// 　　　　DWORD*       現在のポリゴンインデックスへのポインタ
// 備　考：成功後、UpdatePositionで高さが変化する
//=============================================================================
bool Model::SetCurrentIndex(const D3DXVECTOR3* pVecPos, DWORD* pCurrentIndex)
{
	return m_pModelBase->SetCurrentIndex(pVecPos, pCurrentIndex);
}

//=============================================================================
// 隣接ポリゴンチェック用フラグのクリア
// 備　考：UpdatePositionを呼び出す前に必ず呼び出さなければならない
//=============================================================================
void Model::ClearCheckFlag()
{
	m_pModelBase->ClearCheckFlag();
}

//=============================================================================
// 移動後の位置を設定する
// 戻り値：成功 true、失敗 false
// 引　数：D3DXVECTOR3* 現在位置座標へのポインタ
// 　　　　DWORD*       現在のポリゴンインデックスへのポインタ
// 備　考：成功した場合、pVecPosの[y座標]が更新される
//=============================================================================
bool Model::UpdatePosition(D3DXVECTOR3* pVecPos, DWORD* pCurrentIndex)
{
	return m_pModelBase->UpdatePosition(pVecPos, *pCurrentIndex, pCurrentIndex);
}

//=============================================================================
// バウンディングスフィア半径の取得
// 戻り値：バウンディングスフィアの半径
//=============================================================================
float Model::GetBoundingShpereRadius()
{
	return m_pModelBase->GetBoundingShpereRadius();
}

//=============================================================================
// バウンディングボックス最大値の取得
// 戻り値：バウンディングボックス最大値
//=============================================================================
D3DXVECTOR3 Model::GetBoundingBoxMax()
{
	return m_pModelBase->GetBoundingBoxMax();
}

//=============================================================================
// バウンディングボックス最小値の取得
// 戻り値：バウンディングボックス最小値
//=============================================================================
D3DXVECTOR3 Model::GetBoundingBoxMin()
{
	return m_pModelBase->GetBoundingBoxMin();
}

//===========================================================================
// レイとポリゴンの交点までの距離を取得
// 戻り値：レイとポリゴンの交点距離
// 引　数：D3DXVECTOR3& 現在位置座標への参照
//		　 D3DXVECTOR3& キャラクターの向きへの参照
// 　　　　float        チェックする最大距離
//===========================================================================
float Model::GetDistance(const D3DXVECTOR3 &position, const D3DXVECTOR3 &ray, const float limitDistance)
{
	return m_pModelBase->GetDistance(position, ray, limitDistance);
}

//===========================================================================
// アニメーションを進める時間を設定する（トラックが１つしかないアニメーション対応）
// 引　数：maxFrame 最大フレーム数
//===========================================================================
void Model::SetAdvanceTimeOneTrackAnimation(const int maxFrame)
{
	assert(m_pAnimationController);

	m_advanceTime = m_pAnimationSets[0].m_pAnimationSet->GetPeriod() / maxFrame;

	m_bOneTrackAnimation = true;
}

//===========================================================================
// アニメーションを変更する（トラックが１つしかないアニメーション対応）
// 引　数：start 開始フレーム
// 　　　　end   終了フレーム
//===========================================================================
void Model::ChangeAnimationOneTrackAnimation(const int start, const int end)
{
	if (m_startFrame == start && m_endFrame == end) {
		return;
	}

	m_startFrame = start;
	m_endFrame = end;

	m_trackPosition = m_advanceTime * m_startFrame;
	m_pAnimationController->SetTrackPosition(0, m_trackPosition);
	m_bEndLoop = false;
}

//===========================================================================
// アニメーションをループ再生する（トラックが１つしかないアニメーション対応）
//===========================================================================
void Model::LoopAnimationOneTrackAnimation()
{
	if (m_bEndLoop) {
		m_trackPosition = m_advanceTime * m_startFrame;
		m_pAnimationController->SetTrackPosition(0, m_trackPosition);
		m_bEndLoop = false;
	}

	m_trackPosition += m_advanceTime;

	if (m_trackPosition >= m_advanceTime * m_endFrame) {
		m_bEndLoop = true;
	}

	m_pAnimationController->AdvanceTime(m_advanceTime, nullptr);
}

//===========================================================================
// アニメーションを完了まで再生する（トラックが１つしかないアニメーション対応）
// 戻り値：終了 true
//===========================================================================
bool Model::WaitFinishAnimationOneTrackAnimation()
{
	m_trackPosition += m_advanceTime;

	if (m_trackPosition >= m_advanceTime * m_endFrame) {
		return true;
	}

	m_pAnimationController->AdvanceTime(m_advanceTime, nullptr);

	return false;
}

//===========================================================================
// 現在のフレーム番号を取得（トラックが１つしかないアニメーション対応）
// 戻り値：フレーム番号
//===========================================================================
int Model::GetNowFrameOneTrackAnimation()
{
	return (int)(m_trackPosition / m_advanceTime);
}


//*****************************************************************************
// private関数
//*****************************************************************************


//-----------------------------------------------------------------------------
// アニメーションを進める
//-----------------------------------------------------------------------------
void Model::AdvanceTime()
{
	if (m_pAnimationController) {

		m_pAnimationSets[m_nowAnimationSet].m_weightTime += static_cast<FLOAT>(m_advanceTime);

		if (m_pAnimationSets[m_nowAnimationSet].m_weightTime <= m_pAnimationSets[m_nowAnimationSet].m_shiftTime) {

			//ｱﾆﾒｰｼｮﾝﾌﾞﾚﾝﾄﾞ中

			//ｳｪｲﾄ算出
			FLOAT weight = m_pAnimationSets[m_nowAnimationSet].m_weightTime / m_pAnimationSets[m_nowAnimationSet].m_shiftTime;

			m_pAnimationController->SetTrackWeight(0, weight);
			m_pAnimationController->SetTrackWeight(1, 1 - weight);
		}
		else {

			//ｱﾆﾒｰｼｮﾝﾌﾞﾚﾝﾄﾞ終了

			m_pAnimationController->SetTrackWeight(0, 1);
			m_pAnimationController->SetTrackEnable(1, false);
		}

		m_pAnimationController->AdvanceTime(m_advanceTime, nullptr);
	}
}




