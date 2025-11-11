//*****************************************************************************
//
// フォント管理
//
// FontManage.cpp
//
// K_Yamaoka
//
// 2011/11/10
//
// 2016/12/02 フォントの追加、解放の戻り値をvoidに変更（エラーを返さない）
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "FontManage.h"

#include <cassert>

//=============================================================================
// コンストラクタ
//=============================================================================
FontManage::FontManage()
{
	m_mapFont.clear();
}

//=============================================================================
// デストラクタ
//=============================================================================
FontManage::~FontManage()
{
	if (!m_mapFont.empty()) {

		//登録されたフォントクラスの削除

		FONT_ITER fontIter = m_mapFont.begin();

		for (; fontIter != m_mapFont.end(); ++fontIter) {
			if (fontIter->second) {
				delete fontIter->second;
			}
		}

		m_mapFont.clear();
	}
}

//=============================================================================
// フォントの追加
// 引　数：string   フォント名
// 　　　　pClsFont フォントクラスのアドレス
//=============================================================================
void FontManage::AddFont(std::string strFontName, Font* pClsFont)
{
	assert(strFontName.length() > 0 && pClsFont);

	FONT_ITER fontIter = m_mapFont.find(strFontName);

	if (fontIter != m_mapFont.end()) {
		return;
	}

	m_mapFont.insert(make_pair(strFontName, pClsFont));
}

//=============================================================================
// フォントの解放
// 引　数：string フォント名
//=============================================================================
void FontManage::ReleaseFont(std::string strFontName)
{
	assert(strFontName.length() > 0);

	if (!m_mapFont.empty()) {

		FONT_ITER fontIter = m_mapFont.find(strFontName);

		if (fontIter == m_mapFont.end()) {
			return;
		}

		if (fontIter->second) {
			delete fontIter->second;
		}

		m_mapFont.erase(fontIter);
	}
}

//=============================================================================
// フォントの一時解放（デバイスリセット用）
//=============================================================================
void FontManage::ReleaseAllFont()
{
	if (!m_mapFont.empty()) {

		FONT_ITER fontIter = m_mapFont.begin();

		for (; fontIter != m_mapFont.end(); ++fontIter) {
			if (fontIter->second) {
				//フォントクラス内のフォントを解放
				//フォントクラスの領域は残しておく
				fontIter->second->GetFontPointer()->Release();
				fontIter->second->SetFontPointer(nullptr);
			}
		}
	}
}

//=============================================================================
// フォントの再作成（デバイスリセット用）
// 引　数：LPDIRECT3DDEVICE9 デバイスのポインタ
//=============================================================================
void FontManage::ReloadAllFont(LPDIRECT3DDEVICE9 pDevice)
{
	if (!m_mapFont.empty()) {

		FONT_ITER fontIter = m_mapFont.begin();

		for (; fontIter != m_mapFont.end(); ++fontIter) {

			if (fontIter->second) {

				LPD3DXFONT pFont = nullptr;

				HRESULT hr = D3DXCreateFont(pDevice, fontIter->second->GetFontSize(), fontIter->second->GetFontSize(), FW_REGULAR, 10, false, SHIFTJIS_CHARSET, 0, 0, FIXED_PITCH | FF_MODERN, (LPCSTR)fontIter->first.c_str(), &pFont);

				if (FAILED(hr)) {
					throw DxSystemException(DxSystemException::OM_FONT_CREATE_ERROR);
				}

				fontIter->second->SetFontPointer(pFont);
			}
		}
	}
}

//=============================================================================
// フォントの探索
// 戻り値：FontClass* フォントクラスのアドレス（未登録の場合nullptrが戻る）
// 引　数：string     フォント名
// 　　　　pClsFont   フォントクラスのアドレス
//=============================================================================
Font* FontManage::SearchFont(std::string strFontName)
{
	assert(strFontName.length() > 0);

	FONT_ITER fontIter = m_mapFont.find(strFontName);

	if (fontIter == m_mapFont.end()) {
		return nullptr;
	}

	return fontIter->second;
}



