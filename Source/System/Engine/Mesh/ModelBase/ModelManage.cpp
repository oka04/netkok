//*****************************************************************************
//
// モデル管理クラス
//
// ModelManage.cpp
//
// K_Yamaoka
//
// 2015/01/29
//
// 2021/10/10 GetAnimationController関数の戻り値をfalseからnullptrに修正
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "ModelManage.h"

#include <cassert>

//=============================================================================
// コンストラクタ
//=============================================================================
ModelManage::ModelManage()
{
	m_mapModel.clear();
}

//=============================================================================
// デストラクタ
//=============================================================================
ModelManage::~ModelManage()
{
	if (!m_mapModel.empty()) {

		MODEL_ITER modelIter = m_mapModel.begin();

		for (; modelIter != m_mapModel.end(); ++modelIter) {
			if (modelIter->second) {
				delete modelIter->second;
				modelIter->second = nullptr;
			}
		}

		m_mapModel.clear();
	}
}

//=============================================================================
// モデルデータの追加
// 引　数：LPDIRECT3DDEVICE9 ３Ｄデバイスへのポインタ
// 　　　　std::string       Ｘファイル名
// 　　　　bool              隣接データ作成フラグ（デフォルトはfalse）
// 備　考：追加するだけでは使用出来ない。GetModelを使って取得する必要がある。
//=============================================================================
void ModelManage::AddModel(LPDIRECT3DDEVICE9 pDevice, std::string strFilename, bool bCreateAdjacencyData)
{
	//すでに登録されているモデルであれば登録しない

	MODEL_ITER modelIter = m_mapModel.find(strFilename);

	if (modelIter != m_mapModel.end()) {
		return;
	}

	//モデルデータ領域確保

	ModelBase* pModelBase = nullptr;

	pModelBase = new ModelBase();

	if (!pModelBase) {
		throw DxSystemException(DxSystemException::OM_MODELBASE_ALLOCATE_ERROR);
	}

	//モデルデータ読み込み

	try {
		pModelBase->LoadModel(pDevice, strFilename);
	}
	catch (DxSystemException dxSystemException) {
		if (!pModelBase) { delete pModelBase; pModelBase = nullptr; }
		dxSystemException.ShowOriginalMessage();
		throw DxSystemException(DxSystemException::OM_MODELBASE_ALLOCATE_ERROR);
	}

	//隣接データ作成

	if (bCreateAdjacencyData) {
		try {
			pModelBase->CreateAdjacencyData();
		}
		catch (DxSystemException dxSystemException) {
			if (!pModelBase) { delete pModelBase; pModelBase = nullptr; }
			dxSystemException.ShowOriginalMessage();
			throw DxSystemException(DxSystemException::OM_MODELBASE_ALLOCATE_ERROR);
		}
	}

	//モデルデータ登録
	m_mapModel.insert(make_pair(strFilename, pModelBase));

	if (!pModelBase) {
		delete pModelBase;
		pModelBase = nullptr;
	}
}

//=============================================================================
// モデルデータの取得
// 戻り値：ModelBase* モデルベースクラスのポインタ
// 引　数：string Ｘファイル名
//=============================================================================
ModelBase* ModelManage::GetModel(std::string strFilename)
{
	MODEL_ITER modelIter = m_mapModel.find(strFilename);

	if (modelIter == m_mapModel.end()) {
		return nullptr;
	}

	return modelIter->second;
}

//=============================================================================
// アニメーションコントローラーの取得
// 戻り値：ID3DXAnimationController* アニメーションコントローラーのポインタ
// 引　数：string Ｘファイル名
//=============================================================================
ID3DXAnimationController* ModelManage::GetAnimationController(std::string strFilename)
{
	MODEL_ITER modelIter = m_mapModel.find(strFilename);

	if (modelIter == m_mapModel.end()) {
		return nullptr;
	}

	ID3DXAnimationController* pCloneAnimationController = modelIter->second->CloneAnimationController();

	return pCloneAnimationController;
}

//=============================================================================
// モデルデータの解放
// 引　数：string Ｘファイル名
//=============================================================================
void ModelManage::ReleaseModel(std::string strFilename)
{
	if (!m_mapModel.empty()) {

		MODEL_ITER modelIter = m_mapModel.find(strFilename);

		if (modelIter == m_mapModel.end()) {
			return;
		}

		if (modelIter->second) {
			delete modelIter->second;
			modelIter->second = nullptr;
		}

		m_mapModel.erase(modelIter);
	}
}



