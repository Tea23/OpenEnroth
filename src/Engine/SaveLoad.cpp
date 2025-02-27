#include "Engine/SaveLoad.h"

#include <stdlib.h>
#include <filesystem>
#include <algorithm>
#include <string>


#include "Engine/Engine.h"
#include "Engine/LOD.h"
#include "Engine/Localization.h"
#include "Engine/Party.h"
#include "Engine/Time.h"
#include "Engine/ZlibWrapper.h"
#include "Engine/stru123.h"

#include "Engine/Graphics/ImageLoader.h"
#include "Engine/Graphics/Level/Decoration.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Overlays.h"
#include "Engine/Graphics/PCX.h"
#include "Engine/Graphics/Viewport.h"

#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/SpriteObject.h"

#include "Engine/Serialization/LegacyImages.h"

#include "GUI/GUIFont.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UISaveLoad.h"
#include "GUI/UI/UIStatusBar.h"

#include "Media/Audio/AudioPlayer.h"

struct SavegameList *pSavegameList = new SavegameList;
unsigned int uNumSavegameFiles;
std::array<unsigned int, MAX_SAVE_SLOTS> pSavegameUsedSlots;
std::array<Image *, MAX_SAVE_SLOTS> pSavegameThumbnails;
std::array<SavegameHeader, MAX_SAVE_SLOTS> pSavegameHeader;

bool CopyFile(const std::string &from, const std::string &to) {
    int file_size = -1;
    int bytes_read = 0;
    int bytes_wrote = 0;

    FILE *copy_from = fopen(from.c_str(), "rb");
    if (copy_from) {
        FILE *copy_to = fopen(to.c_str(), "wb+");
        if (copy_to) {
            fseek(copy_from, 0, SEEK_END);
            file_size = ftell(copy_from);
            fseek(copy_from, 0, SEEK_SET);

            unsigned char *buf = new unsigned char[file_size];
            if (buf) {
                bytes_read = fread(buf, 1, file_size, copy_from);
                if (bytes_read == file_size) {
                    bytes_wrote = fwrite(buf, 1, file_size, copy_to);
                }

                delete[] buf;
            }
            fclose(copy_to);
        }
        fclose(copy_from);
    }

    return file_size != -1 && bytes_read == bytes_wrote;
}

void LoadGame(unsigned int uSlot) {
    MapsLongTimers_count = 0;
    if (!pSavegameUsedSlots[uSlot]) {
        pAudioPlayer->PlaySound(SOUND_error, 0, 0, -1, 0, 0);
        logger->Warning("LoadGame: slot %u is empty", uSlot);
        return;
    }

    pNew_LOD->CloseWriteFile();
    // uCurrentlyLoadedLevelType = LEVEL_null;

    std::string filename = MakeDataPath("saves", pSavegameList->pFileList[uSlot]);
    std::string to_file_path = MakeDataPath("data", "new.lod");
    remove(to_file_path.c_str());
    if (!CopyFile(filename, to_file_path)) {
        Error("Failed to copy: %s", filename.c_str());
    }

    pNew_LOD->LoadFile(to_file_path, 0);

    static_assert(sizeof(SavegameHeader) == 100, "Wrong type size");
    Blob headerBlob = pNew_LOD->LoadRaw("header.bin");
    SavegameHeader *header = (SavegameHeader*)headerBlob.data();
    if (header == nullptr) {
        logger->Warning(localization->FormatString(
            LSTR_FMT_SAVEGAME_CORRUPTED, 100).c_str());
    }

    {
        Blob partyBlob = pNew_LOD->LoadRaw("party.bin");
        Party_MM7 *serialization = (Party_MM7*)partyBlob.data();
        if (serialization == nullptr) {
            logger->Warning(localization->FormatString(
                LSTR_FMT_SAVEGAME_CORRUPTED, 101).c_str());
        } else {
            Deserialize(*serialization, pParty);

            pParty->bTurnBasedModeOn = false;  // We always start in realtime after loading a game.

            for (size_t i = 0; i < 4; i++) {
                Player *player = &pParty->pPlayers[i];
                for (size_t j = 0; j < 5; j++) {
                    if (j >= player->vBeacons.size()) {
                        continue;
                    }
                    LloydBeacon &beacon = player->vBeacons[j];
                    std::string str = StringPrintf("lloyd%d%d.pcx", i + 1, j + 1);
                    //beacon.image = Image::Create(new PCX_LOD_Raw_Loader(pNew_LOD, str));
                    beacon.image = render->CreateTexture_PCXFromLOD(pNew_LOD, str);
                    beacon.image->GetWidth();
                }
            }
        }
    }

    {
        Blob timerBlob = pNew_LOD->LoadRaw("clock.bin");
        Timer_MM7 *serialization = (Timer_MM7*)timerBlob.data();
        if (serialization == nullptr) {
            logger->Warning(localization->FormatString(
                LSTR_FMT_SAVEGAME_CORRUPTED, 102).c_str());
        } else {
            Deserialize(*serialization, pEventTimer);
        }
    }

    {
        Blob blob = pNew_LOD->LoadRaw("overlay.bin");
        OtherOverlayList_MM7 *serialization = (OtherOverlayList_MM7*)blob.data();
        if (serialization == nullptr) {
            logger->Warning(localization->FormatString(
                LSTR_FMT_SAVEGAME_CORRUPTED, 103).c_str());
        } else {
            Deserialize(*serialization, pOtherOverlayList);
        }
    }

    {
        Blob blob = pNew_LOD->LoadRaw("npcdata.bin");
        NPCData_MM7 *serialization = (NPCData_MM7*)blob.data();
        if (serialization == nullptr) {
            logger->Warning(localization->FormatString(
                LSTR_FMT_SAVEGAME_CORRUPTED, 104).c_str());
        } else {
            for (unsigned int i = 0; i < 501; ++i) {
                Deserialize(serialization[i], &pNPCStats->pNewNPCData[i]);
            }
            pNPCStats->OnLoadSetNPC_Names();
        }
    }

    {
        Blob blob = pNew_LOD->LoadRaw("npcgroup.bin");
        void *npcgroup = blob.data();
        if (npcgroup == nullptr) {
            logger->Warning(localization->FormatString(
                LSTR_FMT_SAVEGAME_CORRUPTED, 105).c_str());
            __debugbreak();
        } else if (sizeof(pNPCStats->pGroups_copy) != 102) {
            logger->Warning("NPCStats: deserialization warning");
        } else {
            memcpy(pNPCStats->pGroups_copy, npcgroup, sizeof(pNPCStats->pGroups_copy));
        }
    }

    uActiveCharacter = 0;
    for (uint i = 0; i < 4; ++i) {
        if (pParty->pPlayers[i].CanAct()) {
            uActiveCharacter = i + 1;
            break;
        }
    }
/*
    for (uint i = 0; i < 4; ++i) {
        if (pParty->pPlayers[i].uQuickSpell) {
            AA1058_PartyQuickSpellSound[i].AddPartySpellSound(
                pParty->pPlayers[i].uQuickSpell, i + 1);
        }

        for (uint j = 0; j < 2; ++j) {
            uint uEquipIdx = pParty->pPlayers[i].pEquipment.pIndices[j];
            if (uEquipIdx) {
                int pItemID = pParty->pPlayers[i]
                                  .pInventoryItemList[uEquipIdx - 1]
                                  .uItemID;
                if (pItemTable->pItems[pItemID].uEquipType == EQUIP_WAND &&
                    pItemID) {       // жезл
                    __debugbreak();  // looks like offset in player's inventory
                                     // and wand_lut much like case in 0042ECB5
                    stru_A750F8[i].AddPartySpellSound(
                        wand_spell_ids[pItemID], i + 9);
                }
            }
        }
    }
*/
    current_screen_type = CURRENT_SCREEN::SCREEN_GAME;

    viewparams->bRedrawGameUI = true;

    SetUserInterface(pParty->alignment, true);

    pEventTimer->Resume();
    pEventTimer->StopGameTime();

    if (!pGames_LOD->DoesContainerExist(header->pLocationName)) {
        Error("Unable to find: %s!", header->pLocationName);
    }

    pCurrentMapName = header->pLocationName;

    dword_6BE364_game_settings_1 |= GAME_SETTINGS_LOADING_SAVEGAME_SKIP_RESPAWN | GAME_SETTINGS_0001;

    for (uint i = 0; i < uNumSavegameFiles; ++i) {
        if (pSavegameThumbnails[i] != nullptr) {
            pSavegameThumbnails[i]->Release();
            pSavegameThumbnails[i] = nullptr;
        }
    }

    // pAudioPlayer->SetMusicVolume(engine->config->music_level);
    // pAudioPlayer->SetMasterVolume(engine->config->sound_level);

    // TODO: what is this magic? old party position correction with current angle settings?
    if (engine->config->settings.TurnSpeed.Get() > 0) {
        pParty->sRotationZ = engine->config->settings.TurnSpeed.Get() * pParty->sRotationZ / engine->config->settings.TurnSpeed.Get();
    }
    MM7Initialization();

    // TODO: disable flashing for all books until we save state to savegame file
    bFlashQuestBook = false;
    bFlashAutonotesBook = false;
    bFlashHistoryBook = false;
    viewparams->bRedrawGameUI = true;
}

void SaveGame(bool IsAutoSAve, bool NotSaveWorld) {
    s_SavedMapName = pCurrentMapName;
    if (pCurrentMapName == "d05.blv") {  // arena
        return;
    }

    char *uncompressed_buff = (char *)malloc(1000000);

    int pPositionX = pParty->vPosition.x;
    int pPositionY = pParty->vPosition.y;
    int pPositionZ = pParty->vPosition.z;
    int sPRotationY = pParty->sRotationZ;
    int sPRotationX = pParty->sRotationY;
    pParty->vPosition.x = pParty->vPrevPosition.x;
    pParty->vPosition.z = pParty->vPrevPosition.z;
    pParty->vPosition.y = pParty->vPrevPosition.y;

    pParty->uFallStartZ = pParty->vPrevPosition.z;

    pParty->sRotationZ = pParty->sPrevRotationZ;
    pParty->sRotationY = pParty->sPrevRotationY;
    if (uCurrentlyLoadedLevelType == LEVEL_Indoor)
        pIndoor->stru1.last_visit = pParty->GetPlayingTime();
    else
        pOutdoor->loc_time.last_visit = pParty->GetPlayingTime();

    unsigned int buf_size = 0;
    render->PackScreenshot(150, 112, uncompressed_buff, 1000000, &buf_size);  // создание скриншота

    // saving - please wait

    // if (current_screen_type == SCREEN_SAVEGAME) {
    //    render->DrawTextureAlphaNew(8 / 640.0f, 8 / 480.0f,
    //        saveload_ui_loadsave);
    //    render->DrawTextureAlphaNew(18 / 640.0f, 141 / 480.0f,
    //        saveload_ui_loadsave);
    //    int text_pos = pFontSmallnum->AlignText_Center(186, localization->GetString(190));
    //    pGUIWindow_CurrentMenu->DrawText(pFontSmallnum, text_pos + 25, 219, 0,
    //        localization->GetString(190), 0, 0,
    //        0);  // Сохранение
    //    text_pos = pFontSmallnum->AlignText_Center(
    //        186, pSavegameHeader[uLoadGameUI_SelectedSlot].pName);
    //    pGUIWindow_CurrentMenu->DrawTextInRect(
    //        pFontSmallnum, text_pos + 25, 259, 0,
    //        pSavegameHeader[uLoadGameUI_SelectedSlot].pName, 185, 0);
    //    text_pos =
    //        pFontSmallnum->AlignText_Center(186, localization->GetString(LSTR_PLEASE_WAIT));
    //    pGUIWindow_CurrentMenu->DrawText(pFontSmallnum, text_pos + 25, 299, 0,
    //        localization->GetString(LSTR_PLEASE_WAIT), 0, 0,
    //        0);  // Пожалуйста, подождите
    //    render->Present();
    //}

    if (pNew_LOD->Write("image.pcx", uncompressed_buff, buf_size, 0)) {
        auto error_message = localization->FormatString(
            LSTR_FMT_SAVEGAME_CORRUPTED, 200);
        logger->Warning(error_message.c_str());
    }

    static_assert(sizeof(SavegameHeader) == 100, "Wrong type size");
    SavegameHeader save_header;
    memset(save_header.pName, 0, 20);
    memset(save_header.pLocationName, 0, 20);
    memset(save_header.field_30, 0, 52);
    strcpy(save_header.pLocationName, pCurrentMapName.c_str());
    save_header.playing_time = pParty->GetPlayingTime();
    if (pNew_LOD->Write("header.bin", &save_header, sizeof(SavegameHeader), 0)) {
        auto error_message = localization->FormatString(
            LSTR_FMT_SAVEGAME_CORRUPTED, 201);
        logger->Warning(error_message.c_str());
    }

    {
        Party_MM7 serialization;
        Serialize(*pParty, &serialization);

        if (pNew_LOD->Write("party.bin", &serialization, sizeof(serialization), 0)) {
            auto error_message = localization->FormatString(
                LSTR_FMT_SAVEGAME_CORRUPTED, 202);
            logger->Warning(error_message.c_str());
        }
    }

    {
        Timer_MM7 serialization;
        Serialize(*pEventTimer, &serialization);

        if (pNew_LOD->Write("clock.bin", &serialization, sizeof(serialization), 0)) {
            auto error_message = localization->FormatString(
                LSTR_FMT_SAVEGAME_CORRUPTED, 203);
            logger->Warning(error_message.c_str());
        }
    }

    {
        OtherOverlayList_MM7 serialization;
        Serialize(*pOtherOverlayList, &serialization);

        if (pNew_LOD->Write("overlay.bin", &serialization, sizeof(serialization), 0)) {
            auto error_message = localization->FormatString(
                LSTR_FMT_SAVEGAME_CORRUPTED, 204);
            logger->Warning(error_message.c_str());
        }
    }

    {
        NPCData_MM7 serialization[501];
        for (unsigned int i = 0; i < 501; ++i) {
            Serialize(pNPCStats->pNewNPCData[i], &serialization[i]);
        }

        if (pNew_LOD->Write("npcdata.bin", serialization, sizeof(serialization), 0)) {
            auto error_message = localization->FormatString(
                LSTR_FMT_SAVEGAME_CORRUPTED, 205);
            logger->Warning(error_message.c_str());
        }
    }

    if (pNew_LOD->Write("npcgroup.bin", pNPCStats->pGroups_copy, sizeof(pNPCStats->pGroups_copy), 0)) {
        auto error_message = localization->FormatString(
            LSTR_FMT_SAVEGAME_CORRUPTED, 206);
        logger->Warning(error_message.c_str());
    }

    for (size_t i = 0; i < 4; ++i) {  // 4 - players
        Player *player = &pParty->pPlayers[i];
        for (size_t j = 0; j < 5; ++j) {  // 5 - images
            if (j >= player->vBeacons.size()) {
                continue;
            }
            LloydBeacon *beacon = &player->vBeacons[j];
            Image *image = beacon->image;
            if ((beacon->uBeaconTime.Valid()) && (image != nullptr)) {
                const void *pixels = image->GetPixels(IMAGE_FORMAT_A8B8G8R8);
                if (!pixels)
                    __debugbreak();
                unsigned int pcx_data_size = 30000;
                void *pcx_data = malloc(pcx_data_size);
                PCX::Encode32(pixels, image->GetWidth(), image->GetHeight(),
                              pcx_data, pcx_data_size, &pcx_data_size);
                std::string str = StringPrintf("lloyd%d%d.pcx", i + 1, j + 1);
                if (pNew_LOD->Write(str, pcx_data, pcx_data_size, 0)) {
                    auto error_message = localization->FormatString(
                        LSTR_FMT_SAVEGAME_CORRUPTED, 207);
                    logger->Warning(error_message.c_str());
                }
                free(pcx_data);
            }
        }
    }

    if (!NotSaveWorld) {  // autosave for change location
        CompactLayingItemsList();
        char *compressed_buf = (char *)malloc(1000000);
        if (compressed_buf == nullptr) {
            logger->Warning("Malloc error");
            Error("Malloc");  // is this recoverable
        }
        ODMHeader *odm_data = (ODMHeader*)compressed_buf;
        odm_data->uVersion = 91969;
        odm_data->pMagic[0] = 'm';
        odm_data->pMagic[1] = 'v';
        odm_data->pMagic[2] = 'i';
        odm_data->pMagic[3] = 'i';
        odm_data->uCompressedSize = 0;
        odm_data->uDecompressedSize = 0;

        char *data_write_pos = uncompressed_buff;
        if (uCurrentlyLoadedLevelType == LEVEL_Indoor) {
            pIndoor->dlv.uNumFacesInBModels = pIndoor->pFaces.size();
            pIndoor->dlv.uNumBModels = 0;
            pIndoor->dlv.uNumDecorations = pLevelDecorations.size();
            memcpy(data_write_pos, &pIndoor->dlv, sizeof(DDM_DLV_Header));  // 0x28
            data_write_pos += sizeof(DDM_DLV_Header);
            memcpy(data_write_pos, &pIndoor->_visible_outlines, 0x36B);
            data_write_pos += 875;
            for (int i = 0; i < (signed int)pIndoor->pFaces.size(); ++i) {
                memcpy(data_write_pos, &pIndoor->pFaces[i].uAttributes, 4);
                data_write_pos += 4;
            }

            for (int i = 0; i < (signed int)pLevelDecorations.size(); ++i) {
                memcpy(data_write_pos, &pLevelDecorations[i].uFlags, 2);
                data_write_pos += 2;
            }

            uint32_t uNumActors = pActors.size();
            memcpy(data_write_pos, &uNumActors, 4);
            data_write_pos += 4;

            // memcpy(data_write_pos, &pActors, uNumActors * sizeof(Actor));
            // data_write_pos += uNumActors * sizeof(Actor);
            Actor_MM7* tmp_actor = (Actor_MM7*)malloc(sizeof(Actor_MM7));

            for (int i = 0; i < uNumActors; ++i) {
                Serialize(pActors[i], tmp_actor);
                memcpy(data_write_pos + i * sizeof(Actor_MM7), tmp_actor, sizeof(Actor_MM7));
            }
            free(tmp_actor);
            data_write_pos += uNumActors * sizeof(Actor_MM7);

            uint32_t uNumSpriteObjects = pSpriteObjects.size();
            memcpy(data_write_pos, &uNumSpriteObjects, 4);
            data_write_pos += 4;
            memcpy(data_write_pos, pSpriteObjects.data(), 112 * uNumSpriteObjects);
            data_write_pos += 112 * uNumSpriteObjects;

            data_write_pos += ChestsSerialize(data_write_pos);

            // memcpy(data_write_pos, pIndoor->pDoors, sizeof(BLVDoor) * 200);
            // data_write_pos += 16000;
            BLVDoor_MM7* tmp_door = (BLVDoor_MM7*)malloc(sizeof(BLVDoor_MM7));
            for (int i = 0; i < pIndoor->pDoors.size(); ++i) {
                Serialize(pIndoor->pDoors[i], tmp_door);
                memcpy(data_write_pos + i * sizeof(BLVDoor_MM7), tmp_door, sizeof(BLVDoor_MM7));
            }
            free(tmp_door);
            data_write_pos += pIndoor->pDoors.size() * sizeof(BLVDoor_MM7);

            memcpy(data_write_pos, pIndoor->ptr_0002B4_doors_ddata.data(), pIndoor->blv.uDoors_ddata_Size);
            data_write_pos += pIndoor->blv.uDoors_ddata_Size;
            memcpy(data_write_pos, &stru_5E4C90_MapPersistVars, 0xC8);
            data_write_pos += 200;
            memcpy(data_write_pos, &pIndoor->stru1, 0x38);
            data_write_pos += 56;

        } else {  // for Outdoor
            pOutdoor->ddm.uNumFacesInBModels = 0;
            for (BSPModel &model : pOutdoor->pBModels) {
                pOutdoor->ddm.uNumFacesInBModels += model.pFaces.size();
            }
            pOutdoor->ddm.uNumBModels = pOutdoor->pBModels.size();
            pOutdoor->ddm.uNumDecorations = pLevelDecorations.size();
            memcpy(data_write_pos, &pOutdoor->ddm, sizeof(DDM_DLV_Header));  // 0x28
            data_write_pos += sizeof(DDM_DLV_Header);
            memcpy(data_write_pos, pOutdoor->uFullyRevealedCellOnMap, 0x3C8);
            data_write_pos += 968;
            memcpy(data_write_pos, pOutdoor->uPartiallyRevealedCellOnMap, 0x3C8);
            data_write_pos += 968;
            for (BSPModel &model : pOutdoor->pBModels) {
                for (ODMFace &face : model.pFaces) {
                    memcpy(data_write_pos, &(face.uAttributes), 4);
                    data_write_pos += 4;
                }
            }

            for (size_t i = 0; i < pLevelDecorations.size(); ++i) {
                memcpy(data_write_pos, &pLevelDecorations[i].uFlags, 2);
                data_write_pos += 2;
            }
            uint32_t uNumActors = pActors.size();
            memcpy(data_write_pos, &uNumActors, 4);
            data_write_pos += 4;

            // memcpy(data_write_pos, &pActors, uNumActors * sizeof(Actor));
            // data_write_pos += uNumActors * sizeof(Actor);
            Actor_MM7* tmp_actor = (Actor_MM7*)malloc(sizeof(Actor_MM7));

            for (int i = 0; i < uNumActors; ++i) {
                Serialize(pActors[i], tmp_actor);
                memcpy(data_write_pos + i * sizeof(Actor_MM7), tmp_actor, sizeof(Actor_MM7));
            }
            free(tmp_actor);
            data_write_pos += uNumActors * sizeof(Actor_MM7);

            uint32_t uNumSpriteObjects = pSpriteObjects.size();
            memcpy(data_write_pos, &uNumSpriteObjects, 4);
            data_write_pos += 4;
            memcpy(data_write_pos, pSpriteObjects.data(),
                   uNumSpriteObjects * sizeof(SpriteObject));
            data_write_pos += uNumSpriteObjects * sizeof(SpriteObject);

            data_write_pos += ChestsSerialize(data_write_pos);

            memcpy(data_write_pos, &stru_5E4C90_MapPersistVars, 0xC8);
            data_write_pos += 200;
            memcpy(data_write_pos, &pOutdoor->loc_time, 0x38);
            data_write_pos += 56;
        }

        unsigned int compressed_block_size = 1000000 - sizeof(ODMHeader);
        size_t Size = data_write_pos - uncompressed_buff;
        int res = zlib::Compress(compressed_buf + sizeof(ODMHeader), &compressed_block_size, uncompressed_buff, Size);
        if (res || (compressed_block_size > Size)) {
            memcpy((void *)(compressed_buf + sizeof(ODMHeader)), uncompressed_buff, Size);
            compressed_block_size = Size;
        }

        odm_data->uCompressedSize = compressed_block_size;
        odm_data->uDecompressedSize = Size;

        std::string file_name = pCurrentMapName;
        size_t pos = file_name.find_last_of(".");
        file_name[pos + 1] = 'd';
        if (pNew_LOD->Write(file_name, compressed_buf, compressed_block_size + sizeof(ODMHeader), 0)) {
            auto error_message = localization->FormatString(
                LSTR_FMT_SAVEGAME_CORRUPTED, 208);
            logger->Warning(error_message.c_str());
        }
        free(compressed_buf);
    }
    free(uncompressed_buff);

    if (IsAutoSAve) {
        if (!CopyFile(MakeDataPath("data", "new.lod"), MakeDataPath("saves", "autosave.mm7"))) {
            logger->Warning("Copying of autosave.mm7 failed");
        }
    }
    pParty->vPosition.x = pPositionX;
    pParty->vPosition.y = pPositionY;
    pParty->vPosition.z = pPositionZ;
    pParty->uFallStartZ = pPositionZ;
    pParty->sRotationZ = sPRotationY;
    pParty->sRotationY = sPRotationX;
}

void DoSavegame(unsigned int uSlot) {
    if (pCurrentMapName != "d05.blv") {  // Not Arena(не Арена)
        SaveGame(0, 0);
        strcpy(pSavegameHeader[uSlot].pLocationName, pCurrentMapName.c_str());
        pSavegameHeader[uSlot].playing_time = pParty->GetPlayingTime();
        pNew_LOD->Write("header.bin", &pSavegameHeader[uSlot], sizeof(SavegameHeader), 0);
        pNew_LOD->CloseWriteFile();  //закрыть
        std::string file_name = StringPrintf("save%03d.mm7", uSlot);
        CopyFile(MakeDataPath("data", "new.lod"), MakeDataPath("saves", file_name));
    }
    GUI_UpdateWindows();
    pGUIWindow_CurrentMenu->Release();
    current_screen_type = CURRENT_SCREEN::SCREEN_GAME;

    viewparams->bRedrawGameUI = true;
    for (uint i = 0; i < MAX_SAVE_SLOTS; i++) {
        if (pSavegameThumbnails[i] != nullptr) {
            pSavegameThumbnails[i]->Release();
            pSavegameThumbnails[i] = nullptr;
        }
    }

    if (pCurrentMapName != "d05.blv")
        pNew_LOD->_4621A7();
    else
        GameUI_SetStatusBar(LSTR_NO_SAVING_IN_ARENA);

    pEventTimer->Resume();
    GameUI_SetStatusBar(LSTR_GAME_SAVED);
    viewparams->bRedrawGameUI = true;
}

void SavegameList::Initialize() {
    pSavegameList->Reset();
    uNumSavegameFiles = 0;

    std::string saves_dir = MakeDataPath("saves");

    if (std::filesystem::exists(saves_dir)) {
        for (const auto& entry : std::filesystem::directory_iterator(saves_dir)) {
            if (entry.path().extension() == ".mm7") {
                pSavegameList->pFileList[uNumSavegameFiles++] = entry.path().filename().string();
                if (uNumSavegameFiles == (MAX_SAVE_SLOTS - 1)) break;
            }
        }
    } else {
        logger->Warning("Couldn't find saves directory!");
    }

    if (uNumSavegameFiles)
        std::sort(&pSavegameList->pFileList[0], &pSavegameList->pFileList[uNumSavegameFiles]);
}

SavegameList::SavegameList() { Reset(); }

void SavegameList::Reset() {
    for (int j = 0; j < MAX_SAVE_SLOTS; j++) {
        this->pFileList[j].clear();
    }
}

void SaveNewGame() {
    if (pNew_LOD != nullptr) {
        pNew_LOD->CloseWriteFile();
    }

    std::string file_path = MakeDataPath("data", "new.lod");
    remove(file_path.c_str());  // удалить new.lod

    LOD::FileHeader header;  // заголовок
    strcpy(header.LodVersion, "MMVII");
    strcpy(header.LodDescription, "newmaps for MMVII");
    header.LODSize = 100;
    header.dword_0000A8 = 0;

    pNew_LOD->CreateNewLod(&header, "current", file_path);  // создаётся new.lod в дирректории
    if (pNew_LOD->LoadFile(file_path, false)) {  // загрузить файл new.lod(isFileOpened = true)
        pNew_LOD->CreateTempFile();  // создаётся временный файл OutputFileHandle
        pNew_LOD->ClearSubNodes();

        for (size_t i = pGames_LOD->GetSubNodesCount() / 2; i < pGames_LOD->GetSubNodesCount(); ++i) {  // копирование файлов с 76 по 151
            std::string name = pGames_LOD->GetSubNodeName(i);
            Blob data = pGames_LOD->LoadRaw(name);
            pNew_LOD->AppendDirectory(name, data.data(), data.size());
        }

        strcpy(pSavegameHeader[0].pLocationName, "out01.odm");
        pNew_LOD->AppendDirectory("header.bin", &pSavegameHeader[0], sizeof(SavegameHeader));

        pNew_LOD->FixDirectoryOffsets();

        pParty->vPrevPosition.x = 12552;
        pParty->vPrevPosition.y = 1816;
        pParty->vPrevPosition.z = 0;

        pParty->vPosition.x = 12552;
        pParty->vPosition.y = 1816;
        pParty->vPosition.z = 0;

        pParty->uFallStartZ = 0;

        pParty->sPrevRotationY = 0;
        pParty->sPrevRotationZ = 512;

        pParty->sRotationY = 0;
        pParty->sRotationZ = 512;

        SaveGame(1, 1);
    }
}
