#include "Engine/Graphics/Image.h"

#include <algorithm>

#include "Engine/Engine.h"
#include "Engine/OurMath.h"
#include "Engine/ZlibWrapper.h"

#include "Engine/Graphics/ImageFormatConverter.h"
#include "Engine/Graphics/ImageLoader.h"
#include "Engine/Graphics/Texture.h"

#include "Engine/Serialization/LegacyImages.h"

#include "Library/Serialization/EnumSerialization.h"


struct TextureFrameTable *pTextureFrameTable;

stru355 stru_4E82A4 = {0x20, 0x41, 0, 0x20, 0xFF0000, 0xFF00, 0xFF, static_cast<int>(0xFF000000)};
stru355 stru_4EFCBC = {0x20, 0x41, 0, 0x10, 0x7C00, 0x3E0, 0x1F, 0x8000};

// Texture_MM7 pTex_F7CE30;

MM_DEFINE_ENUM_MAGIC_SERIALIZATION_FUNCTIONS(IMAGE_FORMAT)

unsigned int IMAGE_FORMAT_BytesPerPixel(IMAGE_FORMAT format) {
    switch (format) {
        case IMAGE_FORMAT_R5G6B5:
            return 2;
        case IMAGE_FORMAT_A1R5G5B5:
            return 2;
        case IMAGE_FORMAT_A8R8G8B8:
            return 4;
        case IMAGE_FORMAT_R8G8B8:
            return 3;
        case IMAGE_FORMAT_R8G8B8A8:
            return 4;
        case IMAGE_FORMAT_A8B8G8R8:
            return 4;

        default:
            Error("Invalid format: %d", format);
            return 0;
    }
}

Texture *TextureFrame::GetTexture() {
    if (!this->tex) {
        this->tex = assets->GetBitmap(this->name);
    }
    return this->tex;
}

void TextureFrameTable::ToFile() {
    FILE *file = fopen(MakeDataPath("data", "dtft.bin").c_str(), "wb");
    if (file == nullptr)
        Error("Unable to save dtft.bin!", 0);

    // TODO the code below won't work because this static_assert fails
    // static_assert(sizeof(TextureFrameTable) == 0x14u);
    __debugbreak();

    fwrite(&this->sNumTextures, 4u, 1u, file);
    fwrite(this->pTextures, 0x14u, this->sNumTextures, file);
    fclose(file);
}

void TextureFrameTable::FromFile(const Blob &data_mm6, const Blob &data_mm7, const Blob &data_mm8) {
    uint num_mm6_frames = data_mm6 ? *(int *)data_mm6.data() : 0,
         num_mm7_frames = data_mm7 ? *(int *)data_mm7.data() : 0,
         num_mm8_frames = data_mm8 ? *(int *)data_mm8.data() : 0;

    this->sNumTextures =
        /*num_mm6_frames + */ num_mm7_frames /*+ num_mm8_frames*/;
    Assert(sNumTextures, 0);
    // Assert(!num_mm8_frames);

    TextureFrame_MM7 *frame_data =
        (TextureFrame_MM7 *)((unsigned char *)data_mm7.data() + 4);
    auto frames = new TextureFrame[this->sNumTextures];
    for (unsigned int i = 0; i < this->sNumTextures; ++i) {
        frames[i].name = frame_data->pTextureName.data();
        std::transform(frames[i].name.begin(), frames[i].name.end(),
                       frames[i].name.begin(), ::tolower);

        frames[i].uAnimLength = frame_data->uAnimLength;
        frames[i].uAnimTime = frame_data->uAnimTime;
        frames[i].uFlags = frame_data->uFlags;

        ++frame_data;
    }

    // pTextures = (TextureFrame *)malloc(sNumTextures * sizeof(TextureFrame));
    // memcpy(pTextures,                                   (char *)data_mm7 + 4,
    // num_mm7_frames * sizeof(TextureFrame)); memcpy(pTextures + num_mm7_frames,
    // (char *)data_mm6 + 4, num_mm6_frames * sizeof(TextureFrame));
    // memcpy(pTextures + num_mm6_frames + num_mm7_frames, (char *)data_mm8 + 4,
    // num_mm8_frames * sizeof(TextureFrame));

    this->pTextures = frames;
}

void TextureFrameTable::LoadAnimationSequenceAndPalettes(int uFrameID) {
    if (uFrameID <= this->sNumTextures && uFrameID >= 0) {
        for (unsigned int i = uFrameID;; ++i) {
            // this->pTextures[i].uTextureID =
            // pBitmaps_LOD->LoadTexture(this->pTextures[i].pTextureName,
            // TEXTURE_DEFAULT);

            // if (this->pTextures[i].uTextureID != -1)
            //    pBitmaps_LOD->pTextures[this->pTextures[i].uTextureID].palette_id2
            //    =
            //    pPaletteManager->LoadPalette(pBitmaps_LOD->pTextures[this->pTextures[i].uTextureID].palette_id1);

            if (this->pTextures[i].uFlags & 1) break;
        }
    }
    return;
}

int64_t TextureFrameTable::FindTextureByName(const char *Str2) {
    std::string name = Str2;
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    for (unsigned int i = 0; i < this->sNumTextures; ++i) {
        if (this->pTextures[i].name == name) return i;
    }
    return -1;
}

Texture *TextureFrameTable::GetFrameTexture(int uFrameID, signed int a3) {
    int v3 = uFrameID;
    TextureFrame *v4 = this->pTextures;
    TextureFrame *v5 = &v4[uFrameID];
    int v6 = v5->uAnimLength;
    if (v5->uFlags & 1 && (v6 != 0)) {
        int v7 = (a3 >> 3) % v6;
        for (char *i = (char *)&v5->uAnimTime;; i += 20) {
            int v9 = *(short *)i;
            if (v7 <= v9) break;
            v7 -= v9;
            ++v3;
        }
        return v4[v3].GetTexture();
    } else {
        return v5->GetTexture();
    }
}

void Texture_MM7::Release() {
    header.pName[0] = 0;

    if (header.pBits & 0x0400) {
        __debugbreak();
    }

    if (paletted_pixels != nullptr) {
        free(paletted_pixels);
        paletted_pixels = nullptr;
    }

    if (pPalette24 != nullptr) {
        free(pPalette24);
        pPalette24 = nullptr;
    }

    pLevelOfDetail1 = nullptr;

    memset(&header, 0, sizeof(header));
}

Texture_MM7::Texture_MM7() {
    memset(&header, 0, sizeof(header));
    paletted_pixels = nullptr;
    pLevelOfDetail1 = nullptr;
    pPalette24 = nullptr;
}

Image *Image::Create(ImageLoader *loader) {
    Image *img = new Image();
    if (img) {
        img->loader = loader;
    }

    return img;
}

bool Image::LoadImageData() {
    if (!initialized) {
        void *data = nullptr;
        void *palette = nullptr;
        initialized = loader->Load(&width, &height, &data, &native_format, &palette);
        if (initialized && native_format != IMAGE_INVALID_FORMAT) {
            pixels[native_format] = data;
            palette24 = palette;
        }
    }

    if ((width == 0 || height == 0) && initialized) __debugbreak();

    return initialized;
}

int Image::GetWidth() {
    if (!initialized) {
        LoadImageData();
    }

    if (initialized) {
        if (width == 0) __debugbreak();
        return width;
    }

    return 0;
}

int Image::GetHeight() {
    if (!initialized) {
        LoadImageData();
    }

    if (initialized) {
        if (height == 0) __debugbreak();
        return height;
    }

    return 0;
}

Image *Image::Create(unsigned int width, unsigned int height,
                     IMAGE_FORMAT format, const void *pixels) {
    if (width == 0 || height == 0) __debugbreak();

    Image *img = new Image(false);
    if (img) {
        img->initialized = true;
        img->width = width;
        img->height = height;
        img->native_format = format;
        unsigned int num_pixels = img->GetWidth() * img->GetHeight();
        unsigned int num_pixels_bytes =
            num_pixels * IMAGE_FORMAT_BytesPerPixel(format);
        img->pixels[format] = new unsigned char[num_pixels_bytes];
        if (pixels) {
            memcpy(img->pixels[format], pixels, num_pixels_bytes);
        } else {
            memset(img->pixels[format], 0, num_pixels_bytes);
        }

        return img;

    } else {
        return nullptr;
    }
}

const void *Image::GetPixels(IMAGE_FORMAT format) {
    if (!initialized) {
        LoadImageData();
    }

    if (initialized) {
        if (this->pixels[format]) {
            return this->pixels[format];
        }

        auto native_pixels = this->pixels[this->native_format];
        if (native_pixels) {
            static constexpr IndexedArray<IndexedArray<ImageFormatConverter, IMAGE_NUM_FORMATS>, IMAGE_NUM_FORMATS> converters = {
                {IMAGE_FORMAT_R5G6B5, {
                    {IMAGE_FORMAT_R5G6B5,       nullptr},
                    {IMAGE_FORMAT_A1R5G5B5,     nullptr},
                    {IMAGE_FORMAT_A8R8G8B8,     nullptr},
                    {IMAGE_FORMAT_R8G8B8,       nullptr},
                    {IMAGE_FORMAT_R8G8B8A8,     nullptr},
                    {IMAGE_FORMAT_A8B8G8R8,     Image_R5G6B5_to_A8B8G8R8}
                }},

                {IMAGE_FORMAT_A1R5G5B5, {
                    {IMAGE_FORMAT_R5G6B5,       nullptr},
                    {IMAGE_FORMAT_A1R5G5B5,     nullptr},
                    {IMAGE_FORMAT_A8R8G8B8,     nullptr},
                    {IMAGE_FORMAT_R8G8B8,       nullptr},
                    {IMAGE_FORMAT_R8G8B8A8,     nullptr},
                    {IMAGE_FORMAT_A8B8G8R8,     Image_A1R5G5B5_to_A8B8G8R8}
                }},

                {IMAGE_FORMAT_A8R8G8B8, {
                    {IMAGE_FORMAT_R5G6B5,       nullptr},
                    {IMAGE_FORMAT_A1R5G5B5,     nullptr},
                    {IMAGE_FORMAT_A8R8G8B8,     nullptr},
                    {IMAGE_FORMAT_R8G8B8,       nullptr},
                    {IMAGE_FORMAT_R8G8B8A8,     nullptr},
                    {IMAGE_FORMAT_A8B8G8R8,     Image_A8R8G8B8_to_A8B8G8R8}
                }},

                {IMAGE_FORMAT_R8G8B8, {
                    {IMAGE_FORMAT_R5G6B5,       nullptr},
                    {IMAGE_FORMAT_A1R5G5B5,     nullptr},
                    {IMAGE_FORMAT_A8R8G8B8,     nullptr},
                    {IMAGE_FORMAT_R8G8B8,       nullptr},
                    {IMAGE_FORMAT_R8G8B8A8,     nullptr},
                    {IMAGE_FORMAT_A8B8G8R8,     Image_R8G8B8_to_A8B8G8R8}
                }},

                {IMAGE_FORMAT_R8G8B8A8, {
                    {IMAGE_FORMAT_R5G6B5,       nullptr},
                    {IMAGE_FORMAT_A1R5G5B5,     nullptr},
                    {IMAGE_FORMAT_A8R8G8B8,     nullptr},
                    {IMAGE_FORMAT_R8G8B8,       nullptr},
                    {IMAGE_FORMAT_R8G8B8A8,     nullptr},
                    {IMAGE_FORMAT_A8B8G8R8,     nullptr}
                }},

                {IMAGE_FORMAT_A8B8G8R8, {
                    {IMAGE_FORMAT_R5G6B5,       nullptr},
                    {IMAGE_FORMAT_A1R5G5B5,     nullptr},
                    {IMAGE_FORMAT_A8R8G8B8,     Image_A8B8G8R8_to_A8R8G8B8},
                    {IMAGE_FORMAT_R8G8B8,       nullptr},
                    {IMAGE_FORMAT_R8G8B8A8,     nullptr},
                    {IMAGE_FORMAT_A8B8G8R8,     nullptr}
                }}
            };

            ImageFormatConverter cvt = converters[this->native_format][format];
            if (cvt) {
                unsigned int num_pixels = this->GetWidth() * this->GetHeight();

                void *cvt_pixels =
                    new unsigned char[num_pixels *
                                      IMAGE_FORMAT_BytesPerPixel(format)];
                if (cvt(width * height, native_pixels, cvt_pixels)) {
                    return this->pixels[format] = cvt_pixels;
                } else {
                    delete[] cvt_pixels;
                    cvt_pixels = nullptr;
                }
            } else {
                logger->Warning("No ImageConverter defined from %s to %s",
                                toString(this->native_format).c_str(),
                                toString(format).c_str());
            }
        }
    }
    return nullptr;
}


/**
 * New function.
 *
 * @return                              Returns pointer to image R8G8B8 palette. Size 3 * 256.
 */
const void *Image::GetPalette() {
    if (!initialized) {
        LoadImageData();
    }

    if (initialized) {
        return this->palette24;
    }

    return nullptr;
}

/**
 * New function.
 *
 * @return                              Returns pointer to image pixels 8 bit palette lookup. Size 1 * width * height.
 */
const void *Image::GetPalettePixels() {
    if (!initialized) {
        LoadImageData();
    }

    if (initialized) {
        return this->palettepixels;
    }

    return nullptr;
}

std::string* Image::GetName() {
    if (!loader) __debugbreak();
    return loader->GetResourceNamePtr();
}


bool Image::Release() {
    if (loader) {
        assets->ReleaseImage(loader->GetResourceName());
        assets->ReleaseSprite(loader->GetResourceName());
    }

    if (initialized) {
        if (loader) {
            delete loader;
            loader = nullptr;
        }

        for (void *ptr : pixels)
            delete[] ptr;
        pixels.fill(nullptr);

        native_format = IMAGE_INVALID_FORMAT;
        width = 0;
        height = 0;
    }

    delete this;
    return true;
}
