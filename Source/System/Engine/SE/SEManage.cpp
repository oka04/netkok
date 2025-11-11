//*****************************************************************************
//
// ＳＥ管理
//
// SEManage.cpp
//
// K_Yamaoka
//
// 2011/11/15
//
// 2014/10/28　同じSEを短い時間(100ミリ秒以下)で連続再生した場合ﾗﾝﾀｲﾑｴﾗｰが
// 　　　　　　発生するバグをフィックス
//
// 2014/11/28 エラーメッセージを追加
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "SEManage.h"

#include <cassert>

//=============================================================================
// コンストラクタ
//=============================================================================
SEManage::SEManage()
	: m_pPerformance(nullptr),
	m_pLoader(nullptr),
	PLAY_INTERVAL(100),
	m_timeStart(timeGetTime())
{
	m_mapSE.clear();
}

//=============================================================================
// デストラクタ
//=============================================================================
SEManage::~SEManage()
{
	DeleteAllSE();

	if (m_pPerformance) {
		m_pPerformance->Stop(nullptr, nullptr, 0, 0);
		m_pPerformance->CloseDown();
	}

	if (m_pLoader) {
		m_pLoader->Release();
		m_pLoader = nullptr;
	}

	if (m_pPerformance) {
		m_pPerformance->Release();
		m_pPerformance = nullptr;
	}

	CoUninitialize();
}

//=============================================================================
// サウンド管理クラスの初期化
// 引　数：HWND アプリケーションのウィンドウハンドル
//=============================================================================
void SEManage::Initialize(const HWND hAppWnd)
{
	assert(hAppWnd);

	//ＣＯＭの初期化
	if (FAILED(CoInitialize(nullptr))) {
		throw DxSystemException(DxSystemException::OM_INITCOM_ERROR);
	}

	//パフォーマンスの作成
	if (FAILED(CoCreateInstance(CLSID_DirectMusicPerformance, nullptr, CLSCTX_INPROC, IID_IDirectMusicPerformance8, (void**)&m_pPerformance))) {
		throw DxSystemException(DxSystemException::OM_SE_CREATE_PERFORMANCE_ERROR);
	}

	//オーディオの初期化
	if (FAILED(m_pPerformance->InitAudio(nullptr, nullptr, hAppWnd, DMUS_APATH_SHARED_STEREOPLUSREVERB, 64, DMUS_AUDIOF_ALL, nullptr))) {
		throw DxSystemException(DxSystemException::OM_SE_INIT_AUDIO_ERROR);
	}

	//ローダーの作成
	if (FAILED(CoCreateInstance(CLSID_DirectMusicLoader, nullptr, CLSCTX_INPROC, IID_IDirectMusicLoader8, (void**)&m_pLoader))) {
		throw DxSystemException(DxSystemException::OM_SE_CREATE_LOADER_ERROR);
	}

	CHAR strPath[MAX_PATH];

	//カレントディレクトリの取得
	if (!GetCurrentDirectory(MAX_PATH, (LPSTR)strPath)) {
		throw DxSystemException(DxSystemException::OM_SE_GETCURRENTDIR_ERROR);
	}

	WCHAR wstrSearchPath[MAX_PATH];

	MultiByteToWideChar(CP_ACP, 0, strPath, -1, wstrSearchPath, MAX_PATH);

	//ローダーへディレクトリを登録
	if (FAILED(m_pLoader->SetSearchDirectory(GUID_DirectMusicAllTypes, wstrSearchPath, false))) {
		throw DxSystemException(DxSystemException::OM_SE_SETSEARCHDIR_ERROR);
	}
}

//=============================================================================
// ＳＥの追加
// 引　数：ＳＥファイル名
//=============================================================================
void SEManage::AddSE(std::string strFileName)
{
	assert(m_pPerformance && m_pLoader && strFileName.length() > 0);

	SE_ITER seIter = m_mapSE.find(strFileName);

	//すでに登録済みのＳＥは登録しない（mapなので登録できないが、無駄は省きたい）
	if (seIter != m_mapSE.end()) {
		return;
	}

	LPDIRECTMUSICSEGMENT8 pSegment = nullptr;

	WCHAR wstrFileName[MAX_PATH];

	MultiByteToWideChar(CP_ACP, 0, strFileName.c_str(), -1, wstrFileName, MAX_PATH);

	//ＳＥのロード
	if (FAILED(m_pLoader->LoadObjectFromFile(CLSID_DirectMusicSegment, IID_IDirectMusicSegment8, wstrFileName, (void**)&pSegment))) {
		throw DxSystemException(DxSystemException::OM_SE_LOAD_ERROR);
	}

	//バンドのダウンロード
	if (FAILED(pSegment->Download(m_pPerformance))) {
		pSegment->Unload(m_pPerformance);
		pSegment->Release();
		throw DxSystemException(DxSystemException::OM_SE_DOWNLOAD_ERROR);
	}

	//ＳＥクラスの作成
	SE* pClsSound;

	try {
		pClsSound = new SE;
	}
	catch (std::bad_alloc) {
		pSegment->Unload(m_pPerformance);
		pSegment->Release();
		throw DxSystemException(DxSystemException::OM_SE_ALLOCATE_ERROR);
	}

	//パフォーマンスのポインタを登録
	pClsSound->SetPerformancePointer(m_pPerformance);

	//セグメントのポインタを登録
	pClsSound->SetSegmentPointer(pSegment);

	//コンテナに登録
	m_mapSE.insert(make_pair(strFileName, pClsSound));
}

//=============================================================================
// ＳＥの再生
// 引　数：string ＳＥファイル名
//=============================================================================
void SEManage::PlaySE(std::string strFileName)
{
	assert(m_pPerformance && m_pLoader && strFileName.length() > 0);

	m_timeNow = timeGetTime();

	m_timePass = m_timeNow - m_timeStart;

	if (m_timePass >= PLAY_INTERVAL) {

		SE_ITER seIter = m_mapSE.find(strFileName);

		if (seIter == m_mapSE.end()) {
			return;
		}

		LPDIRECTMUSICSEGMENT8 pSegment = *seIter->second->GetSegmentPointerAddress();

		if (FAILED(m_pPerformance->PlaySegmentEx(pSegment, nullptr, nullptr, DMUS_SEGF_SECONDARY, 0, nullptr, nullptr, nullptr))) {
			throw DxSystemException(DxSystemException::OM_SE_PLAY_ERROR);
		}

		m_timeStart = timeGetTime();
	}
}

//=============================================================================
// ＳＥの停止
// 引　数：string ＳＥファイル名
//=============================================================================
void SEManage::StopSE(std::string strFileName)
{
	assert(m_pPerformance && m_pLoader && strFileName.length() > 0);

	SE_ITER seIter = m_mapSE.find(strFileName);

	if (seIter == m_mapSE.end()) {
		return;
	}

	LPDIRECTMUSICSEGMENT8 pSegment = *seIter->second->GetSegmentPointerAddress();

	if (FAILED(m_pPerformance->StopEx(pSegment, 0, 0))) {
		throw DxSystemException(DxSystemException::OM_SE_STOP_ERROR);
	}
}

//=============================================================================
// ＳＥの削除（省メモリ対策）
// 引　数：string ＳＥファイル名
//=============================================================================
void SEManage::ReleaseSE(std::string strFileName)
{
	assert(m_pPerformance && m_pLoader && strFileName.length() > 0);

	SE_ITER seIter = m_mapSE.find(strFileName);

	if (seIter == m_mapSE.end()) {
		return;
	}

	if (seIter->second) {
		delete seIter->second;
	}

	m_mapSE.erase(seIter);
}

//=============================================================================
// すべての音楽の停止
//=============================================================================
void SEManage::StopAllSE()
{
	SE_ITER seIter = m_mapSE.begin();

	for (; seIter != m_mapSE.end(); ++seIter) {
		StopSE(seIter->first);
	}
}




//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw
// private関数
//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw


//-----------------------------------------------------------------------------
// すべてのＳＥの削除
// 戻り値：成功 true、失敗 false
//-----------------------------------------------------------------------------
bool SEManage::DeleteAllSE()
{
	if (!m_mapSE.empty()) {

		SE_ITER seIter = m_mapSE.begin();

		for (; seIter != m_mapSE.end(); ++seIter) {

			if (seIter->second) {
				StopSE(seIter->first);
				delete seIter->second;
				seIter->second = nullptr;
			}
		}

		m_mapSE.clear();
	}

	return true;
}





