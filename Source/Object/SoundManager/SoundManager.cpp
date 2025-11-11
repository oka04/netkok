#define _USING_V110_SDK71_ 1
#include "SoundManager.h"

bool SoundManager::s_isInitialized = false;
const AkGameObjectID  SoundManager::ID_LISTENER = 1;

namespace AK 
{
	void* AllocHook(unsigned int size) { return malloc(size); }
	void FreeHook(void* ptr) { free(ptr); }
	void* VirtualAllocHook(void* pMem, unsigned int size, unsigned long flAllocationType, unsigned long flProtect)
	{
		return VirtualAlloc(pMem, size, flAllocationType, flProtect);
	}
	void VirtualFreeHook(void* pMem, unsigned int size, unsigned long dwFreeType)
	{
		VirtualFree(pMem, size, dwFreeType);
	}
}
static const char* AKResultToString(AKRESULT res)
{
	switch (res)
	{
	case AK_Success: return "AK_Success";
	case AK_Fail: return "AK_Fail";
	case AK_PartialSuccess: return "AK_PartialSuccess";
	case AK_InvalidParameter: return "AK_InvalidParameter";
	case AK_SSEInstructionsNotSupported: return "AK_SSEInstructionsNotSupported";
	case AK_MemManagerNotInitialized: return "AK_MemManagerNotInitialized";
	case AK_StreamMgrNotInitialized: return "AK_StreamMgrNotInitialized";
	default: return "AKRESULT_Unknown";
	}
}

//ログを表示
static void LogResult(const char* label, AKRESULT res)
{
	char buf[256];
	sprintf_s(buf, sizeof(buf), "%s: %s (%d)\n", label, AKResultToString(res), (int)res);
	OutputDebugStringA(buf);    
	(stderr, "%s", buf);
}

static CAkDefaultIOHookBlocking g_lowLevelIO;
bool SoundManager::Initialize()
{
	if (s_isInitialized) return true;

	//MemoryMgr
	AkMemSettings memSettings;
	memset(&memSettings, 0, sizeof(memSettings));
	memSettings.uMaxNumPools = 20;
	AKRESULT res = AK::MemoryMgr::Init(&memSettings);
	LogResult("MemoryMgr::Init", res);
	if (res != AK_Success) return false;

	//StreamMgr
	AkStreamMgrSettings stmSettings;
	AK::StreamMgr::GetDefaultSettings(stmSettings);
	if (!AK::StreamMgr::Create(stmSettings)) return false;

	//Device 設定作成
	AkDeviceSettings deviceSettings;
	AK::StreamMgr::GetDefaultDeviceSettings(deviceSettings);

	const wchar_t* bankPath = L"Wwise\\";
	g_lowLevelIO.SetBankPath(bankPath);

	res = g_lowLevelIO.Init(deviceSettings, false);
	LogResult("CAkDefaultIOHookBlocking::Init", res);
	if (res != AK_Success) return false;

	//Device 作成
	AkDeviceID deviceId = AK::StreamMgr::CreateDevice(deviceSettings, &g_lowLevelIO);
	if (deviceId == AK_INVALID_DEVICE_ID) return false;

	//SoundEngine
	AkInitSettings initSettings;
	AkPlatformInitSettings platformInitSettings;
	AK::SoundEngine::GetDefaultInitSettings(initSettings);
	AK::SoundEngine::GetDefaultPlatformInitSettings(platformInitSettings);
	res = AK::SoundEngine::Init(&initSettings, &platformInitSettings);
	LogResult("SoundEngine::Init", res);
	if (res != AK_Success) return false;

#ifdef _DEBUG
	AkCommSettings commSettings;
	AK::Comm::GetDefaultInitSettings(commSettings);
	res = AK::Comm::Init(commSettings);
	LogResult("Comm::Init", res);
	if (res != AK_Success) return false;
#endif

	//MusicEngine
	AkMusicSettings musicSettings;
	AK::MusicEngine::GetDefaultInitSettings(musicSettings);
	res = AK::MusicEngine::Init(&musicSettings);
	LogResult("MusicEngine::Init", res);
	if (res != AK_Success) return false;

	//Load Banks
	AkBankID out_bankID;
	res = AK::SoundEngine::LoadBank(L"Init.bnk", AK_DEFAULT_POOL_ID, out_bankID);
	LogResult("LoadBank INIT", res);

	res = AK::SoundEngine::LoadBank(L"SB_BGM.bnk", AK_DEFAULT_POOL_ID, out_bankID);
	LogResult("LoadBank BGM", res);
	if (res != AK_Success) return false;

	res = AK::SoundEngine::LoadBank(L"SB_SE.bnk", AK_DEFAULT_POOL_ID, out_bankID);
	LogResult("LoadBank SE", res);
	if (res != AK_Success) return false;

	AK::SoundEngine::RegisterGameObj(ID_LISTENER, "Player_Listener");
	AK::SoundEngine::SetDefaultListeners(&ID_LISTENER, 1);
	s_isInitialized = true;
	return true;
}

void SoundManager::Finalize()
{
	if (!s_isInitialized) return;

#ifdef _DEBUG
	AK::Comm::Term();
#endif

	AK::SoundEngine::UnregisterAllGameObj();
	AK::SoundEngine::Term();
	if (AK::IAkStreamMgr::Get()) AK::IAkStreamMgr::Get()->Destroy();
	AK::MemoryMgr::Term();

	s_isInitialized = false;
}

void SoundManager::Update()
{
	if (s_isInitialized)
		AK::SoundEngine::RenderAudio();
}

AkPlayingID SoundManager::Play(AkUniqueID eventId, AkGameObjectID gameObjectId)
{
	if (!s_isInitialized) return AK_INVALID_PLAYING_ID;
	return AK::SoundEngine::PostEvent(eventId, gameObjectId);
}

void SoundManager::SetSwitch(AkUniqueID switchGroupId, AkUniqueID switchStateId, AkGameObjectID gameObjectId)
{
	if (!s_isInitialized) return;
	AK::SoundEngine::SetSwitch(switchGroupId, switchStateId, gameObjectId);
}

void SoundManager::StopEvent(AkPlayingID  playingId)
{
	if (!s_isInitialized) return;
	AK::SoundEngine::StopPlayingID(playingId);
}

void SoundManager::SetPosition(const D3DXVECTOR3& pos, const D3DXVECTOR3& forward, const D3DXVECTOR3& up, AkGameObjectID gameObjectId)
{
	AkSoundPosition soundPos;

	AkVector position;
	position.X = pos.x;
	position.Y = pos.y;
	position.Z = pos.z;

	AkVector orientationFront;
	orientationFront.X = forward.x;
	orientationFront.Y = forward.y;
	orientationFront.Z = forward.z;

	AkVector orientationTop;
	orientationTop.X = up.x;
	orientationTop.Y = up.y;
	orientationTop.Z = up.z;

	soundPos.Set(position, orientationFront, orientationTop);

	AK::SoundEngine::SetPosition(gameObjectId, soundPos);
}

void SoundManager::StopAll(AkGameObjectID gameObjectId)
{
	if (!s_isInitialized) return;
	AK::SoundEngine::StopAll(gameObjectId);
}

void SoundManager::RegisterGameObject(AkGameObjectID id, const char* name)
{
	AK::SoundEngine::RegisterGameObj(id, name);
}

void SoundManager::UnregisterGameObject(AkGameObjectID id)
{
	AK::SoundEngine::UnregisterGameObj(id);
}
