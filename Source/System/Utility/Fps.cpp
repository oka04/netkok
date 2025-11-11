//*****************************************************************************
//
// FPS管理クラス
//
// Fps.cpp
//
// K_Yamaoka
//
// 2013/03/04
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "Fps.h"

//=============================================================================
// コンストラクタ
//=============================================================================
Fps::Fps()
	: m_refreshRate(60)
	, m_counter(0)
	, m_nowFPS(0)
	, m_timeInterval(1000 / m_refreshRate)
{
	timeBeginPeriod(1);

	m_timeSaveFPS = timeGetTime();
	m_timeSaveFix = timeGetTime();
}

//=============================================================================
// コンストラクタ
// 引　数：リフレッシュレート（秒間のフレーム数）
//=============================================================================
Fps::Fps(const DWORD refreshRate)
	: m_refreshRate(refreshRate)
	, m_counter(0)
	, m_nowFPS(0)
	, m_timeInterval(1000 / m_refreshRate)
{
	timeBeginPeriod(1);

	m_timeSaveFPS = timeGetTime();
	m_timeSaveFix = timeGetTime();
}

//=============================================================================
// デストラクタ
//=============================================================================
Fps::~Fps()
{
	timeEndPeriod(1);
}

//=============================================================================
// ＦＰＳの更新
//=============================================================================
void Fps::UpdateFPS()
{
	DWORD nowTime = timeGetTime();

	DWORD passTime = nowTime - m_timeSaveFPS;

	//１秒経過
	if (passTime >= 1000) {
		//FPS値の更新
		m_nowFPS = m_counter;
		//カウンタのリセット
		m_counter = 0;
		//現在時間を保存
		m_timeSaveFPS = nowTime;
	}

	//描画フレーム数のカウント
	m_counter++;
}

//=============================================================================
// ＦＰＳの固定
//=============================================================================
void Fps::FixRefreshRate()
{
	DWORD nowTime = timeGetTime();

	DWORD passTime = nowTime - m_timeSaveFix;

	//固定したい描画間隔と実際の描画の差を求める
	int difference = (int)m_timeInterval - (int)passTime;

	//固定するためウェイトをかける
	if (difference > 0) {
		Sleep((DWORD)difference);
	}

	m_timeSaveFix = timeGetTime();
}

//=============================================================================
// ＦＰＳ値の取得
// 戻り値：ＦＰＳ値
//=============================================================================
DWORD Fps::GetFPS() const
{
	return m_nowFPS;
}














