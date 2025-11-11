//*****************************************************************************
//
// ＢＧＭ管理
//
// BGMManage.cpp
//
// K_Yamaoka
//
// 2011/11/14
//
// 2014/11/28 エラーメッセージを追加
//            Add関数がfalseを返していないバグを修正
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "BGMManage.h"

#include <cassert>

//=============================================================================
// メンバの初期化（コンストラクタ）
//=============================================================================
BGMManage::BGMManage() 
	: m_pGraphBuilder(nullptr)
	, m_pMediaControl(nullptr)
	, m_pMediaEvent(nullptr)
	, m_pMediaSeeking(nullptr)
{

}

//=============================================================================
// デストラクタ
//=============================================================================
BGMManage::~BGMManage()
{
	ReleaseBGM();

	CoUninitialize();
}

//=============================================================================
// ＢＧＭ管理の初期化
//=============================================================================
void BGMManage::Initializie()
{
	if (FAILED(CoInitialize(nullptr))) {
		throw DxSystemException(DxSystemException::OM_INITCOM_ERROR);
	}
}

//=============================================================================
// DirectShow関連の初期化およびメディアファイルの読み込み
// 引　数：strFileName メディアファイル名（mp3）
//=============================================================================
void BGMManage::AddBGM(std::string strFileName)
{
	assert(strFileName.length() > 0);

	//前のＢＧＭが削除されていなければ削除
	if (m_pGraphBuilder) {
		ReleaseBGM();
	}

	int length = strFileName.length() + 1;

	WCHAR* m_pMediaName;

	try {
		m_pMediaName = new WCHAR[length];
	}
	catch (std::bad_alloc) {
		throw DxSystemException(DxSystemException::OM_BGM_NAME_ALLOCATE_ERROR);
	}

	//マルチバイト文字をワイドキャラに変換
	MultiByteToWideChar(CP_ACP, 0, strFileName.c_str(), -1, m_pMediaName, length);

	//グラフビルダーの作成
	if (FAILED(CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&m_pGraphBuilder))) {
		if (m_pMediaName) { delete[] m_pMediaName; m_pMediaName = nullptr; }
		throw DxSystemException(DxSystemException::OM_BGM_CREATE_GRAPHBUILDER_ERROR);
	}

	//メディアコントロールの作成
	if (FAILED(m_pGraphBuilder->QueryInterface(IID_IMediaControl, (void**)&m_pMediaControl))) {
		if (m_pMediaName) { delete[] m_pMediaName; m_pMediaName = nullptr; }
		throw DxSystemException(DxSystemException::OM_BGM_CREATE_MEDIACONTROL_ERROR);
	}

	//メディアイベントの作成
	if (FAILED(m_pGraphBuilder->QueryInterface(IID_IMediaEvent, (void**)&m_pMediaEvent))) {
		if (m_pMediaName) { delete[] m_pMediaName; m_pMediaName = nullptr; }
		throw DxSystemException(DxSystemException::OM_BGM_CREATE_MEDIAEVENT_ERROR);
	}

	//メディアシーキングの作成
	if (FAILED(m_pGraphBuilder->QueryInterface(IID_IMediaSeeking, (void**)&m_pMediaSeeking))) {
		if (m_pMediaName) { delete[] m_pMediaName; m_pMediaName = nullptr; }
		throw DxSystemException(DxSystemException::OM_BGM_CREATE_MEDIASEEKING_ERROR);
	}

	//指定されたファイルをレンダリングするフィルタ グラフを作成
	if (FAILED(m_pGraphBuilder->RenderFile(m_pMediaName, nullptr))) {
		if (m_pMediaName) { delete[] m_pMediaName; m_pMediaName = nullptr; }
		throw DxSystemException(DxSystemException::OM_BGM_CREATE_FILTERGRAPH_ERROR);
	}

	if (m_pMediaName) {
		delete[] m_pMediaName;
		m_pMediaName = nullptr;
	}
}

//=============================================================================
// DirectShow関連領域の解放
//=============================================================================
void BGMManage::ReleaseBGM()
{
	///解放する際にＢＧＭを明示的に止める必要があるのか？
	StopBGM();

	if (m_pMediaSeeking) {
		m_pMediaSeeking->Release();
		m_pMediaSeeking = nullptr;
	}

	if (m_pMediaEvent) {
		m_pMediaEvent->Release();
		m_pMediaEvent = nullptr;
	}

	if (m_pMediaControl) {
		m_pMediaControl->Release();
		m_pMediaControl = nullptr;
	}

	if (m_pGraphBuilder) {
		m_pGraphBuilder->Release();
		m_pGraphBuilder = nullptr;
	}
}

//=============================================================================
// メディアの再生
// 戻り値：成功 true  失敗 false
//=============================================================================
void BGMManage::PlayBGM()
{
	if (m_pMediaControl) {

		if (IsPlaying()) {
			Rewind();
		}

		if (FAILED(m_pMediaControl->Run())) {
			throw DxSystemException(DxSystemException::OM_BGM_PLAY_ERROR);
		}
	}
}

//=============================================================================
// メディアの停止
//=============================================================================
void BGMManage::StopBGM()
{
	if (m_pMediaControl) {
		if (FAILED(m_pMediaControl->Stop())) {
			throw DxSystemException(DxSystemException::OM_BGM_STOP_ERROR);
		}
	}
}

//=============================================================================
// ループ再生
//=============================================================================
void BGMManage::LoopBGM()
{
	if (!IsPlaying()) {
		PlayBGM();
	}
}




//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw
// private関数
//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw


//-----------------------------------------------------------------------------
// 再生中チェック
// 戻り値：再生中 true  それ以外 false
//-----------------------------------------------------------------------------
bool BGMManage::IsPlaying()
{
	bool bPlaying = true;

	if (m_pMediaEvent) {

		long evCode;
		long evParam1;
		long evParam2;

		//メディアからイベントを取得
		while (SUCCEEDED(m_pMediaEvent->GetEvent(&evCode, &evParam1, &evParam2, 0))) {

			//再生が終了していた場合falseを返す
			if (evCode == EC_COMPLETE) {
				bPlaying = false;
			}

			m_pMediaEvent->FreeEventParams(evCode, evParam1, evParam2);
		}
	}

	return bPlaying;
}

//-----------------------------------------------------------------------------
// メディアの巻き戻し
//-----------------------------------------------------------------------------
void BGMManage::Rewind()
{
	if (m_pMediaSeeking) {

		REFERENCE_TIME Start;
		REFERENCE_TIME End;

		bool  bCanSeek = false;

		DWORD caps = AM_SEEKING_CanSeekAbsolute | AM_SEEKING_CanGetDuration;

		bCanSeek = (S_OK == m_pMediaSeeking->CheckCapabilities(&caps));

		if (bCanSeek) {
			m_pMediaSeeking->GetAvailable(&Start, &End);
			m_pMediaSeeking->SetPositions(&Start, AM_SEEKING_AbsolutePositioning, &End, AM_SEEKING_AbsolutePositioning);
		}
	}
}




