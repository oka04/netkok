//*****************************************************************************
//
// ゲームクラス
//
// Game.cpp
//
// K_Yamaoka
//
// 2013/03/04
//
// 2016/11/14 Initialize関数の引数hAppWndを参照へ、hInstanceを追加
//            EngineクラスのInitialize関数の引数を追加
//
// 2018/12/12 サブシーンからSCENE_EXITへの移行が可能になった
//
//*****************************************************************************

#define WIN32_LEAN_AND_MEAN
#define _USING_V110_SDK71_ 1
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Game.h"

#include <cstdlib>
#include <ctime>
#include <new>

using namespace Common;

//=============================================================================
// コンストラクタ
//=============================================================================
Game::Game()
	: m_pEngine(nullptr)
	, m_pNowScene(nullptr)
{
}

//=============================================================================
// デストラクタ
//=============================================================================
Game::~Game()
{
	//現在のシーンクラスの解放
	if (m_pNowScene) {
		delete m_pNowScene;
		m_pNowScene = nullptr;
	}

	//シーンのスタック解放
	if (!Scene::m_stkScene.empty()) {

		while (!Scene::m_stkScene.empty()) {
			delete Scene::m_stkScene.top().m_pPrevScene;
			Scene::m_stkScene.top().m_pPrevScene = nullptr;
			Scene::m_stkScene.pop();
		}
	}

	//エンジンクラスの解放
	if (m_pEngine) {
		delete m_pEngine;
		m_pEngine = nullptr;
	}

	SoundManager::UnregisterGameObject(ID_UI);
	SoundManager::UnregisterGameObject(ID_BGM);

	SoundManager::Finalize();
}

//=============================================================================
// ゲームの初期化
// 引　数：const HWND & ウィンドウハンドル
// 　　　　const HINSTANCE & インスタンスハンドル
//=============================================================================
void Game::Initialize(const HWND &hAppWnd, const HINSTANCE &hInstance)
{
	srand((unsigned int)time(nullptr));

	try {
		//エンジンクラスの初期化
		m_pEngine = new Engine;

		m_pEngine->Initialize(hAppWnd, hInstance);

	}
	catch (DxSystemException dxSystemException) {

		dxSystemException.ShowOriginalMessage();

		throw DxSystemException(DxSystemException::OM_ENGINE_INITIALIZE_ERROR);
	}
	catch (std::bad_alloc) {

		throw DxSystemException(DxSystemException::OM_ENGINE_ALLOCATE_ERROR);
	}

	// サウンド初期化
	if (!SoundManager::Initialize()) {
		throw DxSystemException(DxSystemException::OM_SOUND_INITIALIZE_ERROR);
	}

	SoundManager::RegisterGameObject(ID_BGM, "BGM");
	SoundManager::RegisterGameObject(ID_UI, "UI");

	//最初のシーン設定
	
	Scene::m_prevSceneData.Set(SCENE_INIT, false, nullptr);
	
	Scene::m_nowSceneData = Scene::m_prevSceneData;

	try {
		m_pNowScene = new SceneInit(m_pEngine);
	}
	catch (std::bad_alloc) {
		throw DxSystemException(DxSystemException::OM_NEW_ERROR);
	}
}

//-----------------------------------------------------------------------------
// シーン切り替え
//-----------------------------------------------------------------------------
bool Game::ChangeScene()
{
	switch (Scene::m_nowSceneData.m_scene) {

	case SCENE_INIT:
		m_pNowScene = new SceneInit(m_pEngine);
		break;

	case SCENE_TITLE:
		m_pNowScene = new SceneTitle(m_pEngine);
		break;

	case SCENE_GAME:
		m_pNowScene = new SceneGame(m_pEngine);
		break;

	case SCENE_PAUSE:
		m_pNowScene = new ScenePause(m_pEngine);
		break;
		
	case SCENE_CLEAR:
		m_pNowScene = new SceneClear(m_pEngine);
		break;

	case SCENE_GAMEOVER:
		m_pNowScene = new SceneGameover(m_pEngine);
		break;

	case SCENE_LOBBY:
		m_pNowScene = new SceneLobby(m_pEngine);
		break;

	case SCENE_SELECT_CLIENT:
		m_pNowScene = new SceneSelectClient(m_pEngine);
		break;

	case SCENE_SELECT_HOST:
		m_pNowScene = new SceneSelectHost(m_pEngine);
		break;

	case SCENE_EXIT:
		return false;
	}

	return true;
}

//=============================================================================
// 実行
//=============================================================================
bool Game::Run()
{
	try {

		//シーンが変わったら
		if (Scene::m_nowSceneData.m_scene != Scene::m_prevSceneData.m_scene) {

			//SCENE_EXITで終了
			if (!ChangeScene()) {
				return false;
			}

			Scene::m_prevSceneData = Scene::m_nowSceneData;
		}

		//Runの戻り値がtrue（終了）　→　シーンが変わった
		if (m_pNowScene->Run()) {

			//スタックを調べる
			if (Scene::m_stkScene.empty()) {

				//スタックが空（現在、サブシーンではない）

				//新しいシーンがサブシーンで無ければ
				if (!Scene::m_nowSceneData.m_bSubScene) {

					//シーンの解放
					delete m_pNowScene;
					m_pNowScene = nullptr;

				}
				else {

					//新しいシーンがサブシーンの場合

					//現在のシーンを保存
					Scene::m_prevSceneData.m_pPrevScene = m_pNowScene;

					//スタックにプッシュ
					Scene::m_stkScene.push(Scene::m_prevSceneData);
				}
			}
			else {

				//スタックにシーン情報がある（現在サブシーンに入っている）

				//スタックのトップ同じシーン　→　シーンを戻す
				if (Scene::m_nowSceneData.m_scene == Scene::m_stkScene.top().m_scene) {

					//シーンの解放
					delete m_pNowScene;
					m_pNowScene = nullptr;

					//保存しておいた前のシーンを戻す
					m_pNowScene = Scene::m_stkScene.top().m_pPrevScene;

					//スタックから前のシーンを消す
					Scene::m_stkScene.pop();

					//サブシーンから戻った事をクラスに伝える
					m_pNowScene->ReturnSubScene();

					Scene::m_prevSceneData = Scene::m_nowSceneData;
				}
				else {

					//スタックのトップと違うシーン　→　終了orさらにサブシーンorエラー

					//SCENE_EXITであれば終了
					if (Scene::m_nowSceneData.m_scene == SCENE_EXIT) {
						return false;
					}

					//サブシーン以外への遷移は禁止
					if (!Scene::m_nowSceneData.m_bSubScene) {
						throw DxSystemException(DxSystemException::OM_CANT_CHANGE_SCENE_ERROR);
					}

					//現在のシーンを保存
					Scene::m_prevSceneData.m_pPrevScene = m_pNowScene;

					//スタックにプッシュ
					Scene::m_stkScene.push(Scene::m_prevSceneData);
				}
			}
		}
	}
	catch (DxSystemException dxSystemExeption)
	{
		dxSystemExeption.ShowOriginalMessage();

		return false;
	}
	catch (std::bad_alloc) {

		DxSystemException(DxSystemException::OM_NEW_ERROR).ShowOriginalMessage();

		return false;
	}

	return true;
}








