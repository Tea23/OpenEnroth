#include "Media/Audio/AudioPlayer.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>

#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Level/Decoration.h"
#include "Engine/MM7.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Party.h"
#include "Engine/ZlibWrapper.h"

#include "Media/Audio/OpenALSoundProvider.h"


int sLastTrackLengthMS;
AudioPlayer *pAudioPlayer;
SoundList *pSoundList;

std::array<float, 10> pSoundVolumeLevels = {
    {0.0000000f, 0.1099999f, 0.2199999f, 0.3300000f, 0.4399999f, 0.5500000f,
     0.6600000f, 0.7699999f, 0.8799999f, 0.9700000f}};

enum SOUND_TYPE {
    SOUND_TYPE_LEVEL = 0,
    SOUND_TYPE_SYSTEM = 1,
    SOUND_TYPE_SWAP = 2,
    SOUND_TYPE_UNKNOWN = 3,
    SOUND_TYPE_LOCK = 4,
};

enum SOUND_FLAG {
    SOUND_FLAG_LOCKED = 0x1,
    SOUND_FLAG_3D = 0x2,
};

class SoundInfo {
 public:
    bool Is3D() { return ((uFlags & SOUND_FLAG_3D) == SOUND_FLAG_3D); }

 public:
    std::string sName;
    SOUND_TYPE eType;
    uint32_t uSoundID;
    uint32_t uFlags;
    std::shared_ptr<Blob> buffer;
    PAudioSample sample;
    uint32_t last_pid = PID_INVALID;
};

std::map<uint32_t, SoundInfo> mapSounds;

#pragma pack(push, 1)
struct SoundDesc_mm6 {
    uint8_t pSoundName[32];
    uint32_t uSoundID;
    uint32_t eType;
    uint32_t uFlags;
    uint32_t pSoundDataID[17];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct SoundDesc : public SoundDesc_mm6 {
    uint32_t p3DSoundID;
    uint32_t bDecompressed;
};
#pragma pack(pop)

void SoundList::Initialize() {}

void SoundList::FromFile(const Blob &data_mm6, const Blob &data_mm7, const Blob &data_mm8) {
    static_assert(sizeof(SoundDesc_mm6) == 112, "Wrong type size");
    static_assert(sizeof(SoundDesc) == 120, "Wrong type size");

    size_t num_mm6_sounds = data_mm6 ? *(uint32_t *)data_mm6.data() : 0;
    size_t num_mm7_sounds = data_mm7 ? *(uint32_t *)data_mm7.data() : 0;
    size_t num_mm8_sounds = data_mm8 ? *(uint32_t *)data_mm8.data() : 0;

    unsigned int sNumSounds = num_mm6_sounds + num_mm7_sounds + num_mm8_sounds;
    assert(sNumSounds);
    assert(!num_mm8_sounds);

    SoundDesc *sounds = (SoundDesc *)((char *)data_mm7.data() + 4);
    for (size_t i = 0; i < num_mm7_sounds; i++) {
        SoundInfo si;
        si.sName = (char *)sounds[i].pSoundName;
        si.uSoundID = sounds[i].uSoundID;
        si.eType = (SOUND_TYPE)sounds[i].eType;
        si.uFlags = sounds[i].uFlags;
        mapSounds[si.uSoundID] = si;
    }

    SoundDesc_mm6 *sounds_mm6 = (SoundDesc_mm6 *)((char *)data_mm6.data() + 4);
    for (size_t i = 0; i < num_mm6_sounds; i++) {
        SoundInfo si;
        si.sName = (char *)sounds_mm6[i].pSoundName;
        si.uSoundID = sounds_mm6[i].uSoundID;
        si.eType = (SOUND_TYPE)sounds_mm6[i].eType;
        si.uFlags = sounds_mm6[i].uFlags;
        mapSounds[si.uSoundID] = si;
    }
}

extern OpenALSoundProvider *provider;

void AudioPlayer::MusicPlayTrack(MusicID eTrack) {
    if (currentMusicTrack == eTrack) {
        return;
    }

    if (!engine->config->debug.NoSound.Get() && bPlayerReady && engine->config->settings.MusicLevel.Get() > 0) {
        if (pCurrentMusicTrack) {
            pCurrentMusicTrack->Stop();
        }
        currentMusicTrack = -1;

        std::string file_path = StringPrintf("%d.mp3", eTrack);
        file_path = MakeDataPath("music", file_path);
        if (!std::filesystem::exists(file_path)) {
            logger->Warning("AudioPlayer: %s not found", file_path.c_str());
            return;
        }

        pCurrentMusicTrack = CreateAudioTrack(file_path);
        if (pCurrentMusicTrack) {
            currentMusicTrack = eTrack;
            pCurrentMusicTrack->SetVolume(
                pSoundVolumeLevels[engine->config->settings.MusicLevel.Get()]);
            pCurrentMusicTrack->Play();
        }
    }
}

void AudioPlayer::MusicStart() {}

void AudioPlayer::MusicStop() {
    if (!pCurrentMusicTrack) {
        return;
    }

    pCurrentMusicTrack->Stop();
    pCurrentMusicTrack = nullptr;
    currentMusicTrack = -1;
}

void AudioPlayer::MusicPause() {
    if (!pCurrentMusicTrack) {
        return;
    }

    pCurrentMusicTrack->Pause();
}

void AudioPlayer::MusicResume() {
    if (!pCurrentMusicTrack) {
        return;
    }

    if (!pCurrentMusicTrack->Resume()) {
        int playedMusicTrack = currentMusicTrack;
        if (currentMusicTrack > 0) {
            MusicStop();
            MusicPlayTrack((MusicID)playedMusicTrack);
        }
    }
}

void AudioPlayer::SetMusicVolume(int vol) {
    if (!pCurrentMusicTrack) {
        return;
    }

    vol = std::max(0, vol);
    vol = std::min(9, vol);
    pCurrentMusicTrack->SetVolume(pSoundVolumeLevels[vol] * 2.f);
}

float AudioPlayer::MusicGetVolume() {
    if (!pCurrentMusicTrack) {
        return 0.f;
    }

    return pCurrentMusicTrack->GetVolume();
}

void AudioPlayer::SetMasterVolume(int level) {
    level = std::max(0, level);
    level = std::min(9, level);
    uMasterVolume = (2.f * pSoundVolumeLevels[level]);

    auto iter = mapSounds.begin();
    while (iter != mapSounds.end()) {
        SoundInfo &si = iter->second;
        if (si.sample) {
            // if not voice sample - set volume
            if (PID_TYPE(si.last_pid) != OBJECT_Player)
                si.sample->SetVolume(uMasterVolume);
        }
        ++iter;
    }
}

void AudioPlayer::SetVoiceVolume(int level) {
    level = std::max(0, level);
    level = std::min(9, level);
    uVoiceVolume = (2.f * pSoundVolumeLevels[level]);

    auto iter = mapSounds.begin();
    while (iter != mapSounds.end()) {
        SoundInfo &si = iter->second;
        if (si.sample) {
            // if voice sample - set volume
            if (PID_TYPE(si.last_pid) == OBJECT_Player)
                si.sample->SetVolume(uVoiceVolume);
        }
        ++iter;
    }
}

void AudioPlayer::StopAll(int sample_id) { // sample id is pid of origin
    if (!bPlayerReady) {
        return;
    }

    // looks like this was just meant to stop party walking sounds overrunning
}

void AudioPlayer::PlaySound(SoundID eSoundID, int pid, unsigned int uNumRepeats, int source_x, int source_y, int sound_data_id) {
    if (!bPlayerReady)
        return;

    //logger->Info("AudioPlayer: trying to load sound id %u", eSoundID);

    if (engine->config->settings.SoundLevel.Get() < 1 || (eSoundID == SOUND_Invalid)) {
        return;
    }

    if (mapSounds.find(eSoundID) == mapSounds.end()) {
        logger->Warning("AudioPlayer: sound id %u not found", eSoundID);
        return;
    }

    SoundInfo &si = mapSounds[eSoundID];
    //logger->Info("AudioPlayer: sound id %u found as '%s'", eSoundID, si.sName.c_str());

    if (!si.sample) {
        std::shared_ptr<Blob> buffer;

        if (si.sName == "") {  // enable this for bonus sound effects
            //logger->Info("AudioPlayer: trying to load bonus sound %u", eSoundID);
            //buffer = LoadSound(int(eSoundID));
        } else {
            buffer = LoadSound(si.sName);
        }

        if (!buffer) {
            logger->Warning("AudioPlayer: failed to load sound %u (%s)", eSoundID, si.sName.c_str());
            return;
        }

        si.sample = CreateAudioSample(buffer);
        if (!si.sample) {
            logger->Warning("AudioPlayer: failed to sample sound %u (%s)", eSoundID, si.sName.c_str());
            return;
        }
    }

    si.sample->SetVolume(uMasterVolume);

    if (pid == 0) {  // generic sound like from UI
        si.sample->Play();
    } else if (pid == -1) {  // exclusive sounds - can override
        si.sample->Stop();
        si.sample->Play();
    } else if (pid < 0) {  // exclusive sounds - no override (close chest)
        si.sample->Play();
    } else {
        ObjectType object_type = PID_TYPE(pid);
        unsigned int object_id = PID_ID(pid);
        switch (object_type) {
            case OBJECT_Door: {
                assert(uCurrentlyLoadedLevelType == LEVEL_Indoor);
                assert((int)object_id < pIndoor->pDoors.size());

                provider->SetListenerPosition(pParty->vPosition.x / 50.f,
                                              pParty->vPosition.y / 50.f,
                                              pParty->vPosition.z / 50.f);
                si.sample->SetPosition(pIndoor->pDoors[object_id].pXOffsets[0] / 50.f,
                                       pIndoor->pDoors[object_id].pYOffsets[0] / 50.f,
                                       pIndoor->pDoors[object_id].pZOffsets[0] / 50.f, 500.f);

                si.sample->Play(false, true);

                break;
            }
            case OBJECT_Player: {
                si.sample->SetVolume(uVoiceVolume);
                if (object_id == 5) si.sample->Stop();
                si.sample->Play();

                break;
            }
            case OBJECT_Actor: {
                assert(object_id < pActors.size());

                provider->SetListenerPosition(pParty->vPosition.x / 50.f,
                                              pParty->vPosition.y / 50.f,
                                              pParty->vPosition.z / 50.f);
                si.sample->SetPosition(pActors[object_id].vPosition.x / 50.f,
                                       pActors[object_id].vPosition.y / 50.f,
                                       pActors[object_id].vPosition.z / 50.f, 500.f);

                si.sample->Play(false, true);

                break;
            }
            case OBJECT_Decoration: {
                assert(object_id < pLevelDecorations.size());

                provider->SetListenerPosition((float)pParty->vPosition.x / 50.f,
                                              (float)pParty->vPosition.y / 50.f,
                                              (float)pParty->vPosition.z / 50.f);
                si.sample->SetPosition((float)pLevelDecorations[object_id].vPosition.x / 50.f,
                                       (float)pLevelDecorations[object_id].vPosition.y / 50.f,
                                       (float)pLevelDecorations[object_id].vPosition.z / 50.f, 2000.f);

                si.sample->Play(true, true);

                break;
            }
            case OBJECT_Item: {
                assert(object_id < pSpriteObjects.size());

                provider->SetListenerPosition(pParty->vPosition.x / 50.f,
                                              pParty->vPosition.y / 50.f,
                                              pParty->vPosition.z / 50.f);
                si.sample->SetPosition(pSpriteObjects[object_id].vPosition.x / 50.f,
                                       pSpriteObjects[object_id].vPosition.y / 50.f,
                                       pSpriteObjects[object_id].vPosition.z / 50.f, 500.f);

                si.sample->Play(false, true);
               // return;
                break;
            }
            case OBJECT_Face: {
                si.sample->Play();

                break;
            }

            default:
                break;
        }
    }

    si.last_pid = pid;

    if (engine->config->debug.VerboseLogging.Get()) {
        if (si.sName == "")
            logger->Info("AudioPlayer: playing sound %u", eSoundID);
        else
            logger->Info("AudioPlayer: playing sound %u with name '%s'", eSoundID, si.sName.c_str());
    }

    return;
}

void AudioPlayer::ResumeSounds() {
    auto iter = mapSounds.begin();
    while (iter != mapSounds.end()) {
        SoundInfo &si = iter->second;
        if (si.sample) {
            if (si.sample->Resume() && engine->config->debug.VerboseLogging.Get())
                logger->Info("sound resumed: %s", si.sName.c_str());
        }
        ++iter;
    }
}

void AudioPlayer::UpdateSounds() {
    float pitch = pi * (float)pParty->sRotationY / 1024.f;
    float yaw = pi * (float)pParty->sRotationZ / 1024.f;
    provider->SetOrientation(yaw, pitch);
    provider->SetListenerPosition(pParty->vPosition.x / 50.f,
                                  pParty->vPosition.y / 50.f,
                                  pParty->vPosition.z / 50.f);
}

void AudioPlayer::PauseSounds(int uType) {
    // pause everything
    if (uType == 2) {
        auto iter = mapSounds.begin();
        while (iter != mapSounds.end()) {
            SoundInfo &si = iter->second;
            if (si.sample) {
                if (si.sample->Pause() && engine->config->debug.VerboseLogging.Get())
                    logger->Info("sound paused: %s", si.sName.c_str());
            }
            ++iter;
        }
    } else {
        // pause non exclusives
        auto iter = mapSounds.begin();
        while (iter != mapSounds.end()) {
            SoundInfo &si = iter->second;
            if (si.sample) {
                if (si.last_pid <= 0) {
                    if (si.sample->Pause() && engine->config->debug.VerboseLogging.Get())
                        logger->Info("sound paused: %s", si.sName.c_str());
                }
            }
            ++iter;
        }
    }
}

#pragma pack(push, 1)
struct SoundHeader_mm7 {
    char pSoundName[40];
    uint32_t uFileOffset;
    uint32_t uCompressedSize;
    uint32_t uDecompressedSize;
};
#pragma pack(pop)

void AudioPlayer::LoadAudioSnd() {
    static_assert(sizeof(SoundHeader_mm7) == 52, "Wrong type size");

    std::string file_path = MakeDataPath("sounds", "audio.snd");
    fAudioSnd.open(MakeDataPath("sounds", "audio.snd"), std::ios_base::binary);
    if (!fAudioSnd.good()) {
        logger->Warning("Can't open file: %s", file_path.c_str());
        return;
    }

    uint32_t uNumSoundHeaders {};
    fAudioSnd.read((char*)&uNumSoundHeaders, 4);
    for (uint32_t i = 0; i < uNumSoundHeaders; i++) {
        SoundHeader_mm7 header_mm7 {};
        fAudioSnd.read((char*)&header_mm7, sizeof(SoundHeader_mm7));
        SoundHeader header {};
        header.uFileOffset = header_mm7.uFileOffset;
        header.uCompressedSize = header_mm7.uCompressedSize;
        header.uDecompressedSize = header_mm7.uDecompressedSize;
        mSoundHeaders[ToLower(header_mm7.pSoundName)] = header;
    }
}

void AudioPlayer::Initialize() {
    currentMusicTrack = 0;
    uMasterVolume = 127;

    pAudioPlayer->SetMasterVolume(engine->config->settings.SoundLevel.Get());
    pAudioPlayer->SetVoiceVolume(engine->config->settings.VoiceLevel.Get());
    if (bPlayerReady) {
        SetMusicVolume(engine->config->settings.MusicLevel.Get());
    }
    LoadAudioSnd();

    bPlayerReady = true;
}

void PlayLevelMusic() {
    unsigned int map_id = pMapStats->GetMapInfo(pCurrentMapName);
    if (map_id) {
        pAudioPlayer->MusicPlayTrack((MusicID)pMapStats->pInfos[map_id].uRedbookTrackID);
    }
}


bool AudioPlayer::FindSound(const std::string &pName, AudioPlayer::SoundHeader *header) {
    if (header == nullptr) {
        return false;
    }

    std::map<std::string, SoundHeader>::iterator it = mSoundHeaders.find(ToLower(pName));
    if (it == mSoundHeaders.end()) {
        return false;
    }

    *header = it->second;

    return true;
}


std::shared_ptr<Blob> AudioPlayer::LoadSound(int uSoundID) {  // bit of a kludge (load sound by ID index) - plays some interesting files
    SoundHeader header = { 0 };

    if (uSoundID < 0 || uSoundID > mSoundHeaders.size())
        return nullptr;

    // iterate through to get sound by int ID
    std::map<std::string, SoundHeader>::iterator it = mSoundHeaders.begin();
    std::advance(it, uSoundID);

    if (it == mSoundHeaders.end()) {
        return nullptr;
    }

    header = it->second;

    // read into buffer
    std::shared_ptr<Blob> buffer = Blob::AllocateShared(header.uDecompressedSize);

    fAudioSnd.seekg(header.uFileOffset, std::ios_base::beg);
    if (header.uCompressedSize >= header.uDecompressedSize) {
        header.uCompressedSize = header.uDecompressedSize;
        if (header.uDecompressedSize) {
            fAudioSnd.read((char*)buffer->data(), header.uDecompressedSize);
        } else {
            logger->Warning("Can't load sound file!");
        }
    } else {
        std::shared_ptr<Blob> compressed = Blob::AllocateShared(header.uCompressedSize);
        fAudioSnd.read((char*)compressed->data(), header.uCompressedSize);
        buffer = std::make_shared<Blob>(zlib::Uncompress(*compressed));
    }

    return buffer;
}


std::shared_ptr<Blob> AudioPlayer::LoadSound(const std::string &pSoundName) {
    SoundHeader header = { 0 };
    if (!FindSound(pSoundName, &header)) {
        logger->Warning("AudioPlayer: %s can't load sound header!", pSoundName.c_str());
        return nullptr;
    }

    std::shared_ptr<Blob> buffer = Blob::AllocateShared(header.uDecompressedSize);

    fAudioSnd.seekg(header.uFileOffset, std::ios_base::beg);
    if (header.uCompressedSize >= header.uDecompressedSize) {
        header.uCompressedSize = header.uDecompressedSize;
        if (header.uDecompressedSize) {
            fAudioSnd.read((char*)buffer->data(), header.uDecompressedSize);
        } else {
            logger->Warning("AudioPlayer: %s can't load sound file!", pSoundName.c_str());
        }
    } else {
        std::shared_ptr<Blob> compressed = Blob::AllocateShared(header.uCompressedSize);
        fAudioSnd.read((char*)compressed->data(), header.uCompressedSize);
        *buffer = zlib::Uncompress(*compressed);
    }

    return buffer;
}

std::array<uint16_t, 101> word_4EE088_sound_ids = {
    {00000, 10000, 10010, 10020, 10030, 10040, 10050, 10060, 10070, 10080,
     10090, 10100, 11000, 11010, 11020, 11030, 11040, 11050, 11060, 11070,
     11080, 11090, 11100, 12000, 12010, 12020, 12030, 12040, 12050, 12060,
     12070, 12080, 12090, 12100, 13000, 13010, 13020, 13030, 13040, 13050,
     13060, 13070, 13080, 13090, 13100, 14000, 14010, 14020, 14030, 14040,
     14050, 14060, 14070, 14080, 14090, 14100, 15000, 15010, 15020, 15030,
     15040, 15050, 15060, 15070, 15080, 15090, 15100, 16000, 16010, 16020,
     16030, 16040, 16050, 16060, 16070, 16080, 16090, 16100, 17000, 17010,
     17020, 17030, 17040, 17050, 17060, 17070, 17080, 17090, 17100, 18000,
     18010, 18020, 18030, 18040, 18050, 18060, 18070, 18080, 18090, 18100,
     1}};

void AudioPlayer::PlaySpellSound(unsigned int spell, unsigned int pid) {
    PlaySound((SoundID)word_4EE088_sound_ids[spell], pid, 0, -1, 0, 0);
}

