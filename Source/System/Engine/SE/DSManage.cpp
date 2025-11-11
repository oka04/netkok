//*****************************************************************************
//
// ＳＥ管理（DirectSound）
//
// DSManage.cpp
//
// K_Yamaoka
//
// 2021/10/06
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "DSManage.h"

#include <cassert>

//=============================================================================
// コンストラクタ
//=============================================================================
DSManage::DSManage()
	: m_pSound(nullptr)
	, m_pPrimary(nullptr)
{
	m_mapSE.clear();
}

//=============================================================================
// デストラクタ
//=============================================================================
DSManage::~DSManage()
{
	DeleteAllSE();

	if (m_pPrimary) {
		m_pPrimary->Release();
		m_pPrimary = nullptr;
	}

	if (m_pSound) {
		m_pSound->Release();
		m_pSound = nullptr;
	}
}

//=============================================================================
// サウンド管理クラスの初期化
// 引　数：HWND アプリケーションのウィンドウハンドル
//=============================================================================
void DSManage::Initialize(const HWND hAppWnd)
{
	assert(hAppWnd);

	HRESULT hr;

	hr = DirectSoundCreate8(nullptr, &m_pSound, nullptr);

	if (FAILED(hr)) {
		throw DxSystemException(DxSystemException::OM_SE_CREATE_DIRECTSOUND_ERROR);
	}

	hr = m_pSound->SetCooperativeLevel(hAppWnd, DSSCL_NORMAL);

	if (FAILED(hr)) {
		throw DxSystemException(DxSystemException::OM_SE_SET_COOPERATIVELEVEL_ERROR);
	}

	hr = m_pSound->SetSpeakerConfig(DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_WIDE));

	if (FAILED(hr)) {

		hr = m_pSound->SetSpeakerConfig(DSSPEAKER_MONO);

		if (FAILED(hr)) {
			throw DxSystemException(DxSystemException::OM_SE_SET_SPEEKER_ERROR);
		}
	}

	DSBUFFERDESC  dsbdesc;

	ZeroMemory(&dsbdesc, sizeof(DSBUFFERDESC));
	dsbdesc.dwSize = sizeof(DSBUFFERDESC);
	dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
	dsbdesc.dwBufferBytes = 0;
	dsbdesc.lpwfxFormat = nullptr;

	hr = m_pSound->CreateSoundBuffer(&dsbdesc, &m_pPrimary, nullptr);

	if (FAILED(hr)) {
		throw DxSystemException(DxSystemException::OM_SE_CREATE_PRIMARYBUFFER_ERROR);
	}
}

//=============================================================================
// ＳＥの追加
// 引　数：ＳＥファイル名
//=============================================================================
void DSManage::AddSE(std::string strFileName)
{
	HRESULT hr;

	LPDIRECTSOUNDBUFFER pSoundBuffer = nullptr;

	WAVEFORMATEX* pWaveFormatEx = nullptr;
	
	HMMIO mmioHandle;
	
	MMCKINFO riffChunk;
	MMCKINFO dataChunk;

	//mmioOpen関数にstring::c_str()を渡せないためＣ文字列に変換
	char* pFilename = new char[strFileName.size() + 1];

	if (!pFilename) {
		throw DxSystemException(DxSystemException::OM_SE_ALLOC_FILENAME_ERROR);
	}

	strcpy_s(pFilename, strFileName.size() + 1, strFileName.c_str());

	//Waveファイルのオープンとフォーマットチャンクの読み込み
	hr = WaveFileOpen(pFilename, &mmioHandle, &pWaveFormatEx, &riffChunk);

	if (pFilename) {
		delete[] pFilename;
		pFilename = nullptr;
	}

	if (FAILED(hr)) {
		throw DxSystemException(DxSystemException::OM_SE_WAVE_FILE_OPEN_ERROR);
	}

	//データチャンクに進入
	hr = DescendDataChunk(&mmioHandle, &dataChunk, &riffChunk);

	if (FAILED(hr)) {
		throw DxSystemException(DxSystemException::OM_SE_DESCEND_DATACHUNK_ERROR);
	}

	//セカンダリバッファの定義
	DSBUFFERDESC dsbdesc;
	ZeroMemory(&dsbdesc, sizeof(DSBUFFERDESC));
	dsbdesc.dwSize = sizeof(DSBUFFERDESC);
	dsbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_LOCDEFER | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY;
	dsbdesc.dwBufferBytes = dataChunk.cksize;
	dsbdesc.lpwfxFormat = pWaveFormatEx;

	//バッファを作る
	hr = m_pSound->CreateSoundBuffer(&dsbdesc, &pSoundBuffer, nullptr);

	if (FAILED(hr)) {
		throw DxSystemException(DxSystemException::OM_SE_CREATE_SECONDARYBUFFER_ERROR);
	}

	//バッファにデータを書き込む

	LPVOID lpvPtr1;
	DWORD dwBytes1;
	LPVOID lpvPtr2;
	DWORD dwBytes2;

	hr = pSoundBuffer->Lock(0, dataChunk.cksize, &lpvPtr1, &dwBytes1, &lpvPtr2, &dwBytes2, 0);

	// DSERR_BUFFERLOSTが返された場合，Restoreメソッドを使ってバッファを復元する
	if (hr == DSERR_BUFFERLOST) {
		pSoundBuffer->Restore();
		hr = pSoundBuffer->Lock(0, dataChunk.cksize, &lpvPtr1, &dwBytes1, &lpvPtr2, &dwBytes2, 0);
	}

	if (SUCCEEDED(hr)) {

		CopyWaveData(mmioHandle, dwBytes1, (LPBYTE)lpvPtr1, &dataChunk);

		if (dwBytes2) {
			CopyWaveData(mmioHandle, dwBytes2, (LPBYTE)lpvPtr2, &dataChunk);
		}

		hr = pSoundBuffer->Unlock(lpvPtr1, dwBytes1, lpvPtr2, dwBytes2);
	}

	mmioClose(mmioHandle, 0);

	if (pWaveFormatEx) {
		delete pWaveFormatEx;
		pWaveFormatEx = nullptr;
	}

	//ＳＥクラスの作成
	DS* pDirectSound = nullptr;

	try {
		pDirectSound = new DS;
		pDirectSound->SetSoundBuffer(pSoundBuffer);
	}
	catch (std::bad_alloc) {
		throw DxSystemException(DxSystemException::OM_SE_ALLOCATE_ERROR);
	}

	//コンテナに登録
	m_mapSE.insert(make_pair(strFileName, pDirectSound));
}

//=============================================================================
// ＳＥの再生
// 引　数：string ＳＥファイル名
// 　　　：DOWRD 優先度(0～0xffffffff)
//=============================================================================
void DSManage::PlaySE(std::string strFileName, DWORD priority)
{
	std::map< std::string, DS* >::iterator seIter = m_mapSE.find(strFileName);

	if (seIter == m_mapSE.end()) {
		return;
	}

	LPDIRECTSOUNDBUFFER pSoundBuffer = seIter->second->GetSoundBuffer();
	
	pSoundBuffer->SetCurrentPosition(0);

	if(FAILED(pSoundBuffer->Play(0, priority, 0))) {
		throw DxSystemException(DxSystemException::OM_SE_PLAY_ERROR);
	}
}

//=============================================================================
// ＳＥの停止
// 引　数：string ＳＥファイル名
//=============================================================================
void DSManage::StopSE(std::string strFileName)
{
	std::map< std::string, DS* >::iterator seIter = m_mapSE.find(strFileName);

	if (seIter == m_mapSE.end()) {
		return;
	}

	LPDIRECTSOUNDBUFFER pSoundBuffer = seIter->second->GetSoundBuffer();

	if (FAILED(pSoundBuffer->Stop())) {
		throw DxSystemException(DxSystemException::OM_SE_STOP_ERROR);
	}
}

//=============================================================================
// ＳＥの削除（省メモリ対策）
// 引　数：string ＳＥファイル名
//=============================================================================
void DSManage::ReleaseSE(std::string strFileName)
{
	std::map< std::string, DS* >::iterator seIter = m_mapSE.find(strFileName);

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
void DSManage::StopAllSE()
{
	std::map< std::string, DS* >::iterator seIter = m_mapSE.begin();

	for (; seIter != m_mapSE.end(); seIter++) {
		StopSE(seIter->first);
	}
}

//=============================================================================
// ＳＥの周波数をリセット
//=============================================================================
void DSManage::SetStandardFrequency(std::string strFileName)
{
	std::map< std::string, DS* >::iterator seIter = m_mapSE.find(strFileName);

	if (seIter == m_mapSE.end()) {
		return;
	}

	seIter->second->SetStandardFrequency();
}

//=============================================================================
// ＳＥの周波数を調整
// 引　数：int 増減させる周波数
//=============================================================================
void DSManage::SetFrequency(std::string strFileName, int value)
{
	std::map< std::string, DS* >::iterator seIter = m_mapSE.find(strFileName);

	if (seIter == m_mapSE.end()) {
		return;
	}

	seIter->second->SetFrequency(value);
}

//=============================================================================
// ＳＥのパンをリセット
//=============================================================================
void DSManage::SetStandardPan(std::string strFileName)
{
	std::map< std::string, DS* >::iterator seIter = m_mapSE.find(strFileName);

	if (seIter == m_mapSE.end()) {
		return;
	}

	seIter->second->SetStandardPan();
}

//=============================================================================
// ＳＥのパンを調整
// 引　数：int 増減させる数値
//=============================================================================
void DSManage::SetPan(std::string strFileName, int value)
{
	std::map< std::string, DS* >::iterator seIter = m_mapSE.find(strFileName);

	if (seIter == m_mapSE.end()) {
		return;
	}

	seIter->second->SetPan(value);
}

//=============================================================================
// ＳＥのボリュームをリセット
//=============================================================================
void DSManage::SetStandardVolume(std::string strFileName)
{
	std::map< std::string, DS* >::iterator seIter = m_mapSE.find(strFileName);

	if (seIter == m_mapSE.end()) {
		return;
	}

	seIter->second->SetStandardVolume();
}

//=============================================================================
// ＳＥのボリュームを調整
// 引　数：int 増減させる数値
//=============================================================================
void DSManage::SetVolume(std::string strFileName, int value)
{
	std::map< std::string, DS* >::iterator seIter = m_mapSE.find(strFileName);

	if (seIter == m_mapSE.end()) {
		return;
	}

	seIter->second->SetVolume(value);
}



//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw
// private関数
//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw


//-----------------------------------------------------------------------------
// Waveファイルを開く
// 引　数：CHAR*         ファイル名
// 　　　：HMMIO*        ハンドルへのポインタ
// 　　　：WAVEFORMATEX* フォーマットへのポインタのアドレス
// 　　　：MMCKINFO*     RIFFチャンク情報へのポインタ
// 戻り値：HRESULT
//-----------------------------------------------------------------------------
HRESULT DSManage::WaveFileOpen(CHAR* strFileName, HMMIO* pMmioHandle, WAVEFORMATEX** ppWaveFormatEx, MMCKINFO* pRiffChunk)
{
	HRESULT hr;

	HMMIO mmioHandle = mmioOpen(strFileName, nullptr, MMIO_ALLOCBUF | MMIO_READ);

	if (!mmioHandle) {
		return E_FAIL;
	}

	hr = ReadFormat(mmioHandle, pRiffChunk, ppWaveFormatEx);

	if (FAILED(hr)) {
		mmioClose(mmioHandle, 0);
		return hr;
	}

	*pMmioHandle = mmioHandle;

	return S_OK;
}

//-----------------------------------------------------------------------------
// フォーマットチャンクデータの読み込み
// 引　数：HMMIO*        ハンドルへのポインタ
// 　　　：MMCKINFO*     RIFFチャンク情報へのポインタ
// 　　　：WAVEFORMATEX* フォーマットへのポインタのアドレス
// 戻り値：HRESULT
//-----------------------------------------------------------------------------
HRESULT DSManage::ReadFormat(HMMIO mmioHandle, MMCKINFO* pRiffChunk, WAVEFORMATEX** ppWaveFormatEx)
{
	MMCKINFO        ckIn;
	PCMWAVEFORMAT   pcmWaveFormat;

	*ppWaveFormatEx = nullptr;

	//Riffチャンクに進入
	if (mmioDescend(mmioHandle, pRiffChunk, nullptr, 0)) {
		return E_FAIL;
	}

	//形式チェック
	if ((pRiffChunk->ckid != FOURCC_RIFF) || (pRiffChunk->fccType != mmioFOURCC('W', 'A', 'V', 'E'))) {
		return E_FAIL;
	}

	ckIn.ckid = mmioFOURCC('f', 'm', 't', ' ');

	//フォーマットチャンクに進入
	if (mmioDescend(mmioHandle, &ckIn, pRiffChunk, MMIO_FINDCHUNK)) {
		return E_FAIL;
	}

	if (ckIn.cksize < (LONG) sizeof(PCMWAVEFORMAT)) {
		return E_FAIL;
	}

	if (mmioRead(mmioHandle, (HPSTR)&pcmWaveFormat, sizeof(pcmWaveFormat)) != sizeof(pcmWaveFormat)) {
		return E_FAIL;
	}

	if (pcmWaveFormat.wf.wFormatTag == WAVE_FORMAT_PCM) {

		*ppWaveFormatEx = new WAVEFORMATEX;
		
		if (!(*ppWaveFormatEx)) {
			return E_FAIL;
		}

		memcpy(*ppWaveFormatEx, &pcmWaveFormat, sizeof(pcmWaveFormat));

		(*ppWaveFormatEx)->cbSize = 0;
	}
	else
	{
		WORD cbExtraBytes = 0L;

		if (mmioRead(mmioHandle, (CHAR*)&cbExtraBytes, sizeof(WORD)) != sizeof(WORD)) {
			return E_FAIL;
		}

		*ppWaveFormatEx = (WAVEFORMATEX*)new CHAR[sizeof(WAVEFORMATEX) + cbExtraBytes];

		if (!(*ppWaveFormatEx)) {
			return E_FAIL;
		}

		memcpy(*ppWaveFormatEx, &pcmWaveFormat, sizeof(pcmWaveFormat));

		(*ppWaveFormatEx)->cbSize = cbExtraBytes;

		if (mmioRead(mmioHandle, (CHAR*)(((BYTE*)&((*ppWaveFormatEx)->cbSize)) + sizeof(WORD)), cbExtraBytes) != cbExtraBytes) {
			delete *ppWaveFormatEx;
			*ppWaveFormatEx = nullptr;
			return E_FAIL;
		}
	}

	//チャンクから退出
	if (mmioAscend(mmioHandle, &ckIn, 0))  {
		delete *ppWaveFormatEx;
		*ppWaveFormatEx = nullptr;
		return E_FAIL;
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// データチャンクへ進入
// 引　数：HMMIO*    ハンドルへのポインタ
// 　　　：MMCKINFO* データチャンク情報へのポインタ
// 　　　：MMCKINFO* RIFFチャンク情報へのポインタ
// 戻り値：HRESULT
//-----------------------------------------------------------------------------
HRESULT DSManage::DescendDataChunk(HMMIO* pMmioHandle, MMCKINFO* pDataChunk, MMCKINFO* pRiffChunk)
{
	if (mmioSeek(*pMmioHandle, pRiffChunk->dwDataOffset + sizeof(FOURCC), SEEK_SET) == -1) {
		return E_FAIL;
	}

	pDataChunk->ckid = mmioFOURCC('d', 'a', 't', 'a');

	if (mmioDescend(*pMmioHandle, pDataChunk, pRiffChunk, MMIO_FINDCHUNK)) {
		return E_FAIL;
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// データをバッファにコピーする
// 引　数：HMMIO*    ハンドルへのポインタ
// 　　　：UINT      データのバイト数
// 　　　：BYTE*     バッファの先頭アドレス
// 　　　：MMCKINFO* データチャンク情報へのポインタ
// 戻り値：HRESULT
//-----------------------------------------------------------------------------
HRESULT DSManage::CopyWaveData(HMMIO mmioHandle, UINT size, BYTE* pBuffer, MMCKINFO* pDataChunk)
{
	MMIOINFO mmioinfoIn;

	if (mmioGetInfo(mmioHandle, &mmioinfoIn, 0)) {
		return E_FAIL;
	}

	UINT cbDataIn = size;

	if (cbDataIn > pDataChunk->cksize) {
		cbDataIn = pDataChunk->cksize;
	}

	pDataChunk->cksize -= cbDataIn;

	for (DWORD cT = 0; cT < cbDataIn; cT++) {

		if (mmioinfoIn.pchNext == mmioinfoIn.pchEndRead) {

			if (mmioAdvance(mmioHandle, &mmioinfoIn, MMIO_READ)) {
				return E_FAIL;
			}

			if (mmioinfoIn.pchNext == mmioinfoIn.pchEndRead) {
				return E_FAIL;
			}
		}

		*((BYTE*)pBuffer + cT) = *((BYTE*)mmioinfoIn.pchNext);

		mmioinfoIn.pchNext++;
	}

	if (mmioSetInfo(mmioHandle, &mmioinfoIn, 0)) {
		return E_FAIL;
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// すべてのＳＥの削除
// 戻り値：成功 true、失敗 false
//-----------------------------------------------------------------------------
bool DSManage::DeleteAllSE()
{
	if (!m_mapSE.empty()) {

		std::map< std::string, DS* >::iterator seIter = m_mapSE.begin();

		for (; seIter != m_mapSE.end(); seIter++) {

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





