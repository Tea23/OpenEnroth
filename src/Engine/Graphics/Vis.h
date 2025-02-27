#pragma once

#include <variant>

#include "Engine/Graphics/IRender.h"
#include "Engine/Objects/Actor.h"
#include "Camera.h"

enum VisObjectType : uint32_t {
    VisObjectType_Any = 0,
    VisObjectType_Sprite = 1,
    VisObjectType_Face = 2
};

enum VisSelectFlags : uint32_t {
    None = 0,
    VisSelectFlags_1 = 1,  // not set in any of the standard filters. Used to check something that seems to be ally/enemy-based?
    ExcludeType = 2,
    ExclusionIfNoEvent = 4,
    TargetUndead = 8
};

/*  150 */
#pragma pack(push, 1)
// NOTE: The variable names here are correct when the filter is used for VisObjectType_Sprite, but wrong for VisObjectType_Face
struct Vis_SelectionFilter {  // stru157
    VisObjectType vis_object_type;
    ObjectType object_type;
    int at_ai_state;
    int no_at_ai_state;
    VisSelectFlags select_flags;
};
#pragma pack(pop)
extern Vis_SelectionFilter vis_sprite_filter_1;  // 00F93E1C
extern Vis_SelectionFilter vis_sprite_filter_2;  // 00F93E30
extern Vis_SelectionFilter vis_face_filter;      // 00F93E44
extern Vis_SelectionFilter vis_door_filter;      // 00F93E58
extern Vis_SelectionFilter vis_sprite_filter_3;  // 00F93E6C
extern Vis_SelectionFilter vis_sprite_filter_4;  // static to sub_44EEA7

#pragma pack(push, 1)
struct Vis_PIDAndDepth {
    uint16_t object_pid;
    int16_t depth;
};
#pragma pack(pop)

using Vis_Object = std::variant<std::monostate, int /* index */, ODMFace *, BLVFace *>;

#pragma pack(push, 1)
struct Vis_ObjectInfo {
    Vis_Object object;
    uint16_t object_pid = PID_INVALID;
    int16_t depth = -1;
    VisObjectType object_type = VisObjectType_Any;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Vis_SelectionList {
    enum class PointerCreationType { All = 0, Unique = 1 };
    using enum PointerCreationType;

    Vis_ObjectInfo *SelectionPointers(int a2, int a3);
    void create_object_pointers(PointerCreationType type = All);
    void sort_object_pointers();

    inline void AddObject(Vis_Object object, VisObjectType type, int depth, int pid) {
        object_pool[uSize].object = object;
        object_pool[uSize].object_type = type;
        object_pool[uSize].depth = depth;
        object_pool[uSize].object_pid = pid;
        uSize++;
    }

    std::array<Vis_ObjectInfo, 512> object_pool;
    std::array<Vis_ObjectInfo*, 512> object_pointers = {{}};
    unsigned int uSize = 0;
};
#pragma pack(pop)

/*  116 */
#pragma pack(push, 1)
class Vis {
 public:
    Vis();
    //----- (004C05A2) --------------------------------------------------------
    // virtual ~Vis() {}
    //----- (004C05BE) --------------------------------------------------------
    virtual ~Vis() {}
    bool PickKeyboard(float pick_depth, Vis_SelectionList *list,
                      Vis_SelectionFilter *sprite_filter,
                      Vis_SelectionFilter *face_filter);
    void PickBillboards_Keyboard(float pick_depth, Vis_SelectionList *list,
                                 Vis_SelectionFilter *filter);
    void PickIndoorFaces_Keyboard(float pick_depth, Vis_SelectionList *list,
                                  Vis_SelectionFilter *filter);
    void PickOutdoorFaces_Keyboard(float pick_depth, Vis_SelectionList *list,
                                   Vis_SelectionFilter *filter);

    bool PickMouse(float fDepth, float fMouseX, float fMouseY,
                   Vis_SelectionFilter *sprite_filter,
                   Vis_SelectionFilter *face_filter);
    void PickBillboards_Mouse(float fPickDepth, float fX, float fY,
                              Vis_SelectionList *list,
                              Vis_SelectionFilter *filter);
    void PickIndoorFaces_Mouse(float fDepth, struct RenderVertexSoft *pRay,
                               Vis_SelectionList *list,
                               Vis_SelectionFilter *filter);
    void PickOutdoorFaces_Mouse(float fDepth, struct RenderVertexSoft *pRay,
                                Vis_SelectionList *list,
                                Vis_SelectionFilter *filter,
                                bool only_reachable);

    bool is_part_of_selection(const Vis_Object &what,
                              Vis_SelectionFilter *filter);
    bool DoesRayIntersectBillboard(float fDepth, unsigned int uD3DBillboardIdx);
    Vis_ObjectInfo *DetermineFacetIntersection(struct BLVFace *face,
                                               unsigned int a3,
                                               float pick_depth);
    bool IsPolygonOccludedByBillboard(struct RenderVertexSoft *vertices,
                                      int num_vertices, float x, float y);
    void GetPolygonCenter(struct RenderVertexD3D3 *pVertices,
                          unsigned int uNumVertices, float *pCenterX,
                          float *pCenterY);
    void GetPolygonScreenSpaceCenter(struct RenderVertexSoft *vertices,
                                     int num_vertices, float *out_center_x,
                                     float *out_center_y);
    bool IsPointInsideD3DBillboard(struct RenderBillboardD3D *a1, float x,
                                   float y);
    unsigned short PickClosestActor(ObjectType object_type, unsigned int pick_depth,
                                    VisSelectFlags selectFlags, int not_at_ai_state, int at_ai_state);
    void _4C1A02();
    void SortVectors_x(RenderVertexSoft *pArray, int start, int end);
    Vis_PIDAndDepth get_object_zbuf_val(Vis_ObjectInfo *info);
    Vis_PIDAndDepth get_picked_object_zbuf_val();
    bool Intersect_Ray_Face(struct RenderVertexSoft *pRayStart,
                            struct RenderVertexSoft *pRayEnd, float *pDepth,
                            RenderVertexSoft *Intersection, BLVFace *pFace,
                            signed int pBModelID);
    bool CheckIntersectBModel(BLVFace *pFace, Vec3s IntersectPoint,
                              signed int sModelID);
    void CastPickRay(RenderVertexSoft *pRay, float fMouseX, float fMouseY,
                     float fPickDepth);
    void SortVerticesByX(struct RenderVertexD3D3 *pArray, unsigned int uStart,
                         unsigned int uEnd);
    void SortVerticesByY(struct RenderVertexD3D3 *pArray, unsigned int uStart,
                         unsigned int uEnd);
    void SortByScreenSpaceX(struct RenderVertexSoft *pArray, int start,
                            int end);
    void SortByScreenSpaceY(struct RenderVertexSoft *pArray, int start,
                            int end);

    Vis_SelectionList default_list;
    RenderVertexSoft stru_200C;
    RenderVertexSoft stru_203C;
    RenderVertexSoft stru_206C;
    RenderVertexSoft stru_209C;

    RenderVertexSoft debugpick;

    Logger *log = nullptr;
};
#pragma pack(pop)


/**
 * @param model                         Pointer to model to check against.
 * @param reachable_depth               A depth distance for checking interaction against.
 * @param[out] reachable                Whether the model is within the reachable depth specified.
 *
 * @return                              Whether the bounding radius of the model is visible within the camera frustum planes.
 */
bool IsBModelVisible(BSPModel *model, int reachable_depth, bool *reachable);

/**
 * @param center                        Vec3f of centre point of sphere.
 * @param radius                        Float of sphere radius.
 * @param frustum                       Ptr to vec4f set of planes to check against - camera used in not supplied
 *
 * @return                              Whether the bounding radius of the sphere is visible within the frustum planes.
 */
bool IsSphereInFrustum(Vec3f center, float radius, IndoorCameraD3D_Vec4* frustum = nullptr);

/**
 * @param center                        Vec3f of centre point of cylinder.
 * @param radius                        Float of cylinder radius.
 *
 * @return                              Whether the cylinder is visible within the L/R camera frustum planes.
 */
bool IsCylinderInFrustum(Vec3f center, float radius);
