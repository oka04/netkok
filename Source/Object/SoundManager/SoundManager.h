#pragma once
#include <d3dx9.h>
#include "..\\..\\Source\\System\\Engine\\Engine.h"

#include <AK\SoundEngine\Common\AkSoundEngine.h>
#include <AK\SoundEngine\Common\AkSpeakerConfig.h>
#include <AK\SoundEngine\Common\AkTypes.h>
#include <AK\SoundEngine\Common\AkMemoryMgr.h>
#include <AK\SoundEngine\Common\AkModule.h>
#include <AK\SoundEngine\Common\IAkStreamMgr.h>
#include <AK\SoundEngine\Common\AkStreamMgrModule.h> 
#include <AK\Tools\Common\AkPlatformFuncs.h>
#include <AK\MusicEngine\Common\AkMusicEngine.h> 
#include <AK\AkDefaultIOHookBlocking.h> 
#include <AK\Comm\AkCommunication.h>
#include <AK\Wwise_IDs.h> 
#include <cstdio>
#include <cstring>

class SoundManager
{
public:
    static bool Initialize();
    static void Finalize();
	static void Update();
	static AkPlayingID Play(AkUniqueID eventId, AkGameObjectID gameObjectId);
	static void SetSwitch(AkUniqueID switchGroupId, AkUniqueID switchStateId, AkGameObjectID gameObjectId);
	static void StopEvent(AkPlayingID  playingId);
	static void StopAll(AkGameObjectID gameObjectId);
	static void SetPosition(const D3DXVECTOR3& pos, const D3DXVECTOR3& forward, const D3DXVECTOR3& up, AkGameObjectID gameObjectId);
    static void RegisterGameObject(AkGameObjectID id, const char* name);
    static void UnregisterGameObject(AkGameObjectID id);

	static const AkGameObjectID ID_LISTENER;
private:
    static bool s_isInitialized;
};