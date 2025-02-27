#pragma once

#include <array>
#include <cstring>
#include <cstdint>

#include "Utility/Memory/Blob.h"

class Texture;

class Icon {
 public:
    inline Icon() : img(nullptr) {}

    inline void SetAnimationName(const char *name) {
        strcpy(this->anim_name, name);
    }
    inline const char *GetAnimationName() const { return anim_name; }

    inline void SetAnimLength(unsigned int anim_length) {
        this->anim_length = anim_length;
    }
    inline unsigned int GetAnimLength() const { return this->anim_length; }

    inline void SetAnimTime(unsigned int anim_time) {
        this->anim_time = anim_time;
    }
    inline unsigned int GetAnimTime() const { return this->anim_time; }

    Texture *GetTexture();

    ///* 000 */ char pAnimationName[12];
    /* 00C */ char pTextureName[12]{}; // TODO: std::string
    ///* 018 */ int16_t uAnimTime;
    ///* 01A */ int16_t uAnimLength;
    /* 01C */ int16_t uFlags = 0;
    ///* 01E */ uint16_t uTextureID;
    int id = 0;

 protected:
     char anim_name[64]{}; // TODO: std::string
    // char texture_name[64];
    // unsigned int anim_time;
    unsigned int anim_length = 0;
    unsigned int anim_time = 0;
    // unsigned int flags;
    // unsigned int texture_id;

    Texture *img = nullptr;
};

/*   45 */
#pragma pack(push, 1)
struct IconFrameTable {
    inline IconFrameTable() : uNumIcons(0), pIcons(nullptr) {}

    Icon *GetIcon(unsigned int idx);
    Icon *GetIcon(const char *pIconName);
    unsigned int FindIcon(const char *pIconName);
    Icon *GetFrame(unsigned int uIconID, unsigned int frame_time);
    void InitializeAnimation(unsigned int uIconID);
    void ToFile();
    void FromFile(const Blob &data_mm6, const Blob &data_mm7, const Blob &data_mm8);
    int FromFileTxt(const char *Args);
    // int GetIconAnimLength(unsigned int uIconID);

    unsigned int uNumIcons;
    Icon *pIcons;
};
#pragma pack(pop)

class UIAnimation {
 public:
    class Icon *icon;

    /* 002 */ int16_t field_2;
    /* 004 */ int16_t uAnimTime;
    /* 006 */ int16_t uAnimLength;
    /* 008 */ int16_t x;
    /* 00A */ int16_t y;
    /* 00C */ char field_C;
};

extern struct IconFrameTable *pIconsFrameTable;

extern UIAnimation *pUIAnim_Food;
extern UIAnimation *pUIAnim_Gold;
extern UIAnimation *pUIAnum_Torchlight;
extern UIAnimation *pUIAnim_WizardEye;

extern std::array<UIAnimation *, 4> pUIAnims;
