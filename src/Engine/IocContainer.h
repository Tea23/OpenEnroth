#pragma once

#include <memory>

struct BloodsplatContainer;
struct DecalBuilder;
class LightmapBuilder;
class Logger;
namespace Io {
    class Mouse;
}
class Nuklear;
class ParticleEngine;
struct SpellFxRenderer;
class Vis;

namespace Engine_ {

class IocContainer {
 public:
    static Logger *ResolveLogger();
    static DecalBuilder *ResolveDecalBuilder();
    static BloodsplatContainer *ResolveBloodsplatContainer();
    static SpellFxRenderer *ResolveSpellFxRenderer();
    static LightmapBuilder *ResolveLightmapBuilder();
    static std::shared_ptr<Io::Mouse> ResolveMouse();
    static std::shared_ptr<Nuklear> ResolveNuklear();
    static std::shared_ptr<ParticleEngine> ResolveParticleEngine();
    static Vis *ResolveVis();

 private:
     static DecalBuilder *decal_builder;
     static BloodsplatContainer *bloodspalt_container;
     static SpellFxRenderer *spell_fx_renderer;
     static LightmapBuilder *lightmap_builder;
     static std::shared_ptr<Io::Mouse> mouse;
     static std::shared_ptr<Nuklear> nuklear;
     static std::shared_ptr<ParticleEngine> particle_engine;
     static Vis *vis;
};

}  // namespace Engine_


extern Logger *logger;
