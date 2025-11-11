//*****************************************************************************
//
// 2D用テクスチャ管理
//
// Texture.cpp
//
// K_Yamaoka
//
// 2011/11/10
//
// 2014/11/28 エラーメッセージを追加
//
// 2016/12/21 フリーサイズテクスチャに対応
//
//*****************************************************************************

#define _USING_V110_SDK71_ 1

#include "TextureManage.h"

#include <cassert>

//=============================================================================
// コンストラクタ
//=============================================================================
TextureManage::TextureManage()
	: m_nMaxTextureWidth(0)
	, m_nMaxTextureHeight(0)
{
	m_mapTexture.clear();
}

//=============================================================================
// デストラクタ
//=============================================================================
TextureManage::~TextureManage()
{
	if (!m_mapTexture.empty()) {

		//テクスチャークラスの解放

		TEXTURE_ITER texIter = m_mapTexture.begin();

		for (; texIter != m_mapTexture.end(); ++texIter) {
			if (texIter->second) {
				delete texIter->second;
			}
		}

		m_mapTexture.clear();
	}
}

//=============================================================================
// テクスチャ管理の初期化
// 引　数：LPDIRECT3D9 Direct3D本体のポインタ
//=============================================================================
void TextureManage::Initialize(LPDIRECT3D9 pD3D9)
{
	//テクスチャの最大サイズを取得
	D3DCAPS9 caps9;

	HRESULT hr = pD3D9->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps9);

	if (FAILED(hr)) {
		throw DxSystemException(DxSystemException::OM_D3D_GETCAPS_ERROR);
	}

	m_nMaxTextureWidth = caps9.MaxTextureWidth;
	m_nMaxTextureHeight = caps9.MaxTextureHeight;
}

//=============================================================================
// テクスチャーの登録
// 引　数：string        ファイル名
// 　　　　TextureClass* テクスチャークラスのアドレス
//=============================================================================
void TextureManage::AddTexture(std::string strFileName, Texture* pClsTexture)
{
	assert(strFileName.length() > 0 && pClsTexture);

	TEXTURE_ITER texIter = m_mapTexture.find(strFileName);

	if (texIter != m_mapTexture.end()) {
		return;
	}

	m_mapTexture.insert(make_pair(strFileName, pClsTexture));
}

//=============================================================================
// テクスチャーの解放（省メモリ対策）
// 引　数：string ファイル名
//=============================================================================
void TextureManage::ReleaseTexture(std::string strFileName)
{
	assert(strFileName.length() > 0);

	if (!m_mapTexture.empty()) {

		TEXTURE_ITER texIter = m_mapTexture.find(strFileName);

		if (texIter == m_mapTexture.end()) {
			return;
		}

		if (texIter->second) {
			delete texIter->second;
		}

		m_mapTexture.erase(texIter);
	}
}

//=============================================================================
// テクスチャーの一時解放（デバイスリセット用）
//=============================================================================
void TextureManage::ReleaseAllTexture()
{
	if (!m_mapTexture.empty()) {

		TEXTURE_ITER texIter = m_mapTexture.begin();

		for (; texIter != m_mapTexture.end(); ++texIter) {
			if (texIter->second) {
				//テクスチャクラス内のテクスチャを解放
				//テクスチャクラスの領域は残しておく
				texIter->second->GetTexturePointer()->Release();
				texIter->second->SetTexturePointer(nullptr);
			}
		}
	}
}	

//=============================================================================
// テクスチャーの再読み込み（デバイスリセット用）
// 引　数：LPDIRECT3DDEVICE9 デバイスのポインタ
//=============================================================================
void TextureManage::ReloadAllTexture(LPDIRECT3DDEVICE9 pDevice)
{
	if (!m_mapTexture.empty()) {

		TEXTURE_ITER texIter = m_mapTexture.begin();

		for (; texIter != m_mapTexture.end(); ++texIter) {

			if (texIter->second) {

				LPDIRECT3DTEXTURE9 pTexture = nullptr;

				HRESULT hr = D3DXCreateTextureFromFileEx(pDevice, (LPCSTR)texIter->first.c_str(), 0, 0, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
					D3DPOOL_DEFAULT, D3DX_FILTER_NONE, D3DX_FILTER_NONE, texIter->second->GetColorKey(), nullptr, nullptr, &pTexture);

				if (FAILED(hr)) {
					throw DxSystemException(DxSystemException::OM_TEXTURE_CREATE_ERROR);
				}

				texIter->second->SetTexturePointer(pTexture);
			}
		}
	}
}	

//=============================================================================
// テクスチャーの探索
// 戻り値：DIRECT3DTEXTURE9のアドレス
// 引　数：string ファイル名
//=============================================================================
LPDIRECT3DTEXTURE9 TextureManage::SearchTexture(std::string strFileName)
{
	assert(strFileName.length() > 0);

	TEXTURE_ITER texIter = m_mapTexture.find(strFileName);

	if (texIter == m_mapTexture.end()) {
		return nullptr;
	}

	return texIter->second->GetTexturePointer();
}

//=============================================================================
// テクスチャーのサイズチェック
// 引　数：D3DXIMAGE_INFO テクスチャ情報
	// 　　　：bool フリーサイズテクスチャサポートフラグ
//=============================================================================
void TextureManage::CheckTextureSize(const D3DXIMAGE_INFO& imageInfo, const bool bFreeSizeTexture) const
{
	if (imageInfo.Width > m_nMaxTextureWidth) {
		throw DxSystemException(DxSystemException::OM_TEXTURE_WIDTH_OVER_ERROR);
	}

	if (imageInfo.Height > m_nMaxTextureHeight) {
		throw DxSystemException(DxSystemException::OM_TEXTURE_HEIGHT_OVER_ERROR);
	}

	if (!bFreeSizeTexture) {

		bool bSizeOK = false;

		for (DWORD mask = 1; mask <= m_nMaxTextureWidth; mask <<= 1) {
			if (imageInfo.Width == mask) {
				bSizeOK = true;
				break;
			}
		}

		if (!bSizeOK) {
			throw DxSystemException(DxSystemException::OM_TEXTURE_SIZE_ERROR);
		}

		bSizeOK = false;

		for (DWORD mask = 1; mask <= m_nMaxTextureHeight; mask <<= 1) {
			if (imageInfo.Height == mask) {
				bSizeOK = true;
				break;
			}
		}

		if (!bSizeOK) {
			throw DxSystemException(DxSystemException::OM_TEXTURE_SIZE_ERROR);
		}
	}
}