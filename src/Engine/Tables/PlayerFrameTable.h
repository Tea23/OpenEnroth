#pragma once

#include "Engine/Objects/Player.h"

#include "Utility/Memory/Blob.h"

/*   46 */
#pragma pack(push, 1)
struct PlayerFrame {
    CHARACTER_EXPRESSION_ID expression;
    uint16_t uTextureID;
    int16_t uAnimTime;
    int16_t uAnimLength;
    int16_t uFlags;
};
#pragma pack(pop)

/*   47 */
#pragma pack(push, 1)
struct PlayerFrameTable {
    inline PlayerFrameTable() : uNumFrames(0), pFrames(nullptr) {}

    unsigned int GetFrameIdByExpression(CHARACTER_EXPRESSION_ID expression);
    PlayerFrame *GetFrameBy_x(unsigned int uFramesetID, unsigned int uFrameID);
    PlayerFrame *GetFrameBy_y(int *a2, int *a3, int a4);
    void ToFile();
    void FromFile(const Blob &data_mm6, const Blob &data_mm7, const Blob &data_mm8);
    int FromFileTxt(const char *Args);

    unsigned int uNumFrames;
    struct PlayerFrame *pFrames;
};
#pragma pack(pop)

extern struct PlayerFrameTable *pPlayerFrameTable;  // idb
