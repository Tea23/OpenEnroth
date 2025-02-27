#include "Engine/Graphics/DecalBuilder.h"

#include "Engine/Engine.h"
#include "Engine/Graphics/Camera.h"
#include "Engine/OurMath.h"
#include "Engine/Time.h"
#include "Engine/stru314.h"

#include "Outdoor.h"

#include "Engine/Graphics/ClippingFunctions.h"

//----- (0043B570) --------------------------------------------------------
float Decal::Fade_by_time() {
    // splats dont fade
    if (!(decal_flags & DecalFlagsFade)) return 1.0f;
    if (!engine->config->graphics.BloodSplatsFade.Get()) return 1.0f;

    // splats fade
    int64_t delta = fadetime - pEventTimer->Time();
    float result = (float(delta) + 3840.0f) / 3840.0f;
    if (result < 0.0f) result = 0.0f;
    return result;
}

//----- (0043B6EF) --------------------------------------------------------
void BloodsplatContainer::AddBloodsplat(float x, float y, float z, float radius,
    unsigned char r, unsigned char g, unsigned char b) {

    // this adds to store of bloodsplats to apply
    this->pBloodsplats_to_apply[this->uNumBloodsplats].pos.x = x;
    this->pBloodsplats_to_apply[this->uNumBloodsplats].pos.y = y;
    this->pBloodsplats_to_apply[this->uNumBloodsplats].pos.z = z;
    this->pBloodsplats_to_apply[this->uNumBloodsplats].radius = radius;
    this->pBloodsplats_to_apply[this->uNumBloodsplats].r = r;
    this->pBloodsplats_to_apply[this->uNumBloodsplats].g = g;
    this->pBloodsplats_to_apply[this->uNumBloodsplats].b = b;
    this->pBloodsplats_to_apply[this->uNumBloodsplats].dot_dist = 0;
    this->pBloodsplats_to_apply[this->uNumBloodsplats].blood_flags = DecalFlagsNone;
    this->pBloodsplats_to_apply[this->uNumBloodsplats].fade_timer = 0;

    this->uNumBloodsplats++;
    if (this->uNumBloodsplats == 64) this->uNumBloodsplats = 0;
}

//----- (0049B490) --------------------------------------------------------
void DecalBuilder::AddBloodsplat(float x, float y, float z, float r, float g, float b, float radius) {
    bloodsplat_container->AddBloodsplat(
        x, y, z, radius, bankersRounding(r * 255.0f),
        bankersRounding(g * 255.0f), bankersRounding(b * 255.0f));
}

//----- (0049B525) --------------------------------------------------------
void DecalBuilder::Reset(bool bPreserveBloodsplats) {
    if (!bPreserveBloodsplats) {
        bloodsplat_container->uNumBloodsplats = 0;
    }
    DecalsCount = 0;
}

//----- (0049B540) --------------------------------------------------------
char DecalBuilder::BuildAndApplyDecals(int light_level, LocationFlags locationFlags, stru154* FacePlane, int NumFaceVerts,
                                       RenderVertexSoft* FaceVerts, char ClipFlags, unsigned int uSectorID) {
    if (!NumFaceVerts) return 0;

    static stru314 static_FacePlane;
    static_FacePlane.Normal.y = FacePlane->face_plane.vNormal.y;
    static_FacePlane.Normal.x = FacePlane->face_plane.vNormal.x;
    static_FacePlane.Normal.z = FacePlane->face_plane.vNormal.z;
    static_FacePlane.dist = FacePlane->face_plane.dist;
    if (!pCamera3D->GetFacetOrientation(
        FacePlane->polygonType, &static_FacePlane.Normal, &static_FacePlane.field_10,
        &static_FacePlane.field_1C)) {
        log->Warning("Error: Failed to get the facet orientation");
        return 0;
    }

    if (this->uNumSplatsThisFace > 0) {
        for (int i = 0; i < this->uNumSplatsThisFace; ++i) {
            int thissplat = this->WhichSplatsOnThisFace[i];
            Bloodsplat* buildsplat = &bloodsplat_container->pBloodsplats_to_apply[thissplat];
            int point_light_level = GetLightLevelAtPoint(
                    light_level, uSectorID,
                    buildsplat->pos.x, buildsplat->pos.y, buildsplat->pos.z);

            int ColourMult = buildsplat->r | (buildsplat->g << 8) | (buildsplat->b << 16);
            int BloodSplatX = (int64_t)buildsplat->pos.x;

            if (!this->Build_Decal_Geometry(
                point_light_level, locationFlags,
                buildsplat,
                buildsplat->radius,
                ColourMult,
                buildsplat->dot_dist,
                &static_FacePlane, NumFaceVerts, FaceVerts, ClipFlags))
                log->Warning("Error: Failed to build decal geometry");
        }
    }
    return 1;
}

//----- (0049B790) --------------------------------------------------------
bool DecalBuilder::Build_Decal_Geometry(
    int LightLevel, LocationFlags locationFlags, Bloodsplat* blood, float DecalRadius,
    unsigned int uColorMultiplier, float DecalDotDist, stru314* FacetNormals, signed int numfaceverts,
    RenderVertexSoft* faceverts, char uClipFlags) {

    if (DecalRadius == 0.0f) return 1;
    Decal* decal = &this->Decals[this->DecalsCount];
    decal->fadetime = blood->fade_timer;
    decal->decal_flags = blood->blood_flags;

    // buildings decals dont fade ??
    if (locationFlags & LocationBuildings) decal->decal_flags = DecalFlagsNone;

    this->field_30C028 = DecalRadius - DecalDotDist;
    this->field_30C02C = sqrt((DecalRadius + DecalRadius - this->field_30C028) * this->field_30C028);

    this->flt_30C030 = 1.0 - (DecalRadius - this->field_30C02C) / DecalRadius;
    decal->DecalXPos = (int64_t)(blood->pos.x - DecalDotDist * FacetNormals->Normal.x);
    decal->DecalYPos = (int64_t)(blood->pos.y - DecalDotDist * FacetNormals->Normal.y);
    decal->DecalZPos = (int64_t)(blood->pos.z - DecalDotDist * FacetNormals->Normal.z);

    // for decal size
    this->field_30C034 = DecalRadius * this->flt_30C030;
    this->field_30C010 = this->field_30C034 * FacetNormals->field_10.x;
    this->field_30C014 = this->field_30C034 * FacetNormals->field_10.y;
    this->field_30C018 = this->field_30C034 * FacetNormals->field_10.z;

    this->field_30C01C = this->field_30C034 * FacetNormals->field_1C.x;
    this->field_30C020 = this->field_30C034 * FacetNormals->field_1C.y;
    this->field_30C024 = this->field_30C034 * FacetNormals->field_1C.z;

    // vertex position sizing
    decal->pVertices[0].vWorldPosition.x = (double)decal->DecalXPos - this->field_30C01C + this->field_30C010;
    decal->pVertices[0].vWorldPosition.y = (double)decal->DecalYPos - this->field_30C020 + this->field_30C014;
    decal->pVertices[0].vWorldPosition.z = (double)decal->DecalZPos - this->field_30C024 + this->field_30C018;
    decal->pVertices[0].u = 0.0;
    decal->pVertices[0].v = 0.0;

    decal->pVertices[1].vWorldPosition.x = (double)decal->DecalXPos - this->field_30C01C - this->field_30C010;
    decal->pVertices[1].vWorldPosition.y = (double)decal->DecalYPos - this->field_30C020 - this->field_30C014;
    decal->pVertices[1].vWorldPosition.z = (double)decal->DecalZPos - this->field_30C024 - this->field_30C018;
    decal->pVertices[1].u = 0.0;
    decal->pVertices[1].v = 1.0;

    decal->pVertices[2].vWorldPosition.x = (double)decal->DecalXPos + this->field_30C01C - this->field_30C010;
    decal->pVertices[2].vWorldPosition.y = (double)decal->DecalYPos + this->field_30C020 - this->field_30C014;
    decal->pVertices[2].vWorldPosition.z = (double)decal->DecalZPos + this->field_30C024 - this->field_30C018;
    decal->pVertices[2].u = 1.0;
    decal->pVertices[2].v = 1.0;

    decal->pVertices[3].vWorldPosition.x = (double)decal->DecalXPos + this->field_30C01C + this->field_30C010;
    decal->pVertices[3].vWorldPosition.y = (double)decal->DecalYPos + this->field_30C020 + this->field_30C014;
    decal->pVertices[3].vWorldPosition.z = (double)decal->DecalZPos + this->field_30C024 + this->field_30C018;
    decal->pVertices[3].u = 1.0;
    decal->pVertices[3].v = 0.0;

    // adjust to plane dist
    for (uint i = 0; i < 4; ++i) {
        double dotdist = FacetNormals->Normal.x * (double)decal->pVertices[i].vWorldPosition.x +
            FacetNormals->Normal.y * (double)decal->pVertices[i].vWorldPosition.y +
            FacetNormals->Normal.z * (double)decal->pVertices[i].vWorldPosition.z + FacetNormals->dist;
        decal->pVertices[i].vWorldPosition.x =
            decal->pVertices[i].vWorldPosition.x - dotdist * FacetNormals->Normal.x;
        decal->pVertices[i].vWorldPosition.y =
            decal->pVertices[i].vWorldPosition.y - dotdist * FacetNormals->Normal.y;
        decal->pVertices[i].vWorldPosition.z =
            decal->pVertices[i].vWorldPosition.z - dotdist * FacetNormals->Normal.z;
    }

    decal->uColorMultiplier = uColorMultiplier;
    decal->uNumVertices = 4;
    decal->DimmingLevel = LightLevel;

    // clip decals to face
    bool result = engine->pStru9Instance->ClipVertsToFace(
        faceverts, numfaceverts, FacetNormals->Normal.x, FacetNormals->Normal.y, FacetNormals->Normal.z, decal->pVertices,
        (signed int*)&decal->uNumVertices);

    if (result) {
        // no verts then discard
        if (!decal->uNumVertices) return 1;

        // otherwise keep this decal
        this->DecalsCount++;
        if (this->DecalsCount == 1024) this->DecalsCount = 0;
        return 1;
    }

    return result;
}

//----- (0049BBBD) --------------------------------------------------------
bool DecalBuilder::ApplyBloodsplatDecals_IndoorFace(unsigned int uFaceID) {
    // reset splat count
    uNumSplatsThisFace = 0;
    BLVFace* pFace = &pIndoor->pFaces[uFaceID];

    if (pFace->Indoor_sky() || pFace->Fluid()) return true;
    for (uint i = 0; i < bloodsplat_container->uNumBloodsplats; ++i) {
        Bloodsplat* pBloodsplat = &bloodsplat_container->pBloodsplats_to_apply[i];
        if (pFace->pBounding.Expanded(pBloodsplat->radius).Contains(pBloodsplat->pos.ToShort())) {
            double dotdist = Dot(pFace->pFacePlane.vNormal, pBloodsplat->pos) + pFace->pFacePlane.dist;
            if (dotdist <= pBloodsplat->radius) {
                // store splat
                pBloodsplat->dot_dist = dotdist;
                WhichSplatsOnThisFace[uNumSplatsThisFace++] = i;
            }
        }
    }

    return true;
}

//----- (0049BCEB) --------------------------------------------------------
bool DecalBuilder::ApplyBloodSplat_OutdoorFace(ODMFace* pFace) {
    // reset splat count
    this->uNumSplatsThisFace = 0;

    // loop through and check
    if (!pFace->Indoor_sky() && !pFace->Fluid()) {
        for (int i = 0; i < bloodsplat_container->uNumBloodsplats; i++) {
            Bloodsplat* pBloodsplat = &bloodsplat_container->pBloodsplats_to_apply[i];
            if (pFace->pBoundingBox.Expanded(pBloodsplat->radius).Contains(pBloodsplat->pos.ToShort())) {
                double dotdist = pFace->pFacePlaneOLD.SignedDistanceTo(
                    round_to_int(pBloodsplat->pos.x), round_to_int(pBloodsplat->pos.y), round_to_int(pBloodsplat->pos.z));
                if (dotdist <= pBloodsplat->radius) {
                    // store splat
                    pBloodsplat->dot_dist = dotdist;
                    this->WhichSplatsOnThisFace[this->uNumSplatsThisFace++] = i;
                }
            }
        }
    }

    return true;
}

//----- (0049BE8A) --------------------------------------------------------
// apply outdoor blodsplats - check to see if bloodsplat hits terrain triangle
bool DecalBuilder::ApplyBloodSplatToTerrain(struct Polygon *terrpoly, Vec3f *terrnorm, float *tridotdist,
                                            RenderVertexSoft *triverts, unsigned int uStripType, char tri_orient, int whichsplat) {
    // tracks how many decals are applied to this tri
    this->uNumSplatsThisFace = 0;

    if (!bloodsplat_container->uNumBloodsplats) return false;
    unsigned int NumBloodsplats = bloodsplat_container->uNumBloodsplats;

    if (NumBloodsplats > 0) {
       // check plane distance
       *tridotdist = -Dot(triverts->vWorldPosition, *terrnorm);
       float planedist = Dot(*terrnorm, bloodsplat_container->pBloodsplats_to_apply[whichsplat].pos) + *tridotdist + 0.5f;

        if (planedist <= bloodsplat_container->pBloodsplats_to_apply[whichsplat].radius) {
            // blood splat hits this terrain tri

            // check if water or something else (maybe should be border tile or swampy?)
            // TODO(pskelton): Convert flags to enums
            if (terrpoly->flags & 2 || terrpoly->flags & 0x100) {
                // apply fade flags
                if (!(bloodsplat_container->pBloodsplats_to_apply[whichsplat].blood_flags & DecalFlagsFade)) {
                    bloodsplat_container->pBloodsplats_to_apply[whichsplat].blood_flags |= DecalFlagsFade;
                    bloodsplat_container->pBloodsplats_to_apply[whichsplat].fade_timer = pEventTimer->Time();
                }
            }

            bloodsplat_container->pBloodsplats_to_apply[whichsplat].fade_timer = pEventTimer->Time();
            bloodsplat_container->pBloodsplats_to_apply[whichsplat].dot_dist = planedist;

            // store this decal to apply
            this->WhichSplatsOnThisFace[this->uNumSplatsThisFace] = whichsplat;
            ++this->uNumSplatsThisFace;
        }
    }

    return true;
}

//----- (0049C2CD) --------------------------------------------------------
void DecalBuilder::DrawDecals(float z_bias) {
    for (uint i = 0; i < DecalsCount; ++i)
        render->DrawDecal(&Decals[i], z_bias);
}

//----- (0049C304) --------------------------------------------------------
void DecalBuilder::DrawBloodsplats() {
    if (!DecalsCount) return;

    render->BeginDecals();
    DrawDecals(0.00039999999f);
    render->EndDecals();
}

//----- (0049C550) --------------------------------------------------------
void DecalBuilder::DrawDecalDebugOutlines() {
    for (int i = 0; i < DecalsCount; i++)
        pCamera3D->debug_outline_sw(Decals[i].pVertices, Decals[i].uNumVertices, 0xC86400u, 0.0f);
}

//----- (0040E4C2) --------------------------------------------------------
void Decal::Decal_base_ctor() {
    uNumVertices = -1;
    for (uint i = 0; i < 64; ++i) pVertices[i].flt_2C = 0.0f;
}
