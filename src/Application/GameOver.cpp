#include <string>

#include "GameOver.h"

#include "Engine/AssetsManager.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/IRender.h"
#include "Engine/Graphics/Viewport.h"
#include "Engine/LOD.h"
#include "Engine/Localization.h"
#include "Engine/Party.h"
#include "Engine/SaveLoad.h"
#include "Engine/Time.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIFont.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIPartyCreation.h"
#include "GUI/UI/UIStatusBar.h"

#include "Io/KeyboardInputHandler.h"
#include "Io/Mouse.h"

#include "Media/Audio/AudioPlayer.h"
#include "Media/MediaPlayer.h"


namespace Application {

//----- (004BF91E) --------------------------------------------------------
void GameOver_Loop(int v15) {
    const char *v1;       // eax@2
    unsigned int result;  // eax@3
    const char *v7;       // edx@10
    const char *v8;       // ecx@12
    const char *v9;       // eax@14
    // unsigned int v10; // eax@25
    GUIWindow pWindow;      // [sp+34h] [bp-9Ch]@1
    unsigned int v14;       // [sp+A4h] [bp-2Ch]@5
    const char *pInString;  // [sp+ACh] [bp-24h]@5
    unsigned int v17;       // [sp+B0h] [bp-20h]@5
    unsigned int v18;       // [sp+B4h] [bp-1Ch]@5
    unsigned int v19;       // [sp+B8h] [bp-18h]@5
    int v20;                // [sp+BCh] [bp-14h]@7
    GUIFont *pFont;         // [sp+C4h] [bp-Ch]@1
    uint64_t v23;   // [sp+C8h] [bp-8h]@5

    dword_6BE364_game_settings_1 &= ~GAME_SETTINGS_4000;
    bGameoverLoop = true;
    pAudioPlayer->PauseSounds(-1);
    render->BeginScene();
    render->ClearBlack();
    render->EndScene();
    render->Present();
    // pMediaPlayer->pResetflag = 0;
    _449B57_test_bit(pParty->_quest_bits, QBIT_LIGHT_PATH);

    Image *background = assets->GetImage_PCXFromIconsLOD("winbg.pcx");
    {
        render->BeginScene();
        render->DrawTextureNew(0, 0, background);
        render->EndScene();
    }
    background->Release();
    background = nullptr;

    window_SpeakInHouse = new GUIWindow(WINDOW_Unknown, {0, 0}, render->GetRenderDimensions(), 0);
    pWindow.uFrameX = 75;
    pWindow.uFrameY = 60;
    pWindow.uFrameWidth = 469;
    pWindow.uFrameHeight = 338;
    pWindow.uFrameZ = 543;
    pWindow.uFrameW = 397;
    pFont = GUIFont::LoadFont("endgame.fnt", "FONTPAL");
    if (pParty->IsPartyGood())
        v1 = localization->GetString(LSTR_GOOD_ENDING);
    else if (pParty->IsPartyEvil())
        v1 = localization->GetString(LSTR_EVIL_ENDING);
    else
        return;
    pInString = v1;

    auto play_time = pParty->GetPlayingTime();
    play_time.SubtractHours(9);  // game begins at 9 am

    v19 = play_time.GetDays();
    v14 = play_time.GetYears();
    v18 = play_time.GetMonthsOfYear();
    v17 = play_time.GetDaysOfMonth();
    if (!v19) v19 = 1;
    render->BeginScene();
    pWindow.DrawTitleText(
        pFont, 1, 0x23, 1, localization->GetString(LSTR_CONGRATULATIONS), 3
    );
    v23 = 0ull;
    v20 = 0;
    for (uint i = 0; i < 4; i++) {
        pWindow.DrawTitleText(
            pFont, 1,
            i * ((unsigned char)pFont->GetHeight() - 2) +
            (unsigned char)pFont->GetHeight() + 46,
            1,
            localization->FormatString(
                LSTR_FMT_S_THE_LEVEL_D_S,
                pParty->pPlayers[i].pName.c_str(),
                pParty->pPlayers[i].GetBaseLevel(),
                localization->GetClassName(pParty->pPlayers[i].classType)),
            3);
        v23 += pParty->pPlayers[i].uExperience;
    }
    v23 = (int64_t)v23 / v19;
    std::string v6 = pFont->FitTextInAWindow(pInString, pWindow.uFrameWidth, 12);
    pWindow.DrawTitleText(pFont, 1, 5 * (pFont->GetHeight() + 11), 1, v6, 0);

    v7 = localization->GetString(LSTR_DAY_CAPITALIZED);
    if (v17 != 1) v7 = localization->GetString(LSTR_DAYS);

    v8 = localization->GetString(LSTR_MONTH);
    if (v18 != 1) v8 = localization->GetString(LSTR_MONTHS);

    v9 = localization->GetString(LSTR_YEAR);
    if (v14 != 1) v9 = localization->GetString(LSTR_YEARS);

    pWindow.DrawTitleText(
        pFont, 1, pWindow.uFrameHeight - 2 * pFont->GetHeight() - 5, 1,
        localization->GetString(LSTR_TOTAL_TIME) + StringPrintf(
            " %lu %s, %lu %s, %lu %s ", v14, v9, v18, v8, v17, v7),
        3
    );

    pWindow.DrawTitleText(pFont, 1, pWindow.uFrameHeight, 1,
        localization->FormatString(LSTR_FMT_YOUR_SCORE_D, v23), 3);
    dword_6BE364_game_settings_1 |= GAME_SETTINGS_4000;
    render->EndScene();
    render->Present();
    // if ( render->pRenderD3D )
    render->pBeforePresentFunction();
    render->SaveWinnersCertificate("MM7_Win.Pcx");
    free(pFont);
    window_SpeakInHouse->Release();
    window_SpeakInHouse = 0;
    if (v15 == 2) {
        result = pMessageQueue_50CBD0->qMessages.size();
    } else {
        HEXRAYS_LODWORD(v23) = platform->TickCount() + 5000;
        while ((unsigned int)v23 > platform->TickCount()) {}

        eventLoop->ProcessMessages(eventHandler);

        pMessageQueue_50CBD0->Flush();
        keyboardInputHandler->ResetKeys();
        do {
            eventLoop->ProcessMessages(eventHandler);
        } while (keyboardActionMapping->IsKeyMatchAction(InputAction::Escape, keyboardInputHandler->LastPressedKey()));
        pMessageQueue_50CBD0->Flush();
    }
    if (v15) {
        /*if ( (signed int)result < 40 )
        {
        pMessageQueue_50CBD0->pMessages[result].eType = UIMSG_Quit;
        }*/
        pMessageQueue_50CBD0->AddGUIMessage(UIMSG_Quit, 1, 0);
    } else {
        pMessageQueue_50CBD0->AddGUIMessage(UIMSG_ShowFinalWindow, 1, 0);
        /*if ( (signed int)result < 40 )
        {
        pMessageQueue_50CBD0->pMessages[result].eType = UIMSG_C5;
        pMessageQueue_50CBD0->pMessages[pMessageQueue_50CBD0->uNumMessages].param
        = 1; result = 3 * pMessageQueue_50CBD0->uNumMessages + 3;
        *(&pMessageQueue_50CBD0->uNumMessages + result) = 0;
        ++pMessageQueue_50CBD0->uNumMessages;
        }*/
    }
    bGameoverLoop = false;
}

} // namespace Application
